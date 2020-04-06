#ifndef FANTOM_LEDGER_GET_TX_SIGN_H
#define FANTOM_LEDGER_GET_TX_SIGN_H

#include "common.h"
#include "handlers.h"
#include "transaction.h"

// handleSignTransaction implements Sign Transaction APDU instruction handler.
handler_fn_t handleSignTransaction;

// tx_stage_t declares stages of the transaction signature building
typedef enum {
    SIGN_STAGE_NONE = 0,
    SIGN_STAGE_COLLECT = 2,
    SIGN_STAGE_FINALIZE = 4,
} tx_stage_t;

// ins_sign_tx_context_t declares context
// for transaction signature building APDU instruction
typedef struct {
    transaction_t tx;
    tx_stage_t stage;
    int uiStep;
} ins_sign_tx_context_t;

#endif //FANTOM_LEDGER_GET_TX_SIGN_H
