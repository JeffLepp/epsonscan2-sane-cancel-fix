# SANE blocking I/O returns `SANE_STATUS_UNSUPPORTED`

In Epson Scan 2 6.7.80.0-1 for Linux, `sane_set_io_mode(handle, SANE_FALSE)`
returns `SANE_STATUS_UNSUPPORTED` during a scan. The backend already uses
blocking reads, so a frontend asking for the mode it is already using gets an
error. [SANE 1.05, section 4.3.12](https://sane-project.gitlab.io/standard/1.05/api.html#sane-set-io-mode)

The [patch](0001-sane-accept-blocking-io-mode.patch) changes the return value in
`src/SaneWrapper/backend.cpp`. Blocking requests return `SANE_STATUS_GOOD`.
Nonblocking requests return `SANE_STATUS_UNSUPPORTED`.

## Test results

I reproduced the error by calling the function in Epson's shipped Linux
library. For the [regression test](io_mode_contract_test.cpp), I compiled the
function from the original and patched source. The test makes five calls,
including a blocking request after a rejected nonblocking request. The
original function failed all three blocking requests. The patched function
passed all five cases. These were isolated function tests. A full driver build
and scanner test are pending.

## Verify

Extract `epsonscan2-6.7.80.0-1.src.tar.gz`, or use the
[source mirror at the tested revision](https://github.com/mfeldheim/epsonscan2/tree/301c366f73a4bd648c07cce9c8730ee92f2bef92).
Run this from the repo root with Python 3, Git, `g++`, and the SANE development
headers installed.

```sh
python3 blocking-mode/verify.py /path/to/epsonscan2-6.7.80.0-1
```

[`verify.py`](verify.py) checks the hash of `backend.cpp`, applies the patch
in a temporary directory, and compiles and runs both versions of the function.

The blocking I/O patch and test code are under [LGPL-2.1-or-later](../COPYING).
