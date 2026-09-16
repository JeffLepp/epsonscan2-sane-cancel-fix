#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
  echo "usage: BOOST_INCLUDE=/path/to/boost $0 /path/to/epsonscan2-6.7.80.0-1" >&2
  exit 2
fi

source_root=$1
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
backend=$source_root/src/SaneWrapper/backend.cpp
expected_backend=6b2d0fbdf6f37cac80a22eba496c8d1b9691626c3a0b5ada9f8e7a96a25779ea

if [ ! -f "$backend" ]; then
  echo "missing Epson backend: $backend" >&2
  exit 2
fi

actual_backend=$(sha256sum "$backend" | awk '{print $1}')
if [ "$actual_backend" != "$expected_backend" ]; then
  echo "refusing unexpected backend: $actual_backend" >&2
  exit 2
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
cp -a "$source_root/." "$work/source"
git -C "$work/source" apply "$script_dir/0001-sane-cancel-active-scans-on-first-request.patch"

boost_include=${BOOST_INCLUDE:-/usr/include}
if [ ! -f "$boost_include/boost/preprocessor/cat.hpp" ]; then
  echo "Boost headers not found under $boost_include" >&2
  exit 2
fi

includes="-I$boost_include -Isrc -Isrc/SaneWrapper -Isrc/Standalone -Isrc/bin -Isrc/CommonUtility -Isrc/CommonUtility/utils"

cd "$work/source"
# Compile the actual patched source before replacing logging in the isolated
# test copy. This catches errors hidden by the logging test seam.
# shellcheck disable=SC2086
g++ -std=c++11 -DBUILDSANE=1 $includes -fsyntax-only src/SaneWrapper/backend.cpp

python3 - "$work/source/src/SaneWrapper/message.h" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()
old = '#define SANE_TRACE_LOG(message,...) AfxGetLog()->MessageLog(ENUM_LOG_LEVEL::LogLevelTrace,"SANEWrapper" ,__FUNCTION__,__FILE__,__LINE__, message, ##__VA_ARGS__ )'
new = '#define SANE_TRACE_LOG(message,...) ((void)0)'
if text.count(old) != 1:
    raise SystemExit('unexpected SANE_TRACE_LOG definition')
path.write_text(text.replace(old, new))
PY

flags="-std=c++11 -DBUILDSANE=1 -ffunction-sections -fdata-sections"
# shellcheck disable=SC2086
g++ $flags $includes -c src/SaneWrapper/backend.cpp -o "$work/backend.o"
# shellcheck disable=SC2086
g++ $flags $includes -c src/Standalone/supervisor.cpp -o "$work/supervisor.o"
# shellcheck disable=SC2086
g++ $flags $includes -c "$script_dir/cancel_contract_test.cpp" -o "$work/test.o"
g++ -Wl,--gc-sections "$work/backend.o" "$work/supervisor.o" "$work/test.o" \
  -ldl -pthread -o "$work/cancel-contract-test"
"$work/cancel-contract-test"
echo "patched cancellation contract: PASS"
