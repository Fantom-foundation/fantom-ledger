#ifndef FANTOM_LEDGER_DERIVE_KEY_H
#define FANTOM_LEDGER_DERIVE_KEY_H

#include "common.h"
#include "handlers.h"
#include "bip44.h"

static const size_t RAW_PRIVATE_KEY_SIZE =  32;
static const size_t PUBLIC_KEY_SIZE =  32;
static const size_t CHAIN_CODE_SIZE =  32;

// private_key_t declares the private key type
typedef cx_ecfp_private_key_t private_key_t;

// chain_code_t declares chain code buffer type
typedef struct {
    uint8_t code[CHAIN_CODE_SIZE];
} chain_code_t;

// extended_public_key_t declares public key buffer with chain code type
typedef struct {
    uint8_t publicKey[PUBLIC_KEY_SIZE];
    uint8_t chainCode[CHAIN_CODE_SIZE];
} extended_public_key_t;

// derivePrivateKey implements private key derivation from internal root key.
void derivePrivateKey(
        const bip44_path_t* path,
        chain_code_t* chainCode, // 32 byte output
        private_key_t* privateKey // output
);

// deriveRawPublicKey implements public key derivation from provided derived private key.
void deriveRawPublicKey(
        const privateKey_t* privateKey,
        cx_ecfp_public_key_t* publicKey // output
);

// extractRawPublicKey implements extracting public key into an output buffer.
void extractRawPublicKey(
        const cx_ecfp_public_key_t* publicKey,
        uint8_t* outBuffer, size_t outSize
);

// deriveExtendedPublicKey implements public key
// with chain code derivation for the BIP44 path specified.
void deriveExtendedPublicKey(
        const bip44_path_t* path,
        extended_public_key_t* out
);

#endif //FANTOM_LEDGER_DERIVE_KEY_H
