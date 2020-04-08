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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "utils.h"
#include "errors.h"
#include "rlp_utils.h"

// rlpCanDecode implements RLP field length detection in given buffer.
// @see https://github.com/ethereum/wiki/wiki/RLP
bool rlpCanDecode(uint8_t *buffer, uint32_t length, bool *isValid) {
    // sanity check, we need to have at least single byte in the buffer
    VALIDATE(length > 0, ERR_INVALID_DATA);

    // buffer can never exceed max size; freak out if it does
    VALIDATE(length < MAX_BUFFER_SIZE, ERR_INVALID_DATA);

    // values 0x00 - 0x7f this is single byte value itself
    // values 0x80 - 0xb7 represent string (0x80) with max length 55 bytes
    // values 0xb8 - 0xbf represent string with size more than 55 bytes long
    if (*buffer >= 0xb8 && *buffer <= 0xbf) {
        // size of the length has been added to the 0xb7
        // so we need to find out if we already have the full size field
        if (length < (1 + (*buffer - 0xb7))) {
            return false;
        }

        // sizes over 0xbb means the length is to big and can not be handled
        // since there is arbitrary 32 bits length limitation in the encoding
        if (*buffer > 0xbb) {
            *isValid = false;
            return true;
        }
    }

    // values 0xc0 - 0xf7 represent list with total length of all items not exceeding 55 bytes
    // values 0xf8 - 0xff represent list with total length exceeding 55 bytes
    if (*buffer >= 0xf8) {
        // size of the length of the combined list content is added to 0xf7
        // so we need to make sure that we have the full length field in buffer
        if (length < (1 + (*buffer - 0xf7))) {
            return false;
        }

        // sizes over 0xfb means the list length is to big and can not be handled
        // since there is arbitrary 32 bits length limitation in the encoding
        if (*buffer > 0xfb) {
            *isValid = false;
            return true;
        }
    }

    // all other cases listed above are perfectly valid with just single byte
    // in the buffer since the field length, or the field itself can be derived
    // from the buffer
    *isValid = true;
    return true;
}

// rlpDecodeLength implements field length decoder.
// It collects field length information from the field buffer and populates
// some details about the field itself.
bool rlpDecodeLength(uint8_t *buffer, uint32_t bufferLength, uint32_t *fieldLength, uint32_t *offset, bool *isList) {
    // sanity check, we need to have at least single byte in the buffer
    VALIDATE(bufferLength > 0, ERR_INVALID_DATA);

    // decide what to do based on the buffer content
    if (*buffer <= 0x7f) {
        // this is the value itself, no decoding needed
        *offset = 0;
        *fieldLength = 1;
        *isList = false;
    } else if (*buffer <= 0xb7) {
        // this is single string with the length 0-55 bytes
        *offset = 1;
        *fieldLength = *buffer - 0x80;
        *isList = false;
    } else if (*buffer <= 0xbf) {
        // this is single string with length bigger than 55 bytes
        // the length is stored in subsequent buffer fields
        *offset = 1 + (*buffer - 0xb7);
        *isList = false;
        switch (*buffer) {
            case 0xb8:
                // sanity check, we need to have at least two bytes in the buffer
                VALIDATE(bufferLength > 1, ERR_INVALID_DATA);

                // get the length
                *fieldLength = *(buffer + 1);
                break;
            case 0xb9:
                // sanity check, we need to have at least three bytes in the buffer
                VALIDATE(bufferLength > 2, ERR_INVALID_DATA);

                // get the length
                *fieldLength = (*(buffer + 1) << 8) + *(buffer + 2);
                break;
            case 0xba:
                // sanity check, we need to have at least four bytes in the buffer
                VALIDATE(bufferLength > 3, ERR_INVALID_DATA);

                // get the length
                *fieldLength = (*(buffer + 1) << 16) + (*(buffer + 2) << 8) + *(buffer + 3);
                break;
            case 0xbb:
                // sanity check, we need to have at least five bytes in the buffer
                VALIDATE(bufferLength > 4, ERR_INVALID_DATA);

                // get the length
                *fieldLength = (*(buffer + 1) << 24) + (*(buffer + 2) << 16) +
                               (*(buffer + 3) << 8) + *(buffer + 4);
                break;
            default:
                // the length of the string can never exceed 32 bits
                return false;
        }
    } else if (*buffer <= 0xf7) {
        // this is a list with total combined length of all the fields 0-55 bytes
        *offset = 1;
        *fieldLength = *buffer - 0xc0;
        *isList = true;
    } else {
        // this is a list with total combined items length exceeding 55 bytes
        // the length is stored in subsequent buffer fields
        *offset = 1 + (*buffer - 0xf7);
        *isList = true;
        switch (*buffer) {
            case 0xf8:
                // sanity check, we need to have at least two bytes in the buffer
                VALIDATE(bufferLength > 1, ERR_INVALID_DATA);

                // get the length
                *fieldLength = *(buffer + 1);
                break;
            case 0xf9:
                // sanity check, we need to have at least three bytes in the buffer
                VALIDATE(bufferLength > 2, ERR_INVALID_DATA);

                // get the length
                *fieldLength = (*(buffer + 1) << 8) + *(buffer + 2);
                break;
            case 0xfa:
                // sanity check, we need to have at least four bytes in the buffer
                VALIDATE(bufferLength > 3, ERR_INVALID_DATA);

                // get the length
                *fieldLength = (*(buffer + 1) << 16) + (*(buffer + 2) << 8) + *(buffer + 3);
                break;
            case 0xfb:
                // sanity check, we need to have at least five bytes in the buffer
                VALIDATE(bufferLength > 4, ERR_INVALID_DATA);

                // get the length
                *fieldLength = (*(buffer + 1) << 24) + (*(buffer + 2) << 16) +
                               (*(buffer + 3) << 8) + *(buffer + 4);
                break;
            default:
                // the length of all the items combined can never exceed 32 bits
                return false;
        }
    }
    return true;
}
