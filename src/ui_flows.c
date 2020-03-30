#include <os_io_seproxyhal.h>
#include "ui_helpers.h"
#include "glyphs.h"

// ITEMS macro is used for pure formatting purpose.
#define ITEMS(...) { __VA_ARGS__ }

// ---------------------------------------------
// Here starts the UX flow for paginated text.
// ---------------------------------------------

// ui_confirmPaginatedText implements callback for paginated text confirmation.
void ui_confirmPaginatedText() {
    TRY_CATCH_UI({
         ui_assertPaginatedTextGuard();
         paginated_text_state_t *ctx = paginatedTextState;
         ui_callbackConfirm(&ctx->callback);
    });
}

// UX_STEP_CB is a macro for a simple flow step with a validation callback.
// Here we initialize paginated layout with confirmation callback.
UX_STEP_CB(
    ux_paginated_text_flow_paginated,
    paging,
    ui_confirmPaginatedText(),
    ITEMS(
        (char *)&displayState.paginatedText.header,
        (char *)&displayState.paginatedText.fullText
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
        (char *)&displayState.paginatedText.fullText
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
    // decide by the text length if we can use simple flow, or if we need the paginated one
    if (strlen((const char*) &displayState.paginatedText.fullText) < MAX_SIMPLE_TEXT_LENGTH ) {
        // initialize the simple flow defined above
        ux_flow_init(0, ux_short_text_flow, NULL);
    } else {
        // reset the pagination and display paginated flow defined above
        ux_layout_paging_reset();
        ux_flow_init(0, ux_paginated_text_flow, NULL);
    }
}
