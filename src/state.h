#ifndef FANTOM_LEDGER_STATE_H
#define FANTOM_LEDGER_STATE_H

#include "get_pub_key.h"

// Declares what instructions are recognized and processed by the application.
#define INS_NONE = -1;
#define INS_VERSION = 0x01;
#define INS_GET_KEY = 0x10;
#define INS_GET_ADDR = 0x11;
#define INS_SIGN_TX = 0x20;

// instruction_state_t defines unified APDU instruction state.
// We use joined instruction state storage since only one instruction
// can rut at any time.
typedef union {
    ins_get_ext_pubkey_context_t insGetPubKeyContext;
} instruction_state_t;

// currentIns declares a current instruction registry.
// Instructions received from APDU are uint8_t, but we have a special INS_NONE value
// to identify idle/waiting state.
extern int currentIns;

#endif
