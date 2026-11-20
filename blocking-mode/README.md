# Explicit blocking mode returns an error

After starting a scan, an app can call `sane_set_io_mode(handle, SANE_FALSE)` to
explicitly select blocking reads. The Epson backend uses that mode by default,
but the call returns `SANE_STATUS_UNSUPPORTED`. An app that treats the error as
fatal can give up on the scan. [SANE requires blocking requests to succeed during acquisition](https://sane-project.gitlab.io/standard/1.05/api.html#sane-set-io-mode).

The [patch](0001-sane-accept-blocking-io-mode.patch) returns `SANE_STATUS_GOOD`
for blocking requests and `SANE_STATUS_UNSUPPORTED` for nonblocking requests.
The original function failed all three blocking requests in a five-request
test. The patched function passed all five requests.

## Run the test

Extract the [Epson Scan 2 6.7.80.0-1 source](https://support.epson.net/linux/en/epsonscan2.php).
With Python 3, Git, a C++ compiler, and SANE development headers installed, run
this from the repo root.

```sh
python3 blocking-mode/verify.py /path/to/epsonscan2-6.7.80.0-1
```

The script verifies `backend.cpp` against the reviewed source hash, then
compiles the original and patched function in a temporary directory with a
test handle. This checks the return values in isolation. Full-driver and
hardware tests are pending.

The blocking I/O patch and test code are under [LGPL-2.1-or-later](../COPYING).
