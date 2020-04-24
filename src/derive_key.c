/**
 * This implements private and public key derivation logic.
 *
 * Fantom is using Ether compatible account/address space and also the same
 * key calculation primitives. The key derivation uses CX_CURVE_256K1
 * to create child private key and corresponding public key.
 */
#include <os_io_seproxyhal.h>
#include <stdint.h>

#include "assert.h"
#include "errors.h"
#include "derive_key.h"
#include "utils.h"
#include "big_endian_io.h"

// derivePrivateKey implements private key derivation from internal root key.
void derivePrivateKey(
        const bip44_path_t *path,
        chain_code_t *chainCode,
        private_key_t *privateKey
) {
    // make sure the path is correct for the operation
    if (!bip44_hasValidFantomPrefix(path)) {
        THROW(ERR_INVALID_DATA);
    }

    // make sure the advertised path nodes is in pair with
    // the path container size (not reaching beyond the actual path container)
    ASSERT(path->length < ARRAY_LEN(path->path));

    // get a buffer for private key
    uint8_t privateKeyRawBuffer[RAW_PRIVATE_KEY_BUFFER];

    // make sure we can safely erase chain code container since it's the right size
    STATIC_ASSERT(SIZEOF(chainCode->code) == CHAIN_CODE_SIZE, "bad chain code length");
    os_memset(chainCode->code, 0, SIZEOF(chainCode->code));

    // do the extraction
    BEGIN_TRY
    {
        TRY
        {
            // make sure we are at least on the API we need to support this function
            STATIC_ASSERT(CX_APILEVEL >= API_LEVEL_MIN, "unsupported api level");

            // make sure the private key is of expected size
            STATIC_ASSERT(SIZEOF(privateKey->d) == RAW_PRIVATE_KEY_SIZE, "bad private key length");

            // call for private key derivation
            io_seproxyhal_io_heartbeat();
            os_perso_derive_node_bip32(
                    CX_CURVE_256K1,
                    path->path,
                    path->length,
                    privateKeyRawBuffer,
                    chainCode->code);

            // copy the private key
            cx_ecfp_init_private_key(CX_CURVE_256K1, privateKeyRawBuffer, RAW_PRIVATE_KEY_SIZE, privateKey);
            io_seproxyhal_io_heartbeat();
        }
        FINALLY
        {
            // clean up the raw private key in memory so we don't leek it in any way after this call
            os_memset(privateKeyRawBuffer, 0, SIZEOF(privateKeyRawBuffer));
        }
    }
    END_TRY;
}

// deriveRawPublicKey implements public key derivation from provided derived private key.
void deriveRawPublicKey(
        const private_key_t *privateKey,
        cx_ecfp_public_key_t *publicKey
) {
    io_seproxyhal_io_heartbeat();
    cx_ecfp_generate_pair(CX_CURVE_256K1, publicKey, (cx_ecfp_private_key_t *) privateKey, 1);
    io_seproxyhal_io_heartbeat();
}

// extractRawPublicKey implements extracting public key into an output buffer.
void extractRawPublicKey(
        const cx_ecfp_public_key_t *publicKey,
        uint8_t *outBuffer, size_t outSize
) {
    // make sure the public key size is what we expect here
    STATIC_ASSERT(SIZEOF(publicKey->W) == 65, "bad public key length");

    // make sure the output buffer size corresponds with expected key size
    ASSERT(outSize == PUBLIC_KEY_SIZE);

    // copy little endian to big endian, the last byte is terminator
    uint8_t i;
    for (i = 0; i < PUBLIC_KEY_SIZE; i++) {
        outBuffer[i] = publicKey->W[64 - i];
    }

    // any key data beyond 32nd byte?
    if ((publicKey->W[32] & 1) != 0) {
        outBuffer[31] |= 0x80;
    }
}

// deriveExtendedPublicKey implements public key
// with chain code derivation for the BIP44 path specified.
void deriveExtendedPublicKey(
        const bip44_path_t *path,
        extended_public_key_t *out
) {
    private_key_t privateKey;
    chain_code_t chainCode;

    // make sure the output structure is of the right dimension
    // the 1st byte is for public key length, others are for the public key and chain code
    STATIC_ASSERT(SIZEOF(*out) == 1 + PUBLIC_KEY_SIZE + CHAIN_CODE_SIZE, "bad ext pub key size");

    BEGIN_TRY
    {
        TRY
        {
            // get the private code for the path
            derivePrivateKey(
                    path,
                    &chainCode,
                    &privateKey
            );

            // prep container for public key
            cx_ecfp_public_key_t publicKey;

            // derive the public key from the private one
            deriveRawPublicKey(&privateKey, &publicKey);

            // make sure the public key size corresponds with our expectation
            STATIC_ASSERT(SIZEOF(out->publicKey) == PUBLIC_KEY_SIZE, "bad pub key size");

            // extract the public key data to the output buffer
            extractRawPublicKey(&publicKey, out->publicKey, SIZEOF(out->publicKey));
            out->length = PUBLIC_KEY_SIZE;

            // make sure the chain code container size is what we expect
            STATIC_ASSERT(CHAIN_CODE_SIZE == SIZEOF(out->chainCode), "bad chain code size");

            // make sure the chain code source data is of the expected size
            STATIC_ASSERT(CHAIN_CODE_SIZE == SIZEOF(chainCode.code), "bad chain code size");

            // chain code is placed after the public key
            os_memmove(out->chainCode, chainCode.code, CHAIN_CODE_SIZE);
        }
        FINALLY
        {
            // clear the private key storage so we don't leak it after this call
            os_memset(&privateKey, 0, SIZEOF(privateKey));
        }
    }
    END_TRY;
}