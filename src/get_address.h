#ifndef FANTOM_LEDGER_GET_ADDRESS_H
#define FANTOM_LEDGER_GET_ADDRESS_H

#include "common.h"
#include "bip44.h"
#include "handlers.h"

// handleGetAddress implements Get Address APDU instruction handler.
handler_fn_t handleGetAddress;

// ins_get_address_context_t declares context
// for address derivation APDU instruction.
typedef struct {
    uint16_t responseReady;
    bip44_path_t path;
    cx_sha3_t sha3Context;
    struct {
        uint8_t size;
        uint8_t buffer[64];
    } address;
    bool isShowAddress;
    int uiStep;
} ins_get_address_context_t;

#endif //FANTOM_LEDGER_GET_ADDRESS_H
