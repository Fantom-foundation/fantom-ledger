#ifndef FANTOM_LEDGER_GET_TX_SIGN_H
#define FANTOM_LEDGER_GET_TX_SIGN_H

#include "common.h"
#include "handlers.h"
#include "transaction.h"
#include "tx_stream.h"
#include "bip44.h"

// WEI_TO_FTM_DECIMALS defines how many decimals we need to push
// to convert between WEI units used for transaction amounts and human readable FTMs
#define WEI_TO_FTM_DECIMALS 18

// handleSignTransaction implements Sign Transaction APDU instruction handler.
handler_fn_t handleSignTransaction;

// tx_stage_t declares stages of the transaction signature building
typedef enum {
    SIGN_STAGE_NONE = 0,
    SIGN_STAGE_INIT = 1,
    SIGN_STAGE_COLLECT = 2,
    SIGN_STAGE_FINALIZE = 4,
    SIGN_STAGE_DONE = 8,
} tx_stage_t;

// ins_sign_tx_context_t declares context
// for transaction signature building APDU instruction
typedef struct {
    int16_t responseReady;
    bip44_path_t path;
    transaction_t tx;
    tx_stream_context_t stream;
    cx_sha3_t sha3Context;
    tx_signature_t signature;
    tx_stage_t stage;
    int uiStep;
} ins_sign_tx_context_t;

#endif //FANTOM_LEDGER_GET_TX_SIGN_H
