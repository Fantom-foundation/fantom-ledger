#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "os.h"
#include "cx.h"
#include "utils.h"
#include "derive_key.h"
#include "address_utils.h"

// HEXDIGITS defines textual glyphs usable for address generation
static const uint8_t const HEXDIGITS[] = "0123456789abcdef";

// MASK defines masks for digits conversion for addresses with included checksums
static const uint8_t const MASK[] = {0x80, 0x40, 0x20, 0x10,
                                     0x08, 0x04, 0x02, 0x01};

// deriveAddress implements address derivation for given BIP44 path.
size_t deriveAddress(bip44_path_t *path, cx_sha3_t *sha3Context, uint8_t *out, size_t outputSize) {
    // make sure there is enough space in output buffer for the address
    STATIC_ASSERT(SIZEOF(*out) >= 20, "bad output address size");

    // make sanity check, the buffer may never exceed this number
    ASSERT(outputSize < MAX_BUFFER_SIZE);

    // prep containers for private key
    private_key_t privateKey;
    chain_code_t chainCode;
    size_t addressSize = 0;

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

            // get raw address for the public key
            addressSize = getRawAddress(&publicKey, sha3Context, out, outputSize);
        }
        FINALLY
        {
            // clear the private key storage so we don't leak it after this call
            os_memset(&privateKey, 0, SIZEOF(privateKey));
        }
    }
    END_TRY;

    return addressSize;
}

// getRawAddress implements wallet address calculation for given public key.
void getRawAddress(cx_ecfp_public_key_t *publicKey, cx_sha3_t *sha3Context, uint8_t *out, size_t outputSize) {
    // make sure there is enough space in output buffer for the raw address
    STATIC_ASSERT(SIZEOF(*out) >= 20, "bad raw address size");

    // make sanity check, the buffer may never exceed this number
    ASSERT(outputSize < MAX_BUFFER_SIZE);

    // make a buffer
    uint8_t hashAddress[32];

    // init context for SHA3 calculation
    cx_keccak_init(sha3Context, 256);

    // get the public key hash
    cx_hash((cx_hash_t *) sha3Context, CX_LAST, publicKey->W + 1, 64, hashAddress, 32);

    // move the last 20 bytes of the hash to output buffer
    os_memmove(out, hashAddress + 12, 20);
}

// formatAddressStr implements formatting of a raw address into a human readable textual form.
void formatAddressStr(uint8_t *address, cx_sha3_t *sha3Context, uint8_t *out, size_t outputSize) {
    // make sure there is enough space in output buffer for the address, last byte is the terminator
    STATIC_ASSERT(SIZEOF(*out) > 40, "bad output address buffer");

    // make sure tha address is of expected size
    STATIC_ASSERT(SIZEOF(*address) == 20, "bad address size");

    // make sanity check, the buffer may never exceed this number
    ASSERT(outputSize < MAX_BUFFER_SIZE);

    // prep checksum buffer
    uint8_t hashChecksum[32];

    // init SHA3 context
    cx_keccak_init(sha3Context, 256);

    // calculate SHA3 hash from the binary address so we can use it to mark checksum digits
    cx_hash((cx_hash_t *) sha3Context, CX_LAST, address, 20, hashChecksum, 32);

    // loop to convert address elements into the output string
    uint8_t i;
    for (i = 0; i < 40; i++) {
        out[i] = convertDigit(address, i, hashChecksum);
    }

    // terminate the string
    out[40] = '\0';
}

// convertDigit implements single digit conversion with hash being applied
// to the output address for extra safety.
char convertDigit(uint8_t *address, uint8_t index, uint8_t *hash) {
    // get next digit
    unsigned char digit = address[index / 2];

    // calculate the corresponding hexadecimal character involved
    if ((index % 2) == 0) {
        digit = (digit >> 4) & 0x0f;
    } else {
        digit = digit & 0x0f;
    }

    // if this is a letter digit, we check for hash application
    if (digit > 9) {
        unsigned char data = hash[index / 8];
        if ((data & MASK[index % 8]) != 0) {
            return HEXDIGITS[digit] - 'a' + 'A';
        }
    }

    // number are left intact
    return HEXDIGITS[digit];
}