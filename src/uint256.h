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

// isZero128 implements test if given 128 bit value is zero.
bool isZero128(uint128_t *number);

// isZero256 implements a test if given 256 bit value is zero.
bool isZero256(uint256_t *number);

// copy128 implements copying between source and destination 128 bit numbers.
void copy128(uint128_t *target, uint128_t *number);

// copy256 implements copying between source and destination 256 bit numbers.
void copy256(uint256_t *target, uint256_t *number);

// clear128 implements setting given target 128 bit number to zero.
void clear128(uint128_t *target);

// clear256 implements setting given target 256 bit number to zero.
void clear256(uint256_t *target);

// shiftLeft128 implements left shifting of a 128 bit number.
void shiftLeft128(uint128_t *number, uint32_t value, uint128_t *target);

// shiftRight128 implements right shifting of a 128 bit number.
void shiftRight128(uint128_t *number, uint32_t value, uint128_t *target);

// shiftLeft256 implements left shifting of a 256 bit number.
void shiftLeft256(uint256_t *number, uint32_t value, uint256_t *target);

// shiftRight256 implements right shifting of a 256 bit number.
void shiftRight256(uint256_t *number, uint32_t value, uint256_t *target);

// bits128 implements bit transfer of a 128 bit number.
uint32_t bits128(uint128_t *number);

// bits256 implements bit transfer of a 256 bit number.
uint32_t bits256(uint256_t *number);

// equal128 tests if two 128 bit numbers are equal.
bool equal128(uint128_t *number1, uint128_t *number2);

// equal256 tests if two 256 bit numbers are equal.
bool equal256(uint256_t *number1, uint256_t *number2);

// gt128 tests if first 128 bit number is greater than the last.
bool gt128(uint128_t *number1, uint128_t *number2);

// gt256 tests if first 256 bit number is greater than the last.
bool gt256(uint256_t *number1, uint256_t *number2);

// gt128 tests if first 128 bit number is greater than the last or if they are equal.
bool gte128(uint128_t *number1, uint128_t *number2);

// gt256 tests if first 256 bit number is greater than the last or if they are equal.
bool gte256(uint256_t *number1, uint256_t *number2);

// add128 implements adding two 128 bit numbers together.
void add128(uint128_t *number1, uint128_t *number2, uint128_t *target);

// add256 implements adding two 256 bit numbers together.
void add256(uint256_t *number1, uint256_t *number2, uint256_t *target);

// minus128 implements subtracting last 256 bit number from the first one.
void minus128(uint128_t *number1, uint128_t *number2, uint128_t *target);

// minus256 implements subtracting last 256 bit number from the first one.
void minus256(uint256_t *number1, uint256_t *number2, uint256_t *target);

// or128 implements logical superposition of two 128 bit numbers.
void or128(uint128_t *number1, uint128_t *number2, uint128_t *target);

// or256 implements logical superposition of two 256 bit numbers.
void or256(uint256_t *number1, uint256_t *number2, uint256_t *target);

// mul128 implements multiplication of two 128 bit numbers.
void mul128(uint128_t *number1, uint128_t *number2, uint128_t *target);

// mul256 implements multiplication of two 256 bit numbers.
void mul256(uint256_t *number1, uint256_t *number2, uint256_t *target);

// divMod128 implements division with modulo of two 128 bit numbers.
void divMod128(uint128_t *l, uint128_t *r, uint128_t *div, uint128_t *mod);

// divMod256 implements division with modulo of two 256 bit numbers.
void divMod256(uint256_t *l, uint256_t *r, uint256_t *div, uint256_t *mod);

// uint256ConvertBE implements conversion from array of 8 bit values to uint256.
void uint256ConvertBE(uint256_t *out, const uint8_t *data, size_t length);

// readUint128BE implements reading big endian 128 bit value from the given buffer.
void readUint128BE(uint128_t *target, const uint8_t *buffer, size_t length);

// readUint256BE implements reading big endian 256 bit value from the given buffer.
void readUint256BE(uint256_t *target, const uint8_t *buffer, size_t length);

// uint256ToString converts a 256 bit number to textual representation.
size_t uint256ToString(uint256_t *number, uint32_t baseParam, char *out, size_t outLength);

#endif //FANTOM_LEDGER_UINT256_H
