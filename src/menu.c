#include <os_io_seproxyhal.h>
#include "menu.h"
#include "get_version.h"
#include "glyphs.h"

// ITEMS macro is used for pure formatting purpose.
#define ITEMS(...) { __VA_ARGS__ }

// UX_STEP_NOCB is a macro for simple flow step, given its name, layout and content.
// ux_idle_main is the main idle screen.
// The layout contains Fantom logo and two lines of normal text (pnn layout).
// See SDK /lib_ux/include/ux_flow_engine for details about macros,
// see /lib_ux/include/ux_layouts.h for layouts.

UX_STEP_NOCB(
        ux_idle_main,
        pnn,
        ITEMS (
            &C_fantom_logo,
            "Fantom Nano",
            "Ready ..."
        )
);

// ux_idle_version is the version information screen.
// Layout contains two lines, normal text and bold text (bn layout).
UX_STEP_NOCB(
        ux_idle_version,
        bn,
        ITEMS(
            "Version",
            APPVERSION,
        )
);

// UX_STEP_CB is a macro for a simple flow step with a validation callback.
// ux_idle_quit is the app termination screen.
// Layout contains an icon and single line of bold text (pb layout).
UX_STEP_CB(
        ux_idle_quit,
        pb,
        os_sched_exit(-1),
        ITEMS(
            &C_icon_dashboard_x,
            "Quit"
        )
);

// ux_idle_flow defines the idle menu flow, uses steps defined above.
UX_FLOW(
        ux_idle_flow,
        &ux_idle_main,
        &ux_idle_version,
        &ux_idle_quit,
        FLOW_LOOP
);
