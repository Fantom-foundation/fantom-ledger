#include <os_io_seproxyhal.h>
#include "ui_helpers.h"
#include "ux.h"
#include "assert.h"
#include "io.h"
#include "utils.h"
#include "address_utils.h"

// displayState defines the common display state container shared between paginated text and prompt states.
// We use this trick since only one of the two may happen at any time.
ui_display_state_t displayState;

// G_ux is a magic global variable implicitly referenced by the UX_ macros.
// Apps should never need to reference it directly.
ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

// make sure the SDK didn't switch to multi-byte character sets.
STATIC_ASSERT(SIZEOF(uint8_t) == SIZEOF(char), "bad char size");

// assert_uiPaginatedTextGuard implements verification of the shared state
// so we know the state is set for paginated text.
void ui_assertPaginatedTextGuard() {
    ASSERT(paginatedTextState->guard == UI_STATE_GUARD_PAGINATED_TEXT);
}

// assert_uiPromptGuard implements verification of the shared state
// so we know the state is set for prompt.
void ui_assertPromptGuard() {
    ASSERT(promptState->guard == UI_STATE_GUARD_PROMPT);
}

// ui_assertTxDetailsGuard implements verification of the shared state
// so we know the state is set for transaction details.
void ui_assertTxDetailsGuard() {
    ASSERT(promptState->guard == UI_STATE_GUARD_TX_DETAIL);
}

// uiCallbackConfirm implements action callback for confirmed prompt.
void ui_callbackConfirm(ui_callback_t *cb) {
    // do we have the callback for confirmation?
    if (!cb->confirm) {
        return;
    }

    // make sure we are dealing with the right state
    switch (cb->state) {
        case CALLBACK_NOT_RUN:
            // update the state before resolving in case it throws
            cb->state = CALLBACK_RUN;
            cb->confirm();
            break;
        case CALLBACK_RUN:
            // ignore since the callback already fired
            break;
        default:
            // We should not get here, but we did.
            // Overflow somewhere in the code damaged the structure?
            ASSERT(false);
    }
}

// uiCallbackReject implements action callback for rejected prompt.
void ui_callbackReject(ui_callback_t *cb) {
    // do we have the callback for rejections?
    if (!cb->reject) {
        return;
    }

    // make sure we are dealing with the right state
    switch (cb->state) {
        case CALLBACK_NOT_RUN:
            // update the state before resolving in case it throws
            cb->state = CALLBACK_RUN;
            cb->reject();
            break;
        case CALLBACK_RUN:
            // ignore since the callback already fired
            break;
        default:
            // We should not get here, but we did.
            // Overflow somewhere in the code damaged the structure?
            ASSERT(false);
    }
}

// ui_CallbackInit implements callback structure initialization.
static void ui_CallbackInit(ui_callback_t *cb, ui_callback_fn_t *confirm, ui_callback_fn_t *reject) {
    cb->state = CALLBACK_NOT_RUN;
    cb->confirm = confirm;
    cb->reject = reject;
}

// ui_displayPrompt displays a prompt asking and user to decide the course of action.
// The user can confirm, or reject the action and corresponding callback is fired
// to pass the decision.
void ui_displayPrompt(
        const char *headerStr,
        const char *bodyStr,
        ui_callback_fn_t *confirm,
        ui_callback_fn_t *reject
) {
    // get the text sizes so we can validate it will fit inside our reserved space
    size_t header_len = strlen(headerStr);
    size_t text_len = strlen(bodyStr);

    // prevent overflow; we need one extra byte for string terminator
    ASSERT(header_len < SIZEOF(promptState->header));
    ASSERT(text_len < SIZEOF(promptState->text));

    // clear all memory; use safe macro from utils.h
    MEMCLEAR(&displayState, displayState);
    ui_prompt_state_t *ctx = promptState;

    // copy strings from source to the state structure (including string terminator)
    memcpy(ctx->header, headerStr, header_len + 1);
    memcpy(ctx->text, bodyStr, text_len + 1);

    // initialize the callback structure
    ui_CallbackInit(&ctx->callback, confirm, reject);

    // set the guard to mark the shared state as being used by the prompt now
    ctx->guard = UI_STATE_GUARD_PROMPT;

    // validate the i/o state we are in and set it to waiting for user interaction
    ASSERT(io_state == IO_EXPECT_NONE || io_state == IO_EXPECT_UI);
    #ifndef FUZZING
    io_state = IO_EXPECT_UI;
    #endif
    
    // change the UX flow to the configured prompt screen
    ui_doDisplayPrompt();
}

// ui_displayPaginatedText displays paginated text and waits for basic user
// interaction; we don't need user to decide a course of action
// so there is just one callback signaling that user did finish reading the text.
void ui_displayPaginatedText(
        const char *headerStr,
        const char *bodyStr,
        ui_callback_fn_t *callback
) {
    // get the text size so we can validate the text fits inside reserved space
    size_t header_len = strlen(headerStr);
    size_t body_len = strlen(bodyStr);

    // prevent overflow; extra space is for string terminator
    ASSERT(header_len < SIZEOF(paginatedTextState->header));
    ASSERT(body_len < SIZEOF(paginatedTextState->text));

    // clear the state memory; use safe macro from utils.h
    MEMCLEAR(&displayState, displayState);
    ui_paginated_text_state_t *ctx = paginatedTextState;

    // copy strings from source to the state structure (including string terminator)
    memcpy(ctx->header, headerStr, header_len);
    memcpy(ctx->text, bodyStr, body_len);

    // initialize callback; we don't need rejection callback
    // since user is not deciding anything here
    ui_CallbackInit(&ctx->callback, callback, NULL);

    // set guard to mark the shared structure as being used by paginated text
    ctx->guard = UI_STATE_GUARD_PAGINATED_TEXT;

    // validate the i/o state we are in and set it to waiting for user interaction
    ASSERT(io_state == IO_EXPECT_NONE || io_state == IO_EXPECT_UI);
    #ifndef FUZZING
    io_state = IO_EXPECT_UI;
    #endif

    // change the UX flow to configured paginated text
    ui_doDisplayPaginatedText();
}

// ui_displayBusy displays busy screen notifying end user that the device
// is in the middle of processing stuff.
void ui_displayBusy() {
    // clear all memory; use safe macro from utils.h
    MEMCLEAR(&displayState, displayState);

    // validate the i/o state we are in and set it to waiting for user interaction
    ASSERT(io_state == IO_EXPECT_NONE || io_state == IO_EXPECT_IO);

    // change the UX flow to the configured busy screen
    ui_doDisplayBusy();
}

// ui_respondWithUserReject implements sending rejection response
// to host and resetting current instruction from being processed
// any further.
void ui_respondWithUserReject() {
    // send the rejection
    io_send_buf(ERR_REJECTED_BY_USER, NULL, 0);

    // switch UX to idle
    ui_idle();
}
