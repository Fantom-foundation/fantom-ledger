#include <string.h>

#include "common.h"
#include "get_tx_sign.h"
#include "policy.h"
#include "state.h"
#include "derive_key.h"
#include "ux.h"
#include "big_endian_io.h"
#include "address_utils.h"
#include "ui_helpers.h"
#include "tx_stream.h"
#include "bip44.h"
#include "transaction.h"

// ctx keeps local reference to the transaction signature building context
static ins_sign_tx_context_t *ctx = &(instructionState.insSignTxContext);

// RESPONSE_READY_TAG is used to tag output buffer when tx signature is ready.
static uint16_t RESPONSE_READY_TAG = 11711;

// what are possible scenarios of the tx signature handling
// @see /doc/cmd_sign_tx.md for details.
enum {
    P1_NEW_TRANSACTION = 0x00,
    P1_STREAM_DATA = 0x01,
    P1_GET_SIGNATURE = 0x80,
};

// ASSERT_STAGE implements stage validation so the host can not step out off the protocol.
// It's just a cosmetic definition to make the code readable and express our intention better.
static inline void ASSERT_STAGE(tx_stage_t expected) {
    VALIDATE(ctx->stage == expected, ERR_INVALID_STATE);
}

// runSignTransactionInitUIStep implements next step in UX flow of the tx signing initialization flow (the 1st APDU).
static void runSignTransactionInitUIStep();

// runSignTransactionUIStep implements next step in UX flow of the tx signing flow (the final APDU).
static void runSignTransactionUIStep();

// what UX steps we support for starting a new transaction
enum {
    UI_STEP_INIT_WARNING = 100,
    UI_STEP_INIT_CONFIRM,
    UI_STEP_INIT_RESPOND,
    UI_STEP_INIT_INVALID,
};

// what UX steps we support for finishing the transaction signature
enum {
    UI_STEP_TX_RECIPIENT = 200,
    UI_STEP_TX_AMOUNT,
    UI_STEP_TX_FEE,
    UI_STEP_TX_CONFIRM,
    UI_STEP_TX_RESPOND,
    UI_STEP_TX_INVALID,
};

// handleSignTxInit implements TX signature building initialization APDU message.
// It's the first step in signing the transaction where the source address is calculated
// and the whole process is confirmed.
static void handleSignTxInit(uint8_t p2, uint8_t *wireBuffer, size_t wireSize) {
    // make sure we are on the right stage; nothing should have happened before this step
    ASSERT_STAGE(SIGN_STAGE_NONE);

    // validate the p2 value
    VALIDATE(p2 == 0, ERR_INVALID_PARAMETERS);

    // current stage is to init a new transaction
    ctx->stage = SIGN_STAGE_INIT;

    // clear the response ready tag
    ctx->responseReady = 0;

    // initialize the incoming tx data stream
    txStreamInit(&ctx->stream, &ctx->sha3Context, &ctx->tx);

    // parse BIP44 path from the incoming request
    size_t parsedSize = bip44_parseFromWire(&ctx->path, wireBuffer, wireSize);

    // make sure size of the data we parsed corresponds with the data we received
    if (parsedSize != wireSize) {
        THROW(ERR_INVALID_DATA);
    }

    // get the security policy for new transaction from a given address
    security_policy_t policy = policyForSignTxInit(&ctx->path);
    ASSERT_NOT_DENIED(policy);

    // decide what UI step to take first based on policy
    switch (policy) {
        case POLICY_WARN:
            // warn about unusual address request
            ctx->uiStep = UI_STEP_INIT_WARNING;
            break;
        case POLICY_PROMPT:
            ctx->uiStep = UI_STEP_INIT_CONFIRM;
            break;
        case POLICY_ALLOW:
            ctx->uiStep = UI_STEP_INIT_RESPOND;
            break;
        default:
            // if no policy was set, terminate the action
            ASSERT(false);
    }

    // run the first step
    runSignTransactionInitUIStep();
}

// runSignTransactionInitUIStep implements next step in UX flow of the tx signing initialization flow (the 1st APDU).
static void runSignTransactionInitUIStep() {
    // make sure we are on the right stage
    ASSERT_STAGE(SIGN_STAGE_INIT);

    // keep reference to self so we can use it as a callback to resume UI
    ui_callback_fn_t *this_fn = runSignTransactionInitUIStep;

    // resume the stage based on previous result
    switch (ctx->uiStep) {
        case UI_STEP_INIT_WARNING: {
            // display the warning
            ui_displayPaginatedText(
                    "Unusual request",
                    "Be careful!",
                    this_fn
            );

            // set next step
            ctx->uiStep = UI_STEP_INIT_CONFIRM;
            break;
        }

        case UI_STEP_INIT_CONFIRM: {
            // ask user to confirm the key export
            ui_displayPrompt(
                    "Start new",
                    "transaction?",
                    this_fn,
                    ui_respondWithUserReject
            );

            // set next step
            ctx->uiStep = UI_STEP_INIT_RESPOND;
            break;
        }

        case UI_STEP_INIT_RESPOND: {
            // switch stage to receiving user input
            ctx->stage = SIGN_STAGE_COLLECT;

            // respond to host that it's ok to send transaction for signing
            io_send_buf(SUCCESS, NULL, 0);

            // switch user interface to show that we are working on the tx
            ui_displayBusy();

            // set invalid step so we never cycle around
            ctx->uiStep = UI_STEP_INIT_INVALID;
            break;
        }

        default: {
            // we don't tolerate invalid state
            ASSERT(false);
        }
    }
}

// handleSignTxCollect implements transaction details stream APDU processing.
// It's the set of intermediate steps where we collect all the transaction details
// so we can calculate it's signature.
static void handleSignTxCollect(uint8_t p2, uint8_t *wireBuffer, size_t wireSize) {
    // validate we are on the right stage here
    ASSERT_STAGE(SIGN_STAGE_COLLECT);

    // validate the p2 value
    VALIDATE(p2 == 0, ERR_INVALID_PARAMETERS);

    // validate we received at least some data from remote host
    VALIDATE(wireSize >= 1, ERR_INVALID_DATA);

    // process the wire buffer with the tx stream
    tx_stream_status_e status = txStreamProcess(&ctx->stream, wireBuffer, wireSize, 0);
    switch (status) {
        case TX_STREAM_PROCESSING:
            // the stream is waiting for additional data
            // nothing to do here
            break;
        case TX_STREAM_FINISHED:
            // the stream finished and we expect the next stage
            ctx->stage = SIGN_STAGE_FINALIZE;
            break;
        case TX_STREAM_FAULT:
            // the stream failed because the incoming data were incorrect
            // let the case fall through down to ERR_INVALID_DATA
        default:
            // reset the context, the stream is in unknown state
            VALIDATE(false, ERR_INVALID_DATA);
    }

    // respond to the host to continue sending data
    // we send the current stage to verify the progress
    uint8_t res = ctx->stage;
    io_send_buf(SUCCESS, (uint8_t * ) & res, 1);

    // we are still busy loading the data
    ui_displayBusy();
}

// handleSignTxFinalize implements transaction signing finalization step.
// It's the last step where the host signals the transaction is ready for signature,
// the device makes checks to confirm the transaction data are valid, calculates the signature,
// asks user to validate the transaction details and responds to the host with the signature.
static void handleSignTxFinalize(uint8_t p2, uint8_t *wireBuffer MARK_UNUSED, size_t wireSize) {
    // validate we are on the right stage
    // the stream should have signaled to be done parsing the tx by now
    ASSERT_STAGE(SIGN_STAGE_FINALIZE);

    // validate the p2 value
    VALIDATE(p2 == 0, ERR_INVALID_PARAMETERS);

    // we don't expect to receive any data here
    VALIDATE(wireSize == 0, ERR_INVALID_DATA);

    // we don't expect any more data to be coming from the host
    io_state = IO_EXPECT_UI;

    // get the security policy for new transaction from a given address
    security_policy_t policy = policyForSignTxFinalize();
    ASSERT_NOT_DENIED(policy);

    // validate the value CHAIN_ID (transferred as v) of the transaction
    VALIDATE(txGetV(&ctx->tx) == 0, ERR_INVALID_DATA);

    // extract the transaction hash value from SHA3 context
    uint8_t hash[TX_HASH_LENGTH];
    cx_hash((cx_hash_t * ) & ctx->sha3Context, CX_LAST, hash, 0, hash, TX_HASH_LENGTH);

    // get the transaction signature
    txGetSignature(&ctx->signature, &ctx->path, (uint8_t *) hash, sizeof(hash));

    // mark the signature as ready
    ctx->responseReady = RESPONSE_READY_TAG;

    // decide what UI step to take first based on policy
    switch (policy) {
        case POLICY_PROMPT:
            ctx->uiStep = UI_STEP_TX_RECIPIENT;
            break;
        case POLICY_ALLOW:
            ctx->uiStep = UI_STEP_TX_RESPOND;
            break;
        default:
            // if no policy was set, terminate the action
            ASSERT(false);
    }

    // run the first step
    runSignTransactionUIStep();
}

// runSignTransactionUIStep implements next step in UX flow of the tx signing finalization flow (the last APDU).
static void runSignTransactionUIStep() {
    // make sure we are on the right stage
    ASSERT_STAGE(SIGN_STAGE_FINALIZE);

    // keep reference to self so we can use it as a callback to resume UI
    ui_callback_fn_t *this_fn = runSignTransactionUIStep;

    // resume the stage based on previous result
    switch (ctx->uiStep) {
        case UI_STEP_TX_RECIPIENT: {
            // make sure the advertised address length is well inside the address buffer size
            ASSERT(ctx->tx.recipient.length <= SIZEOF(ctx->tx.recipient.value));

            // create formatted address buffer and format for display
            char addrStr[64];
            if (ctx->tx.recipient.length > 0) {
                addressFormatStr(ctx->tx.recipient.value, &ctx->sha3Context, addrStr, sizeof(addrStr));
            } else {
                // smart contract targeted transaction
                strcpy(addrStr, "Contract");
            }

            // display the recipient address
            ui_displayPaginatedText(
                    "Recipient",
                    addrStr,
                    this_fn
            );

            // set next step
            ctx->uiStep = UI_STEP_TX_AMOUNT;
            break;
        }

        case UI_STEP_TX_AMOUNT: {
            // make sure the advertised amount length is well inside the buffer size
            ASSERT(ctx->tx.value.length <= SIZEOF(ctx->tx.value.value));

            // create formatted address buffer and format for display
            char valueStr[40];
            txGetFormattedAmount(&ctx->tx.value, WEI_TO_FTM_DECIMALS, valueStr, sizeof(valueStr));

            // display transferred amount for the transaction
            ui_displayPaginatedText(
                    "Amount (FTM)",
                    valueStr,
                    this_fn
            );

            // set next step
            ctx->uiStep = UI_STEP_TX_FEE;
            break;
        }

        case UI_STEP_TX_FEE: {
            // make sure the advertised gas amount and price length is well inside the buffer size
            ASSERT(ctx->tx.gasPrice.length <= SIZEOF(ctx->tx.gasPrice.value));
            ASSERT(ctx->tx.startGas.length <= SIZEOF(ctx->tx.startGas.value));

            // create formatted address buffer and format for display
            char valueStr[40];
            txGetFormattedFee(&ctx->tx, WEI_TO_FTM_DECIMALS, valueStr, sizeof(valueStr));

            // display max fee for the transaction
            ui_displayPaginatedText(
                    "Max fee (FTM)",
                    valueStr,
                    this_fn
            );

            // set next step
            ctx->uiStep = UI_STEP_TX_CONFIRM;
            break;
        }

        case UI_STEP_TX_CONFIRM: {
            // ask user to confirm the key export
            ui_displayPrompt(
                    "Send",
                    "transaction?",
                    this_fn,
                    ui_respondWithUserReject
            );

            // set next step
            ctx->uiStep = UI_STEP_TX_RESPOND;
            break;
        }

        case UI_STEP_TX_RESPOND: {
            // sanity check; make sure the signature is ready
            VALIDATE(ctx->responseReady == RESPONSE_READY_TAG, ERR_INVALID_DATA);

            // switch stage to mark we are done here
            ctx->stage = SIGN_STAGE_DONE;

            // respond to host that it's ok to send transaction for signing
            io_send_buf(SUCCESS, (uint8_t * ) & ctx->signature, sizeof(ctx->signature));

            // switch user to idle; we are done here
            ui_idle();

            // set invalid step so we never cycle around
            ctx->uiStep = UI_STEP_TX_INVALID;
            break;
        }

        default: {
            // we don't tolerate invalid state
            ASSERT(false);
        }
    }
}

// handleSignTransaction implements transaction signature processing proxy.
// Each sign. request goes here first and this function decides where to send it next.
void handleSignTransaction(
        uint8_t p1,
        uint8_t p2,
        uint8_t *wireBuffer,
        size_t wireSize,
        bool isOnInit
) {
    // make sure the state is clean
    if (isOnInit) {
        os_memset(ctx, 0, SIZEOF(*ctx));
    }

    // decide based on the p1 value
    switch (p1) {
        case P1_NEW_TRANSACTION:
            handleSignTxInit(p2, wireBuffer, wireSize);
            break;
        case P1_STREAM_DATA:
            handleSignTxCollect(p2, wireBuffer, wireSize);
            break;
        case P1_GET_SIGNATURE:
            handleSignTxFinalize(p2, wireBuffer, wireSize);
            break;
        default:
            VALIDATE(false, ERR_INVALID_PARAMETERS);
    }
}