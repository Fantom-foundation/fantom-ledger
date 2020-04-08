/*******************************************************************************
* Fantom Ledger App
* (c) 2020 Fantom Foundation
*
* Some parts of the code are derived from Ledger Ethereum App distributed under
* Apache License, Version 2.0. You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
********************************************************************************/
#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "errors.h"
#include "transaction.h"
#include "derive_key.h"
#include "bip44.h"
#include "uint256.h"

// txGetV implements transaction "v" value calculator.
// The "v" value is used to identify chain on which the transaction should exist.
uint32_t txGetV(const transaction_t *tx) {
    // default is no V
    uint32_t v = 0;

    // validate the V value length
    VALIDATE(tx->v.length >= 0 && tx->v.length <= 4, ERR_INVALID_DATA);

    // any V present?
    if (tx->v.length > 0) {
        uint8_t i;
        v = tx->v.value[0];

        // extract 32 bit value of the V from array of bytes
        for (i = 1; i < tx->v.length; i++) {
            v = (v << 8) | tx->v.value[i];
        }
    }

    return v;
}

// txGetSignature implements ECDSA signature calculation of a transaction hash.
void txGetSignature(
        const tx_signature_t *signature,
        const bip44_path_t *path,
        const uint8_t *hash,
        size_t hashLength
) {
    private_key_t privateKey;
    chain_code_t chainCode;
    uint8_t sigLength;
    uint8_t sig[100];

    // validate hash size
    VALIDATE(hashLength == TX_HASH_LENGTH, ERR_INVALID_DATA);

    // make sure the signature is of expected length (v + r + s)
    STATIC_ASSERT(SIZEOF(*signature) == 1 + TX_SIGNATURE_HASH_LENGTH + TX_SIGNATURE_HASH_LENGTH, "bad signature size");

    // do the extraction
    BEGIN_TRY
    {
        TRY
        {
            // derive private key
            derivePrivateKey(path, &chainCode, &privateKey);
            io_seproxyhal_io_heartbeat();

            // calculate signature of the hash
            unsigned int info = 0;
            sigLength = cx_ecdsa_sign(&privateKey,
                                      CX_RND_RFC6979 | CX_LAST,
                                      CX_SHA256,
                                      hash, hashLength,
                                      sig, sizeof(sig),
                                      &info);

            // any signature received?
            VALIDATE(sigLength > 0, ERR_INVALID_DATA);

            // calculate the V (parity) value
            signature->v = 27;

            // CX_ECCINFO_PARITY_ODD flag is set if Y is odd when computing k.G
            if (info & CX_ECCINFO_PARITY_ODD) {
                signature->v++;
            }
            if (info & CX_ECCINFO_xGTn) {
                signature->v += 2;
            }

            // ECDSA signature is encoded as TLV:  30 L 02 Lr r 02 Ls s
            // Where Lr + Ls are lengths and r + s are respective values
            // @see Ledger SDK /include/cx.h
            // copy the R value
            uint8_t xOffset = 4;
            uint8_t xLength = sig[xOffset - 1];
            if (xLength == 33) {
                xLength = 32;
                xOffset++;
            }

            // transfer the R data; validate that the size is ok for memory transfer
            VALIDATE(xLength <= TX_SIGNATURE_HASH_LENGTH, ERR_INVALID_DATA);
            memmove(signature->r + TX_SIGNATURE_HASH_LENGTH - xLength, sig + xOffset, xLength);

            // copy the S value; skip the r value and TagLEn
            xOffset = xLength + 2;
            xLength = sig[xOffset - 1];
            if (xLength == 33) {
                xLength = 32;
                xOffset++;
            }

            // transfer the S data; validate that the size is ok for memory transfer
            VALIDATE(xLength <= TX_SIGNATURE_HASH_LENGTH, ERR_INVALID_DATA);
            memmove(signature->s + TX_SIGNATURE_HASH_LENGTH - xLength, sig + xOffset, xLength);
        }
        FINALLY
        {
            // clean up the private key in memory so we don't leek it in any way after this call
            os_memset(&privateKey, 0, SIZEOF(privateKey));
        }
    }
    END_TRY;
}

// adjustDecimals adjust decimal places for the given decimal number.
// We use it to convert amounts from WEI to FTM units.
static size_t
adjustDecimals(const char *src, uint32_t srcLength, uint8_t decimals, const char *target, size_t targetLength) {
    // we need at least some space to convert
    VALIDATE(targetLength > 2, ERR_INVALID_DATA);

    // value is zero
    if ((srcLength == 1) && (*src == '0')) {
        target[0] = '0';
        target[1] = '\0';
        return 1;
    }

    uint32_t startOffset;
    uint32_t offset = 0;

    // the value is smaller than the top decimal and so the final output
    // will be value smaller than zero
    if (srcLength <= decimals) {
        // calculate the digits available
        uint32_t delta = decimals - srcLength;

        // do we have enough space?
        VALIDATE(targetLength > srcLength + 1 + 2 + delta, ERR_INVALID_DATA);

        // add prefix
        target[offset++] = '0';
        target[offset++] = '.';

        // add leading zeros
        for (uint32_t i = 0; i < delta; i++) {
            target[offset++] = '0';
        }

        // add actual digits
        startOffset = offset;
        for (uint32_t i = 0; i < srcLength; i++) {
            target[offset++] = src[i];
        }

        // add terminator
        target[offset] = '\0';
    } else {
        // the value is still bigger that number of decimals
        // so we need to split it into integer part and decimal part
        uint32_t sourceOffset = 0;
        uint32_t delta = srcLength - decimals;

        // do we have enough space to store the output?
        VALIDATE(targetLength > srcLength + 1 + 1, ERR_INVALID_DATA);

        // add digits up to the decimal
        while (offset < delta) {
            target[offset++] = src[sourceOffset++];
        }

        // any decimals? add delimiter
        if (decimals != 0) {
            target[offset++] = '.';
        }

        // add the rest of the number
        startOffset = offset;
        while (sourceOffset < srcLength) {
            target[offset++] = src[sourceOffset++];
        }

        // add terminator
        target[offset] = '\0';
    }

    // detect trailing zeros so we can remove them
    uint32_t lastZeroOffset = 0;
    for (uint32_t i = startOffset; i < offset; i++) {
        if (target[i] == '0') {
            if (lastZeroOffset == 0) {
                lastZeroOffset = i;
            }
        } else {
            lastZeroOffset = 0;
        }
    }

    // any trailing zeros detected?
    if (lastZeroOffset != 0) {
        offset = lastZeroOffset;
        target[lastZeroOffset] = '\0';

        if (target[lastZeroOffset - 1] == '.') {
            target[lastZeroOffset - 1] = '\0';
            offset = lastZeroOffset - 1;
        }
    }

    return offset;
}

// txGetFormattedAmount creates human readable string representation of given int256 amount/value converted to FTM.
void txGetFormattedAmount(const tx_int256_t *value, uint8_t decimals, const char *out, size_t outSize) {
    // make sanity check, the buffer may never exceed this size
    ASSERT(outSize < MAX_BUFFER_SIZE);

    // convert to 256 bit value
    uint256_t tmpValue;
    uint256ConvertBE(&tmpValue, value->value, value->length);

    // convert the value to decimal string
    char tmp[64];
    size_t length = uint256ToString(&tmpValue, 10, &tmp, sizeof(tmp));

    // make sure we have any number here
    VALIDATE(length > 0, ERR_INVALID_DATA);

    // adjust decimals and copy to output
    adjustDecimals(tmp, length, decimals, out, outSize);
}

// txGetFormattedFee calculates the transaction fee and formats it to human readable FTM value.
void txGetFormattedFee(const transaction_t *tx, uint8_t decimals, const char *out, size_t outSize) {
    // make sanity check, the buffer may never exceed this size
    ASSERT(outSize < MAX_BUFFER_SIZE);

    // prep conversion containers
    uint256_t gasPrice;
    uint256_t gasVolume;
    uint256_t fee;

    // calculate the max fee from gas price and available gas volume
    uint256ConvertBE(&gasPrice, tx->gasPrice.value, tx->gasPrice.length);
    uint256ConvertBE(&gasVolume, tx->startGas.value, tx->startGas.length);
    mul256(&gasPrice, &gasVolume, &fee);

    // convert the value to decimal string
    char tmp[64];
    size_t length = uint256ToString(&fee, 10, &tmp, sizeof(tmp));

    // make sure we have any number here
    VALIDATE(length > 0, ERR_INVALID_DATA);

    // adjust decimals and copy to output
    adjustDecimals(tmp, length, decimals, out, outSize);
}
