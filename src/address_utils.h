#ifndef FANTOM_LEDGER_ADDRESS_UTILS_H
#define FANTOM_LEDGER_ADDRESS_UTILS_H

#include <stdint.h>
#include "cx.h"

// getAddressStr implements formatting wallet address for given public key.
void getAddressStr(cx_ecfp_public_key_t *publicKey, uint8_t *out, cx_sha3_t *sha3Context);

#endif //FANTOM_LEDGER_ADDRESS_UTILS_H
