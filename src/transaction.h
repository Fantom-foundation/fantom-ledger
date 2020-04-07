#ifndef FANTOM_LEDGER_TRANSACTION_H
#define FANTOM_LEDGER_TRANSACTION_H

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

// transaction_t declares transaction detail structure.
typedef struct {
    tx_int256_t gasPrice;
    tx_int256_t startGas;
    tx_int256_t value;
    tx_address_t recipient;
    tx_v_t v;
} transaction_t;

#endif //FANTOM_LEDGER_TRANSACTION_H
