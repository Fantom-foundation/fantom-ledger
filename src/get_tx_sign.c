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

// ctx keeps local reference to the transaction signature building context
static ins_sign_tx_context_t *ctx = &(instructionState.insSignTxContext);

// ASSERT_STAGE implements stage validation so the host can not step out off the protocol.
static inline void ASSERT_STAGE(sign_tx_stage_t expected) {
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
    UI_STEP_TX_DISPLAY = 100,
    UI_STEP_TX_RESPOND,
    UI_STEP_TX_INVALID,
};

// handleSignTxInit implements TX signature building initialization APDU message.
// It's the first step in signing the transaction where the source address is calculated
// and the whole process is confirmed.
static void handleSignTxInit(uint8_t p2, uint8_t *wireBuffer, size_t wireSize) {
    // make sure we are on the right stage
    ASSERT_STAGE(SIGN_STAGE_NONE);

    // validate the p2 value
    VALIDATE(p2 == 0, ERR_INVALID_PARAMETERS);

    // parse BIP44 path from the incoming request
    size_t parsedSize = bip44_parseFromWire(&ctx->fromPath, wireBuffer, wireSize);

    // make sure size of the data we parsed corresponds with the data we received
    if (parsedSize != wireSize) {
        THROW(ERR_INVALID_DATA);
    }

    // get the security policy for new transaction from a given address
    security_policy_t policy = policyForSignTxInit(&ctx->fromPath);
    ASSERT_NOT_DENIED(policy);

    // decide what UI step to take first based on policy
    switch (policy) {
        case POLICY_WARN:
            // warn about unusual address request
            ctx->uiStep = UI_STEP_INIT_WARNING;
            break;
        case POLICY_PROMPT:
            // see runGetAddressUIStep comment below to get the right starting point
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
            ctx->stage = SIGN_STAGE_INPUTS;

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

// handleSignTxCollect implements transaction details stream APDU message.
// It's the set of intermediate steps where we collect all the transaction details
// so we can calculate it's signature.
static void handleSignTxCollect(uint8_t p2, uint8_t *wireBuffer, size_t wireSize) {
    // validate we are on the right stage here
    CHECK_STAGE(SIGN_STAGE_COLLECT);

    // validate the p2 value
    VALIDATE(p2 == 0, ERR_INVALID_PARAMETERS);

    // validate we received at least some data from remote host
    VALIDATE(wireSize >= 1, ERR_INVALID_DATA);

    // process the wire buffer with the tx stream
    tx_stream_status_e status = txStreamProcess(ctx->stream, wireBuffer, wireSize, 0);
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
            // reset the context, the stream failed
            // because the incoming data were incorrect
        default:
            // reset the context, the stream is in unknown state
            VALIDATE(false, ERR_INVALID_DATA);
    }

    // respond to the host to continue sending data
    io_send_buf(SUCCESS, NULL, 0);

    // we are still busy loading the data
    ui_displayBusy();
}

// handleSignTxFinalize implements transaction signing finalization step.
// It's the last step where the host signals the transaction is ready for signature,
// the device makes checks to confirm the transaction data are valid, calculates the signature,
// asks user to validate the transaction details and responds to the host with the signature.
static void handleSignTxFinalize(uint8_t p2) {
    // validate we are on the right stage here
    CHECK_STAGE(SIGN_STAGE_FINALIZE);

    // validate the p2 value
    VALIDATE(p2 == 0, ERR_INVALID_PARAMETERS);
}