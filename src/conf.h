#ifndef FANTOM_LEDGER_CONF_H
#define FANTOM_LEDGER_CONF_H

// The app is designed for specific Ledger API level.
// In case there is an api change, verify changes before rising the API level here.
#define API_LEVEL_MIN = 9;
#define API_LEVEL_MAX = 10;

// data exchange details
#define CLA 0xE0

// security limits
#define MIN_BIP32_PATH 2
#define MAX_BIP32_PATH 10

#endif
