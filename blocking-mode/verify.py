#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Apply the patch to a temporary copy and test Epson's actual function.

Requires Python 3, git, g++, and SANE development headers. Does not build the
whole Epson package or open hardware. Logging alone is stubbed in the isolated
translation unit; the function under test is extracted from backend.cpp.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

EXPECTED = '6b2d0fbdf6f37cac80a22eba496c8d1b9691626c3a0b5ada9f8e7a96a25779ea'
HERE = Path(__file__).resolve().parent


def run(argv, **kwargs):
    completed = subprocess.run(argv, capture_output=True, text=True, timeout=30, **kwargs)
    return {'argv': [str(arg) for arg in argv], 'returncode': completed.returncode,
            'stdout': completed.stdout, 'stderr': completed.stderr}


def test_function(source, directory, label):
    matches = re.findall(r'^SANE_Status\s*\nsane_set_io_mode\s*\([^)]*\)\s*\n\{.*?^\}',
                         source, flags=re.MULTILINE | re.DOTALL)
    if len(matches) != 1:
        raise ValueError('Expected one sane_set_io_mode definition')
    unit = directory / (label + '.cpp')
    unit.write_text('#include <sane/sane.h>\n#define SANE_TRACE_LOG(...) ((void)0)\n'
                    'extern "C" {\n' + matches[0] + '\n}\n')
    binary = directory / label
    compilation = run(['g++', '-std=c++11', '-Wall', '-Wextra', '-Werror', str(unit),
                       str(HERE / 'io_mode_contract_test.cpp'), '-o', str(binary)])
    if compilation['returncode']:
        raise RuntimeError(compilation['stderr'])
    execution = run([str(binary)])
    return {'compile': compilation, 'run': execution}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source_root', type=Path, help='Epson source root containing src/SaneWrapper/backend.cpp')
    parser.add_argument('--report', type=Path, help='Write a new JSON report; refuses overwrite')
    args = parser.parse_args()
    source_file = args.source_root / 'src/SaneWrapper/backend.cpp'
    original_bytes = source_file.read_bytes()
    digest = hashlib.sha256(original_bytes).hexdigest()
    if digest != EXPECTED:
        raise SystemExit('Unrecognized backend.cpp SHA-256: ' + digest)
    with tempfile.TemporaryDirectory(prefix='epson-io-mode-') as tmp:
        directory = Path(tmp)
        target = directory / 'src/SaneWrapper/backend.cpp'
        target.parent.mkdir(parents=True)
        target.write_bytes(original_bytes)
        patch = HERE / '0001-sane-accept-blocking-io-mode.patch'
        check = run(['git', 'apply', '--check', str(patch)], cwd=directory)
        if check['returncode']:
            raise RuntimeError(check['stderr'])
        apply = run(['git', 'apply', str(patch)], cwd=directory)
        if apply['returncode']:
            raise RuntimeError(apply['stderr'])
        original = test_function(original_bytes.decode(), directory, 'original')
        patched = test_function(target.read_text(), directory, 'patched')
        success = original['run']['returncode'] == 1 and patched['run']['returncode'] == 0
        report = {'source_sha256': digest, 'patch_applied': True,
                  'method': 'actual function extracted from source; logging stubbed',
                  'full_backend_build': False, 'hardware_test': False,
                  'original': original, 'patched': patched, 'regression_verified': success}
        rendered = json.dumps(report, indent=2) + '\n'
        if args.report:
            with args.report.open('x') as stream:
                stream.write(rendered)
        print(rendered, end='')
        raise SystemExit(0 if success else 1)


if __name__ == '__main__':
    main()
