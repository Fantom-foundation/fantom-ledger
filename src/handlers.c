#include <stdlib.h>
#include "handlers.h"

// getHandler implements APDU instruction to handler mapping.
// The APDU protocol uses single byte instruction code (INS)
// to specify which command is to be executed on the device.
// We use this code to lookup for a handler function.
// See doc/app_design.md for list of instructions.
handler_fn_t *getHandler(uint8_t ins) {
    switch (ins) {
        case 0x01:
            return handleGetVersion;

        case 0x10:
            return handleGetPublicKey;

        case 0x11:
            return handleGetAddress;

        case 0x20:
            return handleSignTransaction;

        default:
            // we return NULL for unknown instructions
            // so the main loop can throw ERR_UNKNOWN_INS error
            return NULL;
    }
}
