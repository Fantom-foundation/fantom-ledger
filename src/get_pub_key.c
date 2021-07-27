/**
 * Implements Get Public Key APDU instructions handler and corresponding helpers.
 */
#include "common.h"
#include "errors.h"
#include "big_endian_io.h"
#include "state.h"
#include "ui_helpers.h"
#include "policy.h"
#include "get_pub_key.h"

// ctx hold the direct reference to this instruction context.
static ins_get_ext_pubkey_context_t *ctx = &(instructionState.insGetPubKeyContext);

// RESPONSE_READY_TAG is used to tag the state context that the key is ready.
static int16_t RESPONSE_READY_TAG = 7455;

// runGetPublicKeyUIStep implements next step of UX for the Get Public Key instruction.
static void runGetPublicKeyUIStep();

// what steps are being handled
enum {
    UI_STEP_WARNING = 100,
    UI_STEP_DISPLAY_PATH,
    UI_STEP_CONFIRM,
    UI_STEP_RESPOND,
    UI_STEP_INVALID,
};

// handleGetPublicKey implements APDU instruction handler for Get Public Key instruction.
void handleGetPublicKey(
        uint8_t p1,
        uint8_t p2,
        uint8_t *wireBuffer,
        size_t wireSize,
        bool isOnInit
) {
    // make sure the state is clean
    if (isOnInit) {
        memset(ctx, 0, SIZEOF(*ctx));
    }

    // extra reset the response mark
    ctx->responseReady = 0;

    // validate the values p1 and p2
    // see the documentation for explanation why we check zero values here
    VALIDATE(p1 == 0, ERR_INVALID_PARAMETERS);
    VALIDATE(p2 == 0, ERR_INVALID_PARAMETERS);

    // parse BIP44 path from the wire buffer so we can derive keys for it
    size_t parsedSize = bip44_parseFromWire(&ctx->path, wireBuffer, wireSize);

    // make sure size of the data we parsed corresponds with the data we received
    if (parsedSize != wireSize) {
        THROW(ERR_INVALID_DATA);
    }

    // check security policy for the instruction we are about to run
    security_policy_t policy = policyForGetPublicKey(&ctx->path);
    ASSERT_NOT_DENIED(policy);

    // actually derive the public key
    deriveExtendedPublicKey(
            &ctx->path,
            &ctx->pubKey
    );

    // mark the response to be ready to deliver
    ctx->responseReady = RESPONSE_READY_TAG;

    // where on the UI scenario we start depends on the policy
    switch (policy) {
        case POLICY_WARN:
            ctx->uiStep = UI_STEP_WARNING;
            break;
        case POLICY_PROMPT:
            ctx->uiStep = UI_STEP_DISPLAY_PATH;
            break;
        case POLICY_ALLOW:
            ctx->uiStep = UI_STEP_RESPOND;
            break;
        default:
            // if no policy was set, terminate the action
            ASSERT(false);
    }

    // run the first step
    runGetPublicKeyUIStep();
}

// runGetPublicKeyUIStep implements next step of UX for the Get Public Key instruction.
static void runGetPublicKeyUIStep() {
    // keep the callback to myself
    ui_callback_fn_t *this_fn = runGetPublicKeyUIStep;

    // resume the stage based on previous result
    switch (ctx->uiStep) {
        case UI_STEP_WARNING: {
            // display the warning
            ui_displayPaginatedText(
                    "Unusual Request",
                    "Be careful!",
                    this_fn
            );

            // set next step
            ctx->uiStep = UI_STEP_DISPLAY_PATH;
            break;
        }

        case UI_STEP_DISPLAY_PATH: {
            // prep container for BIP44 path and format it
            char pathStr[100];
            bip44_pathToStr(&ctx->path, pathStr, SIZEOF(pathStr));

            // display BIP44 path
            ui_displayPaginatedText(
                    "Exporting Key",
                    pathStr,
                    this_fn
            );

            // set next step
            ctx->uiStep = UI_STEP_CONFIRM;
            break;
        }

        case UI_STEP_CONFIRM: {
            // ask user to confirm the key export
            ui_displayPrompt(
                    "Confirm",
                    "Key Export?",
                    this_fn,
                    ui_respondWithUserReject
            );

            // set next step
            ctx->uiStep = UI_STEP_RESPOND;
            break;
        }

        case UI_STEP_RESPOND: {
            // make sure the public key is ready
            ASSERT(ctx->responseReady == RESPONSE_READY_TAG);

            // send the data to remote host and switch idle
            io_send_buf(SUCCESS, (uint8_t * ) & ctx->pubKey, SIZEOF(ctx->pubKey));
            ui_idle();

            // set invalid step so we never cycle around
            ctx->uiStep = UI_STEP_INVALID;
            break;
        }

        default: {
            // we don't tolerate invalid state
            ASSERT(false);
        }
    }
}
