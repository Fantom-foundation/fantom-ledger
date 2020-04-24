#ifndef FANTOM_LEDGER_TRANSACTION_H
#define FANTOM_LEDGER_TRANSACTION_H

#include "bip44.h"

#define TX_HASH_LENGTH 32
#define TX_SIGNATURE_HASH_LENGTH 32
#define TX_MAX_INT256_LENGTH 32
#define TX_MAX_ADDRESS_LENGTH 20
#define TX_MAX_V_LENGTH 4

// tx_int256_t declares transaction value/unit type.
typedef struct {
    uint8_t value[TX_MAX_INT256_LENGTH];
    uint8_t length;
} tx_int256_t;

// tx_v_t declares transaction "v" value type.
typedef struct {
    uint8_t value[TX_MAX_V_LENGTH];
    uint8_t length;
} tx_v_t;

// tx_address_t declares transaction address.
typedef struct {
    uint8_t value[TX_MAX_ADDRESS_LENGTH];
    uint8_t length;
} tx_address_t;

// tx_signature_t declares transaction signature structure
typedef struct {
    uint8_t v;
    uint8_t r[TX_SIGNATURE_HASH_LENGTH];
    uint8_t s[TX_SIGNATURE_HASH_LENGTH];
} tx_signature_t;

// transaction_t declares transaction detail structure.
typedef struct {
    tx_int256_t gasPrice;
    tx_int256_t startGas;
    tx_int256_t value;
    tx_address_t recipient;
    tx_v_t v;
} transaction_t;

// txGetV implements transaction "v" value calculator.
// The "v" value is used to identify chain on which the transaction should exist.
uint32_t txGetV(transaction_t *tx);

// txGetSignature implements ECDSA signature calculation of a transaction hash.
void txGetSignature(
        tx_signature_t *signature,
        bip44_path_t *path,
        uint8_t *hash,
        size_t hashLength
);

// txGetFormattedAmount creates human readable string representation of given int256 amount/value converted to FTM.
void txGetFormattedAmount(tx_int256_t *value, uint8_t decimals, char *out, size_t outSize);

// txGetFormattedFee calculates the transaction fee and formats it to human readable FTM value.
void txGetFormattedFee(transaction_t *tx, uint8_t decimals, char *out, size_t outSize);

#endif //FANTOM_LEDGER_TRANSACTION_H
