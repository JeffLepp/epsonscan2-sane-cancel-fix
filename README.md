# Epson Scan 2 SANE backend first-call cancellation fix

This continues the SANE `scanimage` cancellation work in
[`f493dede`](https://gitlab.com/sane-project/backends/-/commit/f493dede5e93057e1ac447646b141fa4faaf710e):
that change reports cancellation correctly after a forced second-signal abort;
this repository fixes the Epson backend behavior that can make the forced abort
necessary.

Epson Scan 2 6.7.80 does not send the SDK cancel operation on the first
`sane_cancel()` call during an active scan. The first call only sets
`cancel_requested`; a second call is required to reach
`kSDIOperationTypeCancel`. A frontend that follows the SANE lifecycle and calls
once can remain blocked in the active operation.

This patch sends the SDK cancel operation on the first active request. Repeated
calls remain idempotent, cancellation before acquisition remains a no-op, and
the existing completed-scan cleanup is preserved.

## Files

- [`0001-sane-cancel-active-scans-on-first-request.patch`](0001-sane-cancel-active-scans-on-first-request.patch) — the backend change
- [`cancel_contract_test.cpp`](cancel_contract_test.cpp) — regression coverage for active, repeated, pre-start, and completed cancellation
- [`verify.sh`](verify.sh) — applies the patch to pristine 6.7.80 source and runs the test
- [`evidence-summary.md`](evidence-summary.md) — reproduction, build, and hardware results
- [`test-plan.md`](test-plan.md) — remaining upstream lifecycle coverage

## Result so far

The regression test fails against pristine 6.7.80 because the first call sends
zero SDK cancel operations. It passes after the patch, and the patched
`sane-epsonscan2` shared-library target builds successfully.

On an Epson Perfection V39, the equivalent two-call frontend workaround
cancelled active 600 and 1200 DPI scans and allowed an immediate successful
retry without resetting USB or power-cycling the scanner. The patched backend
itself still needs hardware coverage before release.

## Verify

Download `epsonscan2-6.7.80.0-1.src.tar.gz` from Epson's
[Linux scanner source page](https://support.epson.net/linux/en/epsonscan2.php),
extract it, then run:

```sh
BOOST_INCLUDE=/path/to/boost ./verify.sh /path/to/epsonscan2-6.7.80.0-1
```

`BOOST_INCLUDE` is optional when Boost headers are under `/usr/include`.

The patch targets Epson's published LGPL-2.1-or-later SANE wrapper. It does not
modify or redistribute Epson's proprietary scanner plug-in.
