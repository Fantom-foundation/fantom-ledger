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
    TX_STREAM_PROCESSING,
    TX_STREAM_FINISHED,
    TX_STREAM_FAULT
} tx_stream_status_e;

// TX_FLAG_TYPE marks transactions with the type present
#define TX_FLAG_TYPE 0x01
#define RLP_LENGTH_BUFFER_SIZE 5

// tx_stream_context_t declares context of a transaction stream
typedef struct {
    tx_rlp_field_e currentField;
    uint32_t currentFieldLength;
    uint32_t currentFieldPos;

    bool isCurrentFieldList;
    bool isProcessingField;
    bool isFieldSingleByte;

    uint32_t dataLength;
    uint8_t rlpBuffer[RLP_LENGTH_BUFFER_SIZE];
    uint32_t rlpBufferPos;
    uint8_t *workBuffer;
    uint32_t commandLength;
    uint32_t processingFlags;

    transaction_t *tx;
    cx_sha3_t *sha3;
} tx_stream_context_t;


// txStreamInit implements new transaction stream initialization.
void txStreamInit(
        tx_stream_context_t *ctx,
        cx_sha3_t *sha3,
        transaction_t *tx);

// txStreamProcess implements processing of a buffer of data into the transaction stream.
tx_stream_status_e txStreamProcess(
        tx_stream_context_t *ctx,
        uint8_t *buffer,
        uint32_t length,
        uint32_t flags);

#endif //FANTOM_LEDGER_TX_STREAM_H
