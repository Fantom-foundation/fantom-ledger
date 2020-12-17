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
#include "address_utils.h"
#include "bip44.h"
#include "uint256.h"

// txGetV implements transaction "v" value calculator.
// The "v" value is used to identify chain on which the transaction should exist.
uint32_t txGetV(transaction_t *tx) {
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
        bip44_path_t *path,
        uint8_t *hash,
        size_t hashLength,
        cx_sha3_t *sha3Context,
        tx_address_t *sender,
        tx_signature_t *signature
) {
    private_key_t privateKey;
    chain_code_t chainCode;
    uint8_t sigLength;
    uint8_t sig[100];

    // make sure the signature is of expected length (v + r + s)
    ASSERT(SIZEOF(*signature) == 1 + TX_SIGNATURE_HASH_LENGTH + TX_SIGNATURE_HASH_LENGTH);

    // make sure the space for the derived sender address is enough
    ASSERT(SIZEOF(*sender) == 1 + TX_MAX_ADDRESS_LENGTH);

    // validate hash size
    ASSERT(hashLength == TX_HASH_LENGTH);

    // do the extraction
    BEGIN_TRY
    {
        TRY
        {
            // beat the i/o
            io_seproxyhal_io_heartbeat();

            // derive private key
            derivePrivateKey(path, &chainCode, &privateKey);

            // derive the public key from the private one
            cx_ecfp_public_key_t publicKey;
            deriveRawPublicKey(&privateKey, &publicKey);

            // get raw address of the sender and put it into the address reference
            size_t adrSize = getRawAddress(&publicKey, sha3Context, sender->value, TX_MAX_ADDRESS_LENGTH);

            // make sanity check here so we are absolutely sure
            // the address is within the provided buffer
            ASSERT(adrSize <= TX_MAX_ADDRESS_LENGTH);
            sender->length = adrSize;

            // beat the i/o
            io_seproxyhal_io_heartbeat();

            // calculate signature of the hash
            unsigned int info = 0;
            sigLength = cx_ecdsa_sign(&privateKey,
                                      CX_RND_RFC6979 | CX_LAST,
                                      CX_SHA256,
                                      hash, hashLength,
                                      sig, sizeof(sig),
                                      &info);

            // beat the i/o
            io_seproxyhal_io_heartbeat();

            // sanity check, make sure we received a signature here
            ASSERT(sigLength > 0);

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

            // validate that the size is ok for memory transfer
            VALIDATE(xLength <= TX_SIGNATURE_HASH_LENGTH, ERR_INVALID_DATA);
            VALIDATE(xOffset + xLength < sigLength, ERR_INVALID_DATA);

            // transfer the <r> data
            memmove(signature->r + TX_SIGNATURE_HASH_LENGTH - xLength, sig + xOffset, xLength);

            // move to the <s> part; skip the copied <r> + tag LE <0x02> + <Ls>
            xOffset = xOffset + xLength + 2;
            xLength = sig[xOffset - 1];
            if (xLength == 33) {
                xLength = 32;
                xOffset++;
            }

            // validate that the size is ok for memory transfer
            VALIDATE(xLength <= TX_SIGNATURE_HASH_LENGTH, ERR_INVALID_DATA);
            VALIDATE(xOffset + xLength <= sigLength, ERR_INVALID_DATA);

            // transfer the <s> data
            memmove(signature->s + TX_SIGNATURE_HASH_LENGTH - xLength, sig + xOffset, xLength);
        }
        CATCH_OTHER(e)
        {
            // clean up the private key in memory so we don't leek it in any way and shape
            // do we need to do that if we re-throw? let's better be safe than sorry with PKs.
            os_memset(&privateKey, 0, SIZEOF(privateKey));

            // re-throw the exception so it's collected in the main loop
            THROW(e);
        }
        FINALLY
        {
            // clean up the private key in memory so we don't leek it in any way
            os_memset(&privateKey, 0, SIZEOF(privateKey));
        }
    }
    END_TRY;
}

// adjustDecimals adjust decimal places for the given decimal number.
// We use it to convert amounts from WEI to FTM units.
static size_t adjustDecimals(
        const char *src,
        uint32_t srcLength,
        uint8_t decimals,
        char *target,
        size_t targetLength
) {
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
void txGetFormattedAmount(tx_int256_t *value, uint8_t decimals, char *out, size_t outSize) {
    // make sanity check, the buffer may never exceed this size
    ASSERT(outSize < MAX_BUFFER_SIZE);

    // convert to 256 bit value
    uint256_t tmpValue;
    uint256ConvertBE(&tmpValue, value->value, value->length);

    // convert the value to decimal string
    char tmp[40];
    size_t length = uint256ToString(&tmpValue, 10, (char *) tmp, sizeof(tmp));

    // make sure we have any number here
    VALIDATE(length > 0, ERR_INVALID_DATA);

    // adjust decimals and copy to output
    adjustDecimals(tmp, length, decimals, out, outSize);
}

// txGetFormattedFee calculates the transaction fee and formats it to human readable FTM value.
void txGetFormattedFee(transaction_t *tx, uint8_t decimals, char *out, size_t outSize) {
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
    char tmp[40];
    size_t length = uint256ToString(&fee, 10, (char *) tmp, sizeof(tmp));

    // make sure we have any number here
    VALIDATE(length > 0, ERR_INVALID_DATA);

    // adjust decimals and copy to output
    adjustDecimals(tmp, length, decimals, out, outSize);
}
