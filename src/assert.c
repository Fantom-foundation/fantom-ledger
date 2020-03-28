#include "common.h"
#include "assert.h"

// assert implements the assertion test
// on PRODUCTION mode it will call Reset on the device
// on DEBUG mode we print the assertion and continue with the
// flow since it takes several loops of SEPROXYHAL to render on display.
void assert(
        int cond,
        const char *msgStr
#ifdef RESET_ON_CRASH
        MARK_UNUSED
#endif
) {
    // condition is met, nothing to do
    if (cond) return;

    #ifdef RESET_ON_CRASH
    io_seproxyhal_se_reset();
    #else
    {
        #ifdef DEVEL
        {
            ui_displayPaginatedText("Assertion failed", msgStr, NULL);
            THROW(ERR_ASSERT);
        }
        #endif
    }
    #endif
}
