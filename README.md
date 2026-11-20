# Epson Scan 2 SANE backend fixes

I started with a cancellation problem on an Epson Perfection V39. This repo
collects the driver patches and regression tests from that work. Both patches
target Epson Scan 2 6.7.80.0-1 for Linux.

## The first cancel request leaves the scan running

The first `sane_cancel()` call during a scan sets a flag. The second call tells
the scanner SDK to stop. The patch sends the stop request on the first call.
Epson accepted this fix.

[Patch](0001-sane-cancel-active-scans-on-first-request.patch) · [Test and results](cancellation.md)

## Explicit requests for blocking I/O return an error

The driver already uses blocking reads. When a frontend asks for that mode
during a scan, `sane_set_io_mode(handle, SANE_FALSE)` returns
`SANE_STATUS_UNSUPPORTED`. The patch makes that call return `SANE_STATUS_GOOD`.

I reproduced the error in the shipped Linux library. The source regression
test failed all three blocking requests before the patch and passed all five
cases after it. The test exercises the function in isolation.

[Patch](blocking-mode/0001-sane-accept-blocking-io-mode.patch) · [Test and results](blocking-mode/README.md)
