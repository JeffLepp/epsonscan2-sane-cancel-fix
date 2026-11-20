// SPDX-License-Identifier: LGPL-2.1-or-later
// No scanner is opened. The target function does not dereference its handle.
#include <sane/sane.h>
#include <cstdio>

int main()
{
    int synthetic_handle_storage = 0;
    SANE_Handle handle = &synthetic_handle_storage;
    const SANE_Bool requests[] = {
        SANE_FALSE, SANE_FALSE, SANE_TRUE, SANE_FALSE, SANE_TRUE
    };
    int failures = 0;
    for (unsigned i = 0; i < sizeof(requests) / sizeof(requests[0]); ++i) {
        const SANE_Status expected = requests[i] == SANE_FALSE
            ? SANE_STATUS_GOOD : SANE_STATUS_UNSUPPORTED;
        const SANE_Status actual = sane_set_io_mode(handle, requests[i]);
        if (actual != expected) {
            std::fprintf(stderr, "request[%u] non_blocking=%d: expected %d, got %d\n",
                         i, requests[i], expected, actual);
            ++failures;
        }
    }
    if (failures) {
        return 1;
    }
    std::puts("PASS: blocking accepted; nonblocking unsupported; repeated requests stable");
    return 0;
}
