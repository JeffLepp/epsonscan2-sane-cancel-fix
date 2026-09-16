# Cancellation test plan

## Automated contract tests

1. Open a valid scanner handle without starting acquisition. Call
   `sane_cancel()` and verify that no SDK scan-job operation is issued.
2. Start acquisition and block the active SANE operation in a fake SDK.
3. Call `sane_cancel()` once. Verify that exactly one
   `kSDIOperationTypeCancel` operation is issued and the blocked operation can
   return `SANE_STATUS_CANCELLED`.
4. Call `sane_cancel()` repeatedly before the blocked operation returns. Verify
   that no additional SDK cancellation operation is issued.
5. Complete a scan normally, call `sane_cancel()` as required by the SANE
   frontend lifecycle, and verify that the existing transfer-event cleanup
   still occurs without issuing an active-scan cancellation.
6. Close only after the cancelled operation returns. Verify one close and one
   dispose, with no use of the handle afterward.

## Hardware lifecycle tests

Run each supported connection path and representative scanner family. The
current evidence covers a USB Epson Perfection V39 only.

1. Cancel during `sane_start()` or the first blocking transfer wait.
2. Cancel during an active 600-DPI scan.
3. Repeat cancellation while the first request is pending.
4. Verify that no complete or partial result is published.
5. Close the handle and immediately reopen the same scanner.
6. Complete and independently validate a retry scan.
7. Repeat at the device's high supported resolution.
8. Exercise timeout, disconnect/reconnect, and process-exit cleanup.
9. Run at least 20 cancellation/reopen cycles to expose retained device or
   process state.

## Acceptance criteria

- One `sane_cancel()` initiates SDK cancellation.
- Repeated calls are safe and idempotent.
- The pending SANE operation returns without requiring process termination.
- Cleanup does not race cancellation or access a released handle.
- The same device can complete the next scan without USB reset, host restart,
  or scanner power cycle.
