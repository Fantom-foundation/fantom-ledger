#ifndef FANTOM_LEDGER_GET_VERSION_H
#define FANTOM_LEDGER_GET_VERSION_H

#include "handlers.h"
#include "common.h"

// Validate that we do have a version number specified in the Makefile.
#ifndef APPVERSION
#error "Missing -DAPPVERSION=x.y.z in Makefile"
#endif

// handleGetVersion implements handler for Get Version APDU instruction.
handler_fn_t handleGetVersion;

#endif //FANTOM_LEDGER_GET_VERSION_H
