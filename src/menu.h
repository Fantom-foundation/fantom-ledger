#ifndef FANTOM_LEDGER_MENU_H
#define FANTOM_LEDGER_MENU_H

#include <os_io_seproxyhal.h>

// use the right menu approach based on the device we are working with
#if defined(TARGET_NANOS)
extern const ux_menu_entry_t menu_main[4];
#elif defined(TARGET_NANOX)
extern const ux_flow_step_t* const ux_idle_flow [];
#endif

#endif //FANTOM_LEDGER_MENU_H
