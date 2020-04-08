#ifndef FANTOM_LEDGER_UINT256_H
#define FANTOM_LEDGER_UINT256_H

#include <stdint.h>
#include <stdbool.h>

// uint128_t declares 128 bit number.
typedef struct {
    uint64_t elements[2];
} uint128_t;

// uint256_t declares 256 bit number.
typedef struct {
    uint128_t elements[2];
} uint256_t;

// define human readable access to elements inside large uint-s.
#define UPPER_P(x) x->elements[0]
#define LOWER_P(x) x->elements[1]
#define UPPER(x) x.elements[0]
#define LOWER(x) x.elements[1]

// mul256 implements multiplication of two 256 bit numbers.
void mul256(uint256_t *number1, uint256_t *number2, uint256_t *target);

// uint256ConvertBE implements conversion from array of 8 bit values to uint256.
void uint256ConvertBE(const uint256_t *out, const uint8_t *data, uint32_t length);

// readUint128BE implements reading big endian 128 bit value from the given buffer.
void readUint128BE(const uint256_t *target, const uint8_t *buffer, size_t length);

// readUint256BE implements reading big endian 256 bit value from the given buffer.
void readUint256BE(const uint256_t *target, const uint8_t *buffer, size_t length);

// uint256ToString converts a 256 bit number to textual representation.
size_t uint256ToString(uint256_t *number, uint32_t baseParam, char *out, size_t outLength);

#endif //FANTOM_LEDGER_UINT256_H
