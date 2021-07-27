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

#include "common.h"
#include "utils.h"
#include "rlp_utils.h"
#include "tx_stream.h"
#include "transaction.h"
#include "errors.h"

// txStreamInit implements new transaction stream initialization.
void txStreamInit(
        tx_stream_context_t *stream,
        cx_sha3_t *sha3Context,
        transaction_t *tx
) {
    // clear the context
    memset(stream, 0, sizeof(tx_stream_context_t));

    // assign SHA3 and init the SHA3 context
    stream->sha3Context = sha3Context;
    cx_keccak_init(stream->sha3Context, 256);

    // keep the transaction content reference
    stream->tx = tx;

    // assign initial expected value
    // TX_RLP_ENVELOPE is the transaction envelope list since
    // the transaction is sent as RLP encoded list of values
    stream->currentField = TX_RLP_ENVELOPE;

    // reset field stats
    stream->isProcessingField = false;
    stream->isFieldSingleByte = false;
}

// txStreamReadByte implements reading singe byte of data from the stream work buffer.
// We use it to detect length field in the incoming data which precedes all the data
// fields except self-encoded single byte data elements.
static uint8_t txStreamReadByte(tx_stream_context_t *stream) {
    uint8_t data;

    // make sure we are safely withing a current command length
    VALIDATE(stream->workBufferLength >= 1, ERR_INVALID_DATA);

    // read the data from work buffer and advance pointers
    data = *stream->workBuffer;
    stream->workBuffer++;
    stream->workBufferLength--;

    // advance field position so we track position in field parsing
    if (stream->isProcessingField) {
        stream->currentFieldPos++;
    }

    // add the field hash to SHA3 context if appropriate
    // we hash everything what isn't single byte field element
    if (!(stream->isProcessingField && stream->isFieldSingleByte)) {
        cx_hash((cx_hash_t *) stream->sha3Context, 0, &data, 1, NULL, 0);
    }

    return data;
}

// txStreamCopyData implements copying data from transaction stream into an output buffer.
static void txStreamCopyData(tx_stream_context_t *stream, uint8_t *out, size_t length) {
    // validate we are safely inside the current command length
    VALIDATE(stream->workBufferLength >= length, ERR_INVALID_DATA);

    // make sure the output buffer is valid before we move the data
    if (out != NULL) {
        // make a sanity check for the max expected transfer length
        // we surely never store more than this amount of data
        ASSERT(length < MAX_BUFFER_SIZE);

        // transfer the data
        memcpy(out, stream->workBuffer, length);
    }

    // add the data hash to SHA3 context if appropriate
    // We hash everything that isn't single byte field element,
    // those were already hashed when we tried to detect field length.
    // Small values below 0x80 are encoded directly in place with RLP.
    if (!(stream->isProcessingField && stream->isFieldSingleByte)) {
        cx_hash((cx_hash_t *) stream->sha3Context, 0, stream->workBuffer, length, NULL, 0);
    }

    // advance the work buffer and clear the command length we already processed
    stream->workBuffer += length;
    stream->workBufferLength -= length;

    // if processing a field, mark the advancement on that field as well
    if (stream->isProcessingField) {
        stream->currentFieldPos += length;
    }
}

// txStreamProcessEnvelope handles tx content processing.
// The content represents the top level envelope for list of actual tx values.
static void txStreamProcessEnvelope(tx_stream_context_t *stream) {
    // the content should be marked as a list of values
    VALIDATE(stream->isCurrentFieldList, ERR_INVALID_DATA);

    // keep data length reference for sanity checks
    stream->dataLength = stream->currentFieldLength;

    // advance expected field processing to the next one
    // see tx_stream.h for list of expected tx fields
    stream->currentField++;
    stream->isProcessingField = false;
}

// txStreamProcessInt256Field implements 256 bits unsigned integer transaction field processing.
// If the field is NULL the data is actually not stored and are thrown away.
static void txStreamProcessInt256Field(tx_stream_context_t *stream, tx_int256_t *field, size_t fieldSize) {
    // the field must not be marked as a list of values, it's a single value
    VALIDATE(!stream->isCurrentFieldList, ERR_INVALID_DATA);

    // make sure the expected length of the field is appropriate
    // it has to fit in the provided buffer
    VALIDATE(stream->currentFieldLength <= fieldSize, ERR_INVALID_DATA);

    // are we safely inside the field length?
    if (stream->currentFieldPos < stream->currentFieldLength) {
        // how much data we need for the current field?
        uint32_t toCopy = (stream->currentFieldLength - stream->currentFieldPos);

        // if the work buffer does not contain the rest of the field, copy only
        // what we can from the buffer and the rest will come in the next APDU
        if (stream->workBufferLength < toCopy) {
            toCopy = stream->workBufferLength;
        }

        // copy into the target field, or throw away the data and just move to the next element?
        if (field != NULL) {
            // copy data to target field on the correct position
            txStreamCopyData(stream, field->value + stream->currentFieldPos, toCopy);
        } else {
            // just throw the data
            txStreamCopyData(stream, NULL, toCopy);
        }
    }

    // are we done with the field?
    // if so, move processing to the next field
    if (stream->currentFieldPos == stream->currentFieldLength) {
        // set the field real size if any
        if (field != NULL) {
            field->length = stream->currentFieldLength;
        }

        // advance parser processing to the next field
        // and reset processing status
        stream->currentField++;
        stream->isProcessingField = false;
        stream->isFieldSingleByte = false;
    }
}

// txStreamProcessGeneralField implements transaction field processing.
static void txStreamProcessGeneralField(tx_stream_context_t *stream) {
    // the field must not be marked as a list of values, it's a single value
    VALIDATE(!stream->isCurrentFieldList, ERR_INVALID_DATA);

    // are we safely inside the field length?
    if (stream->currentFieldPos < stream->currentFieldLength) {
        // how much data we need for the current field?
        uint32_t toCopy = (stream->currentFieldLength - stream->currentFieldPos);

        // if the work buffer does not contain the rest of the field, copy only
        // what we can from the buffer and the rest will come in the next APDU
        if (stream->workBufferLength < toCopy) {
            toCopy = stream->workBufferLength;
        }

        // just throw the data after we run SHA3 on it
        txStreamCopyData(stream, NULL, toCopy);
    }

    // are we done with the field?
    // if so, move processing to the next field
    if (stream->currentFieldPos == stream->currentFieldLength) {
        // advance parser processing to the next field
        // and reset processing status
        stream->currentField++;
        stream->isProcessingField = false;
        stream->isFieldSingleByte = false;
    }
}

// txStreamProcessAddressField implements transaction field processing.
// If the field is NULL the data is actually not stored and are thrown away.
static void txStreamProcessAddressField(tx_stream_context_t *stream, tx_address_t *address, uint32_t fieldSize) {
    // the field must not be marked as a list of values, it's a single value
    VALIDATE(!stream->isCurrentFieldList, ERR_INVALID_DATA);

    // make sure the length is appropriate; it has to fit in the provided buffer
    VALIDATE(stream->currentFieldLength <= fieldSize, ERR_INVALID_DATA);

    // are we safely inside the field length?
    if (stream->currentFieldPos < stream->currentFieldLength) {
        // how much data we need for the current field?
        uint32_t toCopy = (stream->currentFieldLength - stream->currentFieldPos);

        // if the work buffer does not contain the rest of the field, copy only
        // what we can from the buffer and the rest will come in the next APDU
        if (stream->workBufferLength < toCopy) {
            toCopy = stream->workBufferLength;
        }

        // copy into the target field, or throw away the data and just move to the next element?
        if (address != NULL) {
            // copy data to target field on the correct position
            txStreamCopyData(stream, address->value + stream->currentFieldPos, toCopy);
        } else {
            // just throw the data
            txStreamCopyData(stream, NULL, toCopy);
        }
    }

    // are we done with the field?
    // if so, move processing to the next field
    if (stream->currentFieldPos == stream->currentFieldLength) {
        // set the field real size if any
        if (address != NULL) {
            address->length = stream->currentFieldLength;
        }

        // advance the field
        stream->currentField++;
        stream->isProcessingField = false;
        stream->isFieldSingleByte = false;
    }
}

// txStreamProcessVField implements transaction V field processing.
// If the field is NULL the data is actually not stored and are thrown away.
static void txStreamProcessVField(tx_stream_context_t *stream, tx_v_t *v, uint32_t fieldSize) {
    // the field must not be marked as a list of values, it's a single value
    VALIDATE(!stream->isCurrentFieldList, ERR_INVALID_DATA);

    // make sure the length is appropriate; it has to fit in the provided buffer
    VALIDATE(stream->currentFieldLength <= fieldSize, ERR_INVALID_DATA);

    // are we safely inside the field length?
    if (stream->currentFieldPos < stream->currentFieldLength) {
        // how much data we need for the current field?
        uint32_t toCopy = (stream->currentFieldLength - stream->currentFieldPos);

        // if the work buffer does not contain the rest of the field, copy only
        // what we can from the buffer and the rest will come in the next APDU
        if (stream->workBufferLength < toCopy) {
            toCopy = stream->workBufferLength;
        }

        // copy into the target field, or throw away the data and just move to the next element?
        if (v != NULL) {
            // copy data to target field on the correct position
            txStreamCopyData(stream, v->value + stream->currentFieldPos, toCopy);
        } else {
            // just throw the data
            txStreamCopyData(stream, NULL, toCopy);
        }
    }

    // are we done with the field?
    // if so, move processing to the next field
    if (stream->currentFieldPos == stream->currentFieldLength) {
        // set the field real size if any
        if (v != NULL) {
            v->length = stream->currentFieldLength;
        }

        // advance the field
        stream->currentField++;
        stream->isProcessingField = false;
        stream->isFieldSingleByte = false;
    }
}


// txStreamParseFieldProxy implements field parsing proxy routing the parser based
// on the current field expected to be received.
static tx_stream_status_e txStreamParseFieldProxy(tx_stream_context_t *stream) {
    // decide what to do based on the current field parsed
    switch (stream->currentField) {
        case TX_RLP_ENVELOPE:
            // parse the transaction envelope; the RLP starts with
            // the whole transaction array envelope which than contains
            // all the fields encoded in a strict order
            txStreamProcessEnvelope(stream);

            // do we have a type in the TX transfer?
            // if not, skip the TX_RLP_TYPE field
            if ((stream->processingFlags & TX_FLAG_TYPE) == 0) {
                stream->currentField++;
            }
            break;
        case TX_RLP_TYPE:
            // we don't keep the type, but we do run parse on it since
            // all the incoming data participate on SHA3 hash building
            txStreamProcessInt256Field(stream, NULL, TX_MAX_INT256_LENGTH);
            break;
        case TX_RLP_NONCE:
            // we don't keep the sender's address nonce
            txStreamProcessInt256Field(stream, NULL, TX_MAX_INT256_LENGTH);
            break;
        case TX_RLP_GAS_PRICE:
            txStreamProcessInt256Field(stream, &stream->tx->gasPrice, TX_MAX_INT256_LENGTH);
            break;
        case TX_RLP_START_GAS:
            txStreamProcessInt256Field(stream, &stream->tx->startGas, TX_MAX_INT256_LENGTH);
            break;
        case TX_RLP_VALUE:
            txStreamProcessInt256Field(stream, &stream->tx->value, TX_MAX_INT256_LENGTH);
            break;
        case TX_RLP_RECIPIENT:
            txStreamProcessAddressField(stream, &stream->tx->recipient, TX_MAX_ADDRESS_LENGTH);
            break;
        case TX_RLP_DATA:
            // if we are on the beginning of the field, try to detect
            // smart contract call by calculating the data length rounding
            if (stream->currentFieldPos == 0) {
                // The data must contain at least signature and one parameter to qualify.
                // We do not consider no-param calls to lower the chance for false positives.
                // A contract call contains 4 bytes of method signature
                // plus list of params each padded to 32 bytes.
                stream->tx->isContractCall = (stream->currentFieldLength >= 4) &&
                                              ((stream->currentFieldLength - 4) % 32 == 0);
            }

            // process data as a general field, we don't need the content
            txStreamProcessGeneralField(stream);
            break;
        case TX_RLP_V:
            // this could/should contain chain identification
            txStreamProcessVField(stream, &stream->tx->v, TX_MAX_INT256_LENGTH);
            break;
        case TX_RLP_R:
            txStreamProcessGeneralField(stream);
            break;
        case TX_RLP_S:
            txStreamProcessGeneralField(stream);
            break;
        default:
            // we don't allow unknown fields to be processed
            return TX_STREAM_FAULT;
    }

    return TX_STREAM_PROCESSING;
}

// txStreamDetectField tries to detect field length block in the current data buffer.
static tx_stream_status_e txStreamDetectField(tx_stream_context_t *stream, bool *canDecode) {
    // we assume the decoding can not be done by default
    *canDecode = false;

    // read to the end of current buffer
    while (stream->workBufferLength != 0) {
        bool isValid;

        // validate if we are safely within the RLP buffer size
        if (stream->rlpBufferOffset == RLP_LENGTH_BUFFER_SIZE) {
            return TX_STREAM_FAULT;
        }

        // feed the RLP buffer until the new field length can be decoded
        // or until we run out of data in the current wire buffer
        stream->rlpBuffer[stream->rlpBufferOffset] = txStreamReadByte(stream);

        // advance the buffer position for the next byte
        stream->rlpBufferOffset++;

        // check if the current RLP buffer can reveal field
        if (rlpCanDecode(stream->rlpBuffer, stream->rlpBufferOffset, &isValid)) {
            // can not decode invalid data stream
            if (!isValid) {
                return TX_STREAM_FAULT;
            }

            // we can decode here
            *canDecode = true;
            break;
        }
    }

    return TX_STREAM_PROCESSING;
}

// txStreamParse implements incoming buffer parser.
static tx_stream_status_e txStreamParse(tx_stream_context_t *stream) {
    // loop until the buffer is parsed
    for (;;) {
        // are we done with the parsing? only EIP 155 transactions should reach this stage
        if (stream->currentField == TX_RLP_DONE) {
            return TX_STREAM_FINISHED;
        }

        // old school transactions don't have the <v> (chain Id) value and anything beyond
        // but we are working on EIP-155 chain and so this should never happen
        // The <v> is present and added to the hash to prevent replay attacks.

        // did we reach the end of this wire buffer?
        // exit the loop and inform the host we need another APDU
        // to finish the parsing of the transaction
        if (stream->workBufferLength == 0) {
            return TX_STREAM_PROCESSING;
        }

        // we are at an edge of a new field and we need to try to detect
        // the new field length and prepare the context to parse this
        // field from the RLP buffer
        if (!stream->isProcessingField) {
            bool canDecode;
            uint32_t offset;

            // try to detect next field in the current command buffer
            if (txStreamDetectField(stream, &canDecode) == TX_STREAM_FAULT) {
                return TX_STREAM_FAULT;
            }

            // if the current buffer can not decode next field
            // ask for the next APDU so we can continue parsing
            if (!canDecode) {
                return TX_STREAM_PROCESSING;
            }

            // decode field length and type
            if (!rlpDecodeLength(stream->rlpBuffer, stream->rlpBufferOffset, &stream->currentFieldLength, &offset,
                                 &stream->isCurrentFieldList)) {
                return TX_STREAM_FAULT;
            }

            // hack for self encoded single byte
            if (offset == 0) {
                // is this a single byte self encoded field?
                // in RLP encoding the first byte between 0x00 and 0x7f represent the actual value
                // and no offset is needed to decode the value since the next byte is already a new field
                stream->isFieldSingleByte = true;

                // shift the work buffer back to the processed field
                stream->workBuffer--;
                stream->workBufferLength++;
            }

            // reset RLP buffer, we used it to get the length
            stream->rlpBufferOffset = 0;

            // now we are processing a field
            stream->currentFieldPos = 0;
            stream->isProcessingField = true;
        }//(!stream->isProcessingField)

        // parse the current field
        if (txStreamParseFieldProxy(stream) == TX_STREAM_FAULT) {
            return TX_STREAM_FAULT;
        }
    }
}

// txStreamProcess implements processing of a buffer of data into the transaction stream.
tx_stream_status_e txStreamProcess(
        tx_stream_context_t *stream,
        uint8_t *buffer,
        uint32_t length,
        uint32_t flags
) {
    // collect parser result
    tx_stream_status_e result;

    // try to pick where we ended and continue parsing
    BEGIN_TRY
    {
        TRY
        {
            // validate we have at least some data to process
            VALIDATE(length > 0, ERR_INVALID_DATA);

            // make sure we are actually waiting for a field
            // the initial state is beyond TX_RLP_NONE and the TX_RLP_DONE means we don't need anything else
            VALIDATE(stream->currentField > TX_RLP_NONE && stream->currentField < TX_RLP_DONE, ERR_INVALID_DATA);

            // assign the buffer to context
            stream->workBuffer = buffer;
            stream->workBufferLength = length;
            stream->processingFlags = flags;

            // run stream handler
            result = txStreamParse(stream);
        }
        CATCH_OTHER(e)
        {
            result = TX_STREAM_FAULT;
        }
        FINALLY
        {
        }
    }
    END_TRY;

    return result;
}