#include <os_io_seproxyhal.h>
#include <ux.h>
#include "ui_helpers.h"
#include "glyphs.h"

// ITEMS macro is used for pure formatting purpose.
#define ITEMS(...) { __VA_ARGS__ }

// ---------------------------------------------
// Here starts the UX flow for paginated text.
// ---------------------------------------------

// ui_confirmPaginatedText implements UX callback for paginated text confirmation action.
void ui_confirmPaginatedText() {
    TRY_CATCH_UI({
        // make sure we are on the right state before firing a callback
        ui_assertPaginatedTextGuard();

        // fire the callback to confirm end user finished reading the paginated text
        ui_paginated_text_state_t *ctx = paginatedTextState;
        ui_callbackConfirm(&ctx->callback);
    });
}

// UX_STEP_CB is a macro for a simple flow step with a validation callback.
// Here we initialize paginated layout with confirmation callback.
UX_STEP_CB(
    ux_paginated_text_flow_paginated,
    bnnn_paging,
    ui_confirmPaginatedText(),
    ITEMS(
        (char *)&displayState.paginatedText.header,
        (char *)&displayState.paginatedText.text
    )
);

// UX_STEP_CB is a macro for a simple flow step with a validation callback.
// Here we initialize simple layout (pnn layout means icon + two lines of normal text)
// with confirmation callback.
UX_STEP_CB(
    ux_paginated_text_flow_short,
    pnn,
    ui_confirmPaginatedText(),
    ITEMS(
        &C_icon_eye,
        (char *)&displayState.paginatedText.header,
        (char *)&displayState.paginatedText.text
    )
);

// UX_FLOW defines flow for long and paginated text.
UX_FLOW(
    ux_paginated_text_flow,
    &ux_paginated_text_flow_paginated
);

// UX_FLOW defines flow for show text with icon.
UX_FLOW(
    ux_short_text_flow,
    &ux_paginated_text_flow_short
);

// ui_doDisplayPaginatedText implements actual change in UX flow to show the configured paginated text.
void ui_doDisplayPaginatedText() {
    #ifdef FUZZING
    ux_flow_init(0, ux_short_text_flow, NULL);
    ux_stack_push();
    #else
    // decide by the text length if we can use simple flow, or if we need the paginated one
    if (strlen((const char*) &displayState.paginatedText.text) < MAX_SIMPLE_TEXT_LENGTH ) {
        // initialize the simple flow defined above
        ux_flow_init(0, ux_short_text_flow, NULL);
    } else {
        // reset the pagination and display paginated flow defined above
        ux_layout_bnnn_paging_reset();
        ux_flow_init(0, ux_paginated_text_flow, NULL);
    }
    #endif
}

// ---------------------------------------------
// Here starts the UX flow for prompt question.
// ---------------------------------------------

// ui_confirmPrompt implements UX callback for prompt confirmation action.
void ui_confirmPrompt()
{
    TRY_CATCH_UI({
        // verify we are on the right state before firing the callback
        ui_assertPromptGuard();

        // fire the corresponding call to process
        // user's acceptance of the course of action
        ui_prompt_state_t* ctx = promptState;
        ui_callbackConfirm(&ctx->callback);
    });
}

// ui_rejectPrompt implements UX callback for prompt rejection action.
void ui_rejectPrompt()
{
    TRY_CATCH_UI({
        // verify we are on the right state before firing the callback
        ui_assertPromptGuard();

        // fire the corresponding call to process
        // user's rejection of the course of action
        ui_prompt_state_t* ctx = promptState;
        ui_callbackReject(&ctx->callback);
    });
}

// UX_STEP_CB is a macro for a simple flow step with a validation callback.
// Here we initialize simple layout (pbb layout means icon + two lines of bold text)
// with confirmation callback.
UX_STEP_CB(
    ux_display_prompt_confirm_step,
    pbb,
    ui_confirmPrompt(),
    ITEMS(
        &C_icon_validate_14,
        (char *)&displayState.prompt.header,
        (char *)&displayState.prompt.text,
    )
);

// UX_STEP_CB is a macro for a simple flow step with a validation callback.
// Here we initialize simple layout (pb layout means icon + one line of bold text)
// with rejection callback.
UX_STEP_CB(
    ux_display_prompt_reject_step,
    pb,
    ui_rejectPrompt(),
    ITEMS(
        &C_icon_crossmark,
        "Reject"
    )
);

// UX_FLOW defines flow for a question with confirmation prompt.
UX_FLOW(
    ux_prompt_flow,
    &ux_display_prompt_confirm_step,
    &ux_display_prompt_reject_step
);

// ui_doDisplayPrompt implements actual change in UX flow to show the configured prompt.
void ui_doDisplayPrompt() {
    // start the prompt flow
    ux_flow_init(0, ux_prompt_flow, NULL);
}

// ---------------------------------------------
// Here starts the UX flow for busy screen.
// ---------------------------------------------

// UX_STEP_NOCB is a macro for simple flow step without any additional callbacks or params.
// Here we initialize simple layout (pn layout means icon + one line of normal text).
UX_STEP_NOCB(
    ux_display_busy_step,
    pn,
    ITEMS(
        &C_icon_loader,
        "Please wait ..."
    )
);

// UX_FLOW defines flow for a simple busy screen with no user interaction.
UX_FLOW(
    ux_busy_flow,
    &ux_display_busy_step
);

// ui_doDisplayBusy implements actual change in UX flow to show the busy screen.
void ui_doDisplayBusy() {
    // start the busy flow
    ux_flow_init(0, ux_busy_flow, NULL);
}
