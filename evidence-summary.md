# Reproduction and recovery evidence

## Scope

- Scanner: Epson Perfection V39 / GT-S650, USB product `04b8:013d`.
- Installed backend: Epson Scan 2 6.7.61.
- Reviewed source: Epson Scan 2 6.7.80.0-1.
- Source archive SHA-256:
  `f8d617892e75fb43404824e627a4c5894f7e1283bb9840d7cdecfbcef24b2097`.

The source test and hardware exercises were separate.

## Reproduction

Epson's published 6.7.80 `src/SaneWrapper/backend.cpp` contains this state
transition:

| Call | `cancel_requested` | SDK operations |
|---|---:|---|
| First `sane_cancel()` | true | none |
| Second `sane_cancel()` | true | `1` (`kSDIOperationTypeCancel`) |

The first call sets the flag. The second invokes the SDK cancellation
operation. An isolated callback test against the installed 6.7.61 backend
produced the same result without opening or accessing a scanner.

## Hardware recovery

An application-level frontend used the standard SANE ABI and issued the two
calls from a cancellation thread while the acquisition thread could be blocked
inside SANE. It waited for cancellation to return before closing the handle.

- Active 600-DPI cancellation returned SANE cancelled, required no forced
  process kill, and reached observed cleanup in 4.033 seconds.
- No final or partial TIFF was published for the cancelled attempt.
- The same scanner was immediately reopened and completed a valid 600-DPI
  retry without USB reset, scanner power cycle, or host restart.
- The final candidate repeated cancellation/reopen/retry at 1200 DPI. The
  retry produced a valid 10200 by 14040 RGB8 TIFF.
- Repeated-signal and close-order tests verified that one two-call sequence was
  sent and the handle was not closed until it returned.

## Candidate source test

The included source-linked harness replaces the SDK callbacks with counters.
It covers the first active request, repeated cancellation, cancellation before
start, and completed-scan cleanup.

- Patched 6.7.80 source: pass.
- Pristine 6.7.80 source: fails because the first active cancellation produces
  zero SDK operations instead of one.
- Patched 6.7.80 `sane-epsonscan2` CMake target: builds successfully as an
  x86-64 shared library in an isolated Ubuntu 22.04 container.

## Limits

The candidate source patch has not been packaged or run on hardware. Hardware
recovery was demonstrated with the installed 6.7.61 binary and an equivalent
frontend workaround. Epson should build the patch against its current internal
source and exercise additional models, connection types, timeouts,
unplug/replug, and sustained cancellation cycles.
