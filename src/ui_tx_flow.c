#include <os_io_seproxyhal.h>
#include "ui_helpers.h"
#include "glyphs.h"

// ITEMS macro is used for pure formatting purpose.
#define ITEMS(...) { __VA_ARGS__ }

// ui_confirmTransaction implements UX callback for transaction confirmation action.
void ui_confirmTransaction() {
    TRY_CATCH_UI({
        // verify we are on the right state before firing the callback
        ui_assertTxDetailsGuard();

        // fire the corresponding call to process
        // user's acceptance of the course of action
        ui_tx_details_state_t* ctx = txDetailsState;
        ui_callbackConfirm(&ctx->callback);
    });
}

// ui_rejectTransaction implements UX callback for transaction rejection action.
void ui_rejectTransaction() {
    TRY_CATCH_UI({
        // verify we are on the right state before firing the callback
        ui_assertTxDetailsGuard();

        // fire the corresponding call to process
        // user's rejection of the course of action
        ui_tx_details_state_t* ctx = txDetailsState;
        ui_callbackReject(&ctx->callback);
    });
}

// UX_STEP_NOCB defines step 1: Inform user that there is a transaction
// to be validated and confirmed.
// The layout contains icon and two lines of normal text (pnn layout).
UX_STEP_NOCB(
    ux_tx_details_step_1,
    pnn,
    ITEMS (
        &C_icon_eye,
        "Review",
        "transaction"
    )
);

// UX_STEP_NOCB defines step 2: Display sender address
UX_STEP_NOCB(
    ux_tx_details_step_2,
    paging,
    ITEMS(
        "From",
        (char *) &displayState.txDetailsState.from
    )
);

// UX_STEP_NOCB defines step 3: Display total amount
UX_STEP_NOCB(
    ux_tx_details_step_3,
    paging,
    ITEMS(
        "Amount",
        (char *) &displayState.paginatedText.amount
    )
);

// UX_STEP_NOCB defines step 4: Display recipient address
UX_STEP_NOCB(
    ux_tx_details_step_4,
    paging,
    ITEMS(
        "Recipient",
        (char *) &displayState.paginatedText.to
    )
);

// UX_STEP_NOCB defines step 5: Display max fee
UX_STEP_NOCB(
    ux_tx_details_step_5,
    paging,
    ITEMS(
        "Max Fees",
        (char *) &displayState.paginatedText.fee
    )
);

// UX_STEP_CB defines step 6: Display confirmation dialog.
UX_STEP_CB(
    ux_tx_details_step_6,
    pbb,
    ui_confirmTransaction(),
    ITEMS(
        &C_icon_validate_14,
        "Send",
        "transaction",
    )
);

// UX_STEP_CB defines step 7: Display rejection dialog.
UX_STEP_CB(
    ux_tx_details_step_7,
    pbb,
    ui_rejectTransaction(),
    ITEMS(
        &C_icon_crossmark,
        "Reject",
        "transaction",
    )
);

// UX_FLOW defines flow for long and paginated text.
UX_FLOW(
    ux_tx_details_flow,
    &ux_tx_details_step_1,
    &ux_tx_details_step_2,
    &ux_tx_details_step_3,
    &ux_tx_details_step_4,
    &ux_tx_details_step_5,
    &ux_tx_details_step_6,
    &ux_tx_details_step_7,
    FLOW_END_STEP
);

// ui_doDisplayTxDetails implements actual change in UX flow to show the configured tx details.
void ui_doDisplayTxDetails() {
    // start the tx details flow
    ux_flow_init(0, ux_tx_details_flow, NULL);
}
