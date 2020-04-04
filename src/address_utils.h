#ifndef FANTOM_LEDGER_ADDRESS_UTILS_H
#define FANTOM_LEDGER_ADDRESS_UTILS_H

#include <stdint.h>
#include "cx.h"

// deriveAddress implements address derivation for given BIP44 path.
size_t deriveAddress(bip44_path_t *path, cx_sha3_t *sha3Context, uint8_t *out, size_t outputSize);

// getRawAddress implements wallet address calculation for given public key.
void getRawAddress(cx_ecfp_public_key_t *publicKey, cx_sha3_t *sha3Context, uint8_t *out, size_t outputSize);

#endif //FANTOM_LEDGER_ADDRESS_UTILS_H
