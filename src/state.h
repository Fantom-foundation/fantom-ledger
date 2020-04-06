#ifndef FANTOM_LEDGER_STATE_H
#define FANTOM_LEDGER_STATE_H

#include "get_pub_key.h"
#include "get_address.h"
#include "get_tx_sign.h"

// Declares what instructions are recognized and processed by the application.
#define INS_NONE -1
#define INS_VERSION 0x01
#define INS_GET_KEY 0x10
#define INS_GET_ADDR 0x11
#define INS_SIGN_TX 0x20

// instruction_state_t defines unified APDU instruction state.
// We use joined instruction state storage since only one instruction
// can rut at any time.
typedef union {
    ins_get_ext_pubkey_context_t insGetPubKeyContext;
    ins_get_address_context_t insGetAddressContext;
    ins_sign_tx_context_t insSignTxContext;
} instruction_state_t;

// currentIns declares a current instruction registry.
// Instructions received from APDU are uint8_t, but we have a special INS_NONE value
// to identify idle/waiting state.
extern int currentIns;

// instructionState declares a current instruction state registry.
// Only one APDU instruction is being handled at any time and a new one
// can not be started before the previous one is either finished or terminated.
// For that reason we use joined structure for all the states in one place.
extern instruction_state_t instructionState;

#endif