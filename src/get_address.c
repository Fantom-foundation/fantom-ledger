#include "common.h"
#include "get_address.h"
#include "derive_key.h"
#include "state.h"
#include "policy.h"
#include "ui_helpers.h"
#include "address_utils.h"

// RESPONSE_READY_TAG is used to tag output buffer when address is ready.
static uint16_t RESPONSE_READY_TAG = 32123;

// ctx holds the context of the Get Address instruction.
static ins_get_address_context_t *ctx = &(instructionState.insGetAddressContext);

// what are possible scenarios of the address handling
enum {
    P1_RETURN_ADDRESS = 0x01,
    P1_DISPLAY_ADDRESS = 0x02,
};

// runGetAddressUIStep implements next step UX callback for Get Address instruction.
static void runGetAddressUIStep();

// what steps are supported for the Get Address handler.
enum {
    UI_STEP_WARNING = 100,
    UI_STEP_DISPLAY_PATH,
    UI_STEP_ADDRESS,
    UI_STEP_CONFIRM,
    UI_STEP_RESPOND,
    UI_STEP_INVALID,
};

// handleGetAddress implements handling of the
void handleGetAddress(
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

    // clear the response ready tag
    ctx->responseReady = 0;

    // validate the values p1 and p2
    // see the documentation for explanation why we check zero values here
    VALIDATE(p1 == P1_RETURN_ADDRESS || p1 == P1_DISPLAY_ADDRESS, ERR_INVALID_PARAMETERS);
    VALIDATE(p2 == 0, ERR_INVALID_PARAMETERS);

    // decide if the address display will be part of the INS flow
    ctx->isShowAddress = (p1 == P1_DISPLAY_ADDRESS);

    // parse BIP44 path from the incoming request
    size_t parsedSize = bip44_parseFromWire(&ctx->path, wireBuffer, wireSize);

    // make sure size of the data we parsed corresponds with the data we received
    if (parsedSize != wireSize) {
        THROW(ERR_INVALID_DATA);
    }

    // check security policy for the instruction we are about to run
    security_policy_t policy = policyForGetAddress(&ctx->path, ctx->isShowAddress);
    ASSERT_NOT_DENIED(policy);

    // derive the address and mark as ready
    ctx->address.size = deriveAddress(&ctx->path, &ctx->sha3Context, ctx->address.buffer, SIZEOF(ctx->address.buffer));
    ctx->responseReady = RESPONSE_READY_TAG;

    // decide what UI step to take first based on policy
    switch (policy) {
        case POLICY_WARN:
            // warn about unusual address request
            ctx->uiStep = UI_STEP_WARNING;
            break;
        case POLICY_PROMPT:
            // see runGetAddressUIStep comment below to get the right starting point
            ctx->uiStep = (ctx->isShowAddress ? UI_STEP_ADDRESS : UI_STEP_CONFIRM);
            break;
        case POLICY_ALLOW:
            ctx->uiStep = UI_STEP_RESPOND;
            break;
        default:
            // if no policy was set, terminate the action
            ASSERT(false);
    }

    // run the first step
    runGetAddressUIStep();
}

// runGetAddressUIStep implements next step UX callback for Get Address instruction.
// The UI sequence for just sending the address to host is:
//     WARN -> PATH -> CONFIRM -> RESPOND
// The UI sequence for displaying address before sending to host is:
//     WARN -> PATH -> ADDRESS -> RESPOND
static void runGetAddressUIStep() {
    // keep reference to self so we can use it as a callback to resume UI
    ui_callback_fn_t *this_fn = runGetAddressUIStep;

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
                    "Address Path",
                    pathStr,
                    this_fn
            );

            // set next step (check the comment above for the correct next step)
            ctx->uiStep = (ctx->isShowAddress ? UI_STEP_ADDRESS : UI_STEP_CONFIRM);
            break;
        }

        case UI_STEP_ADDRESS: {
            // make sure the address is well inside the available buffer
            ASSERT(ctx->address.size < SIZEOF(ctx->address.buffer));

            // create formatted address buffer and format for display
            char addrStr[64];
            addressFormatStr(ctx->address.buffer, ctx->address.size, &ctx->sha3Context, addrStr, SIZEOF(addrStr));

            // show user the address being exported
            ui_displayPaginatedText(
                    "Address",
                    addrStr,
                    this_fn
            );

            // set next step
            ctx->uiStep = UI_STEP_RESPOND;
            break;
        }

        case UI_STEP_CONFIRM: {
            // ask user to confirm the key export
            ui_displayPrompt(
                    "Confirm",
                    "Address?",
                    this_fn,
                    ui_respondWithUserReject
            );

            // set next step
            ctx->uiStep = UI_STEP_RESPOND;
            break;
        }

        case UI_STEP_RESPOND: {
            // make sure the address is ready
            ASSERT(ctx->responseReady == RESPONSE_READY_TAG);

            // make sure the address length is well inside the buffer size
            ASSERT(ctx->address.size <= SIZEOF(ctx->address.buffer));

            // send the data to remote host and switch idle
            // we don't send the whole address buffer, some space is probably unused
            // we send only the first byte (address length) + the bytes of the active address part
            io_send_buf(SUCCESS, (uint8_t * ) & ctx->address, 1 + ctx->address.size);
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