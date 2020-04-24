#ifndef FANTOM_LEDGER_TX_STREAM_H
#define FANTOM_LEDGER_TX_STREAM_H

#include <stdbool.h>
#include <stdint.h>

#include "transaction.h"

// tx_rlp_field_e declares RLP processed field reference
typedef enum {
    TX_RLP_NONE = 0,
    TX_RLP_ENVELOPE,
    TX_RLP_TYPE,
    TX_RLP_NONCE,
    TX_RLP_GAS_PRICE,
    TX_RLP_START_GAS,
    TX_RLP_RECIPIENT,
    TX_RLP_VALUE,
    TX_RLP_DATA,
    TX_RLP_V,
    TX_RLP_R,
    TX_RLP_S,
    TX_RLP_DONE
} tx_rlp_field_e;

// tx_stream_status_e declares status of a transaction stream processing
typedef enum {
    TX_STREAM_PROCESSING = 0,
    TX_STREAM_FINISHED,
    TX_STREAM_FAULT
} tx_stream_status_e;

// TX_FLAG_TYPE marks transactions with the type present
#define TX_FLAG_TYPE 0x01
#define RLP_LENGTH_BUFFER_SIZE 5

// tx_stream_context_t declares context of a transaction stream
typedef struct {
    // SHA3 hash of the transaction needs to keep the state
    // across incoming chunks of data from the host
    cx_sha3_t *sha3Context;

    // currently processed field details
    tx_rlp_field_e currentField;
    uint32_t currentFieldLength;
    uint32_t currentFieldPos;

    // some flags for the current field type
    bool isCurrentFieldList;
    bool isProcessingField;
    bool isFieldSingleByte;

    // we collect total length of the tx received
    uint32_t dataLength;

    // RLP peek buffer is used to collect the next RLP value
    // signature across data chunks
    uint8_t rlpBuffer[RLP_LENGTH_BUFFER_SIZE];
    uint32_t rlpBufferOffset;

    // work buffer for the currently processed chunk of data
    // received from the host via APDU
    uint8_t *workBuffer;
    uint32_t workBufferLength;

    // chain related processing flags
    uint32_t processingFlags;

    // transaction details container
    // we will be showing some parts of the tx to end user
    // so we need to keep it
    transaction_t *tx;
} tx_stream_context_t;


// txStreamInit implements new transaction stream initialization.
// We assign references, initialize SHA3 context and set the currently expected field.
void txStreamInit(
        tx_stream_context_t *ctx,
        cx_sha3_t *sha3Context,
        transaction_t *tx);

// txStreamProcess implements processing of a buffer of data into the transaction stream.
// Transaction details come from the host in chunks and we process each chunk here
// keeping track of the internal state so we know where we left of.
tx_stream_status_e txStreamProcess(
        tx_stream_context_t *ctx,
        uint8_t *buffer,
        uint32_t length,
        uint32_t flags);

#endif //FANTOM_LEDGER_TX_STREAM_H
