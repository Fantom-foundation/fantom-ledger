#ifndef FANTOM_LEDGER_STATE_H
#define FANTOM_LEDGER_STATE_H

// Declares what instructions are recognized and processed by the application.
#define INS_NONE = -1;
#define INS_VERSION = 0x01;
#define INS_GET_KEY = 0x10;
#define INS_GET_ADDR = 0x11;
#define INS_SIGN_TX = 0x20;

// currentIns declares a current instruction registry.
// Instructions received from APDU are uint8_t, but we have a special INS_NONE value
// to identify idle/waiting state.
extern int currentIns;

#endif
