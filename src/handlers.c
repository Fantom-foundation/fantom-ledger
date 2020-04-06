#include <stdlib.h>
#include "handlers.h"
#include "state.h"
#include "get_version.h"
#include "get_pub_key.h"
#include "get_address.h"
#include "get_tx_sign.h"

// getHandler implements APDU instruction to handler mapping.
// The APDU protocol uses single byte instruction code (INS)
// to specify which command is to be executed on the device.
// We use this code to lookup for a handler function.
// See doc/app_design.md for list of instructions.
handler_fn_t *getHandler(uint8_t ins) {
    switch (ins) {
        case INS_VERSION:
            return handleGetVersion;

        case INS_GET_KEY:
            return handleGetPublicKey;

        case INS_GET_ADDR:
            return handleGetAddress;

        case INS_SIGN_TX:
            return handleSignTransaction;

        default:
            // we return NULL for unknown instructions
            // so the main loop can throw ERR_UNKNOWN_INS error
            return NULL;
    }
}
