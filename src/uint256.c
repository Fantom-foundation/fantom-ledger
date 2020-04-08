/*******************************************************************************
*   Fantom Ledger App
*   (c) 2020 Fantom Foundation
*
*  Parts of the code are adapted from Ledger Ethereum App distributed
*  with Apache license, version 2.0. You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Originally adapted from https://github.com/calccrypto/uint256_t
*
********************************************************************************/

#include "utils.h"
#include "errors.h"
#include "uint256.h"

// HEXDIGITS declares a list of hexadecimal digits.
static const char HEXDIGITS[] = "0123456789abcdef";

// isZero128 implements test if given 128 bit value is zero.
static bool isZero128(uint128_t *number) {
    return ((LOWER_P(number) == 0) && (UPPER_P(number) == 0));
}

// isZero256 implements a test if given 256 bit value is zero.
static bool isZero256(uint256_t *number) {
    return (zero128(&LOWER_P(number)) && zero128(&UPPER_P(number)));
}

// copy128 implements copying between source and destination 128 bit numbers.
static void copy128(uint128_t *target, uint128_t *number) {
    UPPER_P(target) = UPPER_P(number);
    LOWER_P(target) = LOWER_P(number);
}

// copy256 implements copying between source and destination 256 bit numbers.
static void copy256(uint256_t *target, uint256_t *number) {
    copy128(&UPPER_P(target), &UPPER_P(number));
    copy128(&LOWER_P(target), &LOWER_P(number));
}

// clear128 implements setting given target 128 bit number to zero.
static void clear128(uint128_t *target) {
    UPPER_P(target) = 0;
    LOWER_P(target) = 0;
}

// clear256 implements setting given target 256 bit number to zero.
static void clear256(uint256_t *target) {
    clear128(&UPPER_P(target));
    clear128(&LOWER_P(target));
}

// shiftLeft128 implements left shifting of a 128 bit number.
static void shiftLeft128(uint128_t *number, uint32_t value, uint128_t *target) {
    if (value >= 128) {
        clear128(target);
    } else if (value == 64) {
        UPPER_P(target) = LOWER_P(number);
        LOWER_P(target) = 0;
    } else if (value == 0) {
        copy128(target, number);
    } else if (value < 64) {
        UPPER_P(target) =
                (UPPER_P(number) << value) + (LOWER_P(number) >> (64 - value));
        LOWER_P(target) = (LOWER_P(number) << value);
    } else if ((128 > value) && (value > 64)) {
        UPPER_P(target) = LOWER_P(number) << (value - 64);
        LOWER_P(target) = 0;
    } else {
        clear128(target);
    }
}

// shiftLeft256 implements left shifting of a 256 bit number.
static void shiftLeft256(uint256_t *number, uint32_t value, uint256_t *target) {
    if (value >= 256) {
        clear256(target);
    } else if (value == 128) {
        copy128(&UPPER_P(target), &LOWER_P(number));
        clear128(&LOWER_P(target));
    } else if (value == 0) {
        copy256(target, number);
    } else if (value < 128) {
        uint128_t tmp1;
        uint128_t tmp2;
        uint256_t result;
        shiftLeft128(&UPPER_P(number), value, &tmp1);
        shiftRight128(&LOWER_P(number), (128 - value), &tmp2);
        add128(&tmp1, &tmp2, &UPPER(result));
        shiftLeft128(&LOWER_P(number), value, &LOWER(result));
        copy256(target, &result);
    } else if ((256 > value) && (value > 128)) {
        shiftLeft128(&LOWER_P(number), (value - 128), &UPPER_P(target));
        clear128(&LOWER_P(target));
    } else {
        clear256(target);
    }
}

// shiftRight128 implements right shifting of a 128 bit number.
static void shiftRight128(uint128_t *number, uint32_t value, uint128_t *target) {
    if (value >= 128) {
        clear128(target);
    } else if (value == 64) {
        UPPER_P(target) = 0;
        LOWER_P(target) = UPPER_P(number);
    } else if (value == 0) {
        copy128(target, number);
    } else if (value < 64) {
        uint128_t result;
        UPPER(result) = UPPER_P(number) >> value;
        LOWER(result) =
                (UPPER_P(number) << (64 - value)) + (LOWER_P(number) >> value);
        copy128(target, &result);
    } else if ((128 > value) && (value > 64)) {
        LOWER_P(target) = UPPER_P(number) >> (value - 64);
        UPPER_P(target) = 0;
    } else {
        clear128(target);
    }
}

// shiftRight256 implements right shifting of a 256 bit number.
static void shiftRight256(uint256_t *number, uint32_t value, uint256_t *target) {
    if (value >= 256) {
        clear256(target);
    } else if (value == 128) {
        copy128(&LOWER_P(target), &UPPER_P(number));
        clear128(&UPPER_P(target));
    } else if (value == 0) {
        copy256(target, number);
    } else if (value < 128) {
        uint128_t tmp1;
        uint128_t tmp2;
        uint256_t result;
        shiftRight128(&UPPER_P(number), value, &UPPER(result));
        shiftRight128(&LOWER_P(number), value, &tmp1);
        shiftLeft128(&UPPER_P(number), (128 - value), &tmp2);
        add128(&tmp1, &tmp2, &LOWER(result));
        copy256(target, &result);
    } else if ((256 > value) && (value > 128)) {
        shiftRight128(&UPPER_P(number), (value - 128), &LOWER_P(target));
        clear128(&UPPER_P(target));
    } else {
        clear256(target);
    }
}

// bits128 implements bit transfer of a 128 bit number.
static uint32_t bits128(uint128_t *number) {
    uint32_t result = 0;
    if (UPPER_P(number)) {
        result = 64;
        uint64_t up = UPPER_P(number);
        while (up) {
            up >>= 1;
            result++;
        }
    } else {
        uint64_t low = LOWER_P(number);
        while (low) {
            low >>= 1;
            result++;
        }
    }
    return result;
}

// bits256 implements bit transfer of a 256 bit number.
static uint32_t bits256(uint256_t *number) {
    uint32_t result = 0;
    if (!zero128(&UPPER_P(number))) {
        result = 128;
        uint128_t up;
        copy128(&up, &UPPER_P(number));
        while (!zero128(&up)) {
            shiftRight128(&up, 1, &up);
            result++;
        }
    } else {
        uint128_t low;
        copy128(&low, &LOWER_P(number));
        while (!zero128(&low)) {
            shiftRight128(&low, 1, &low);
            result++;
        }
    }
    return result;
}

// equal128 tests if two 128 bit numbers are equal.
static bool equal128(uint128_t *number1, uint128_t *number2) {
    return (UPPER_P(number1) == UPPER_P(number2)) &&
           (LOWER_P(number1) == LOWER_P(number2));
}

// equal256 tests if two 256 bit numbers are equal.
static bool equal256(uint256_t *number1, uint256_t *number2) {
    return (equal128(&UPPER_P(number1), &UPPER_P(number2)) &&
            equal128(&LOWER_P(number1), &LOWER_P(number2)));
}

// gt128 tests if first 128 bit number is greater than the last.
static bool gt128(uint128_t *number1, uint128_t *number2) {
    if (UPPER_P(number1) == UPPER_P(number2)) {
        return (LOWER_P(number1) > LOWER_P(number2));
    }
    return (UPPER_P(number1) > UPPER_P(number2));
}

// gt256 tests if first 256 bit number is greater than the last.
static bool gt256(uint256_t *number1, uint256_t *number2) {
    if (equal128(&UPPER_P(number1), &UPPER_P(number2))) {
        return gt128(&LOWER_P(number1), &LOWER_P(number2));
    }
    return gt128(&UPPER_P(number1), &UPPER_P(number2));
}

// gt128 tests if first 128 bit number is greater than the last or if they are equal.
static bool gte128(uint128_t *number1, uint128_t *number2) {
    return gt128(number1, number2) || equal128(number1, number2);
}

// gt256 tests if first 256 bit number is greater than the last or if they are equal.
static bool gte256(uint256_t *number1, uint256_t *number2) {
    return gt256(number1, number2) || equal256(number1, number2);
}

// add128 implements adding two 128 bit numbers together.
static void add128(uint128_t *number1, uint128_t *number2, uint128_t *target) {
    UPPER_P(target) =
            UPPER_P(number1) + UPPER_P(number2) +
            ((LOWER_P(number1) + LOWER_P(number2)) < LOWER_P(number1));
    LOWER_P(target) = LOWER_P(number1) + LOWER_P(number2);
}

// add256 implements adding two 256 bit numbers together.
static void add256(uint256_t *number1, uint256_t *number2, uint256_t *target) {
    uint128_t tmp;
    add128(&UPPER_P(number1), &UPPER_P(number2), &UPPER_P(target));
    add128(&LOWER_P(number1), &LOWER_P(number2), &tmp);
    if (gt128(&LOWER_P(number1), &tmp)) {
        uint128_t one;
        UPPER(one) = 0;
        LOWER(one) = 1;
        add128(&UPPER_P(target), &one, &UPPER_P(target));
    }
    add128(&LOWER_P(number1), &LOWER_P(number2), &LOWER_P(target));
}

// minus128 implements subtracting last 256 bit number from the first one.
static void minus128(uint128_t *number1, uint128_t *number2, uint128_t *target) {
    UPPER_P(target) =
            UPPER_P(number1) - UPPER_P(number2) -
            ((LOWER_P(number1) - LOWER_P(number2)) > LOWER_P(number1));
    LOWER_P(target) = LOWER_P(number1) - LOWER_P(number2);
}

// minus256 implements subtracting last 256 bit number from the first one.
static void minus256(uint256_t *number1, uint256_t *number2, uint256_t *target) {
    uint128_t tmp;
    minus128(&UPPER_P(number1), &UPPER_P(number2), &UPPER_P(target));
    minus128(&LOWER_P(number1), &LOWER_P(number2), &tmp);
    if (gt128(&tmp, &LOWER_P(number1))) {
        uint128_t one;
        UPPER(one) = 0;
        LOWER(one) = 1;
        minus128(&UPPER_P(target), &one, &UPPER_P(target));
    }
    minus128(&LOWER_P(number1), &LOWER_P(number2), &LOWER_P(target));
}

// or128 implements logical superposition of two 128 bit numbers.
static void or128(uint128_t *number1, uint128_t *number2, uint128_t *target) {
    UPPER_P(target) = UPPER_P(number1) | UPPER_P(number2);
    LOWER_P(target) = LOWER_P(number1) | LOWER_P(number2);
}

// or256 implements logical superposition of two 256 bit numbers.
static void or256(uint256_t *number1, uint256_t *number2, uint256_t *target) {
    or128(&UPPER_P(number1), &UPPER_P(number2), &UPPER_P(target));
    or128(&LOWER_P(number1), &LOWER_P(number2), &LOWER_P(target));
}

// mul128 implements multiplication of two 128 bit numbers.
static void mul128(uint128_t *number1, uint128_t *number2, uint128_t *target) {
    uint64_t top[4] = {UPPER_P(number1) >> 32, UPPER_P(number1) & 0xffffffff,
                       LOWER_P(number1) >> 32, LOWER_P(number1) & 0xffffffff};
    uint64_t bottom[4] = {UPPER_P(number2) >> 32, UPPER_P(number2) & 0xffffffff,
                          LOWER_P(number2) >> 32,
                          LOWER_P(number2) & 0xffffffff};
    uint64_t products[4][4];
    uint128_t tmp, tmp2;

    for (int y = 3; y > -1; y--) {
        for (int x = 3; x > -1; x--) {
            products[3 - x][y] = top[x] * bottom[y];
        }
    }

    uint64_t fourth32 = products[0][3] & 0xffffffff;
    uint64_t third32 = (products[0][2] & 0xffffffff) + (products[0][3] >> 32);
    uint64_t second32 = (products[0][1] & 0xffffffff) + (products[0][2] >> 32);
    uint64_t first32 = (products[0][0] & 0xffffffff) + (products[0][1] >> 32);

    third32 += products[1][3] & 0xffffffff;
    second32 += (products[1][2] & 0xffffffff) + (products[1][3] >> 32);
    first32 += (products[1][1] & 0xffffffff) + (products[1][2] >> 32);

    second32 += products[2][3] & 0xffffffff;
    first32 += (products[2][2] & 0xffffffff) + (products[2][3] >> 32);

    first32 += products[3][3] & 0xffffffff;

    UPPER(tmp) = first32 << 32;
    LOWER(tmp) = 0;
    UPPER(tmp2) = third32 >> 32;
    LOWER(tmp2) = third32 << 32;
    add128(&tmp, &tmp2, target);
    UPPER(tmp) = second32;
    LOWER(tmp) = 0;
    add128(&tmp, target, &tmp2);
    UPPER(tmp) = 0;
    LOWER(tmp) = fourth32;
    add128(&tmp, &tmp2, target);
}

// mul256 implements multiplication of two 256 bit numbers.
void mul256(uint256_t *number1, uint256_t *number2, uint256_t *target) {
    uint128_t top[4];
    uint128_t bottom[4];
    uint128_t products[4][4];
    uint128_t tmp, tmp2, fourth64, third64, second64, first64;
    uint256_t target1, target2;
    UPPER(top[0]) = 0;
    LOWER(top[0]) = UPPER(UPPER_P(number1));
    UPPER(top[1]) = 0;
    LOWER(top[1]) = LOWER(UPPER_P(number1));
    UPPER(top[2]) = 0;
    LOWER(top[2]) = UPPER(LOWER_P(number1));
    UPPER(top[3]) = 0;
    LOWER(top[3]) = LOWER(LOWER_P(number1));
    UPPER(bottom[0]) = 0;
    LOWER(bottom[0]) = UPPER(UPPER_P(number2));
    UPPER(bottom[1]) = 0;
    LOWER(bottom[1]) = LOWER(UPPER_P(number2));
    UPPER(bottom[2]) = 0;
    LOWER(bottom[2]) = UPPER(LOWER_P(number2));
    UPPER(bottom[3]) = 0;
    LOWER(bottom[3]) = LOWER(LOWER_P(number2));

    for (int y = 3; y > -1; y--) {
        for (int x = 3; x > -1; x--) {
            mul128(&top[x], &bottom[y], &products[3 - x][y]);
        }
    }

    UPPER(fourth64) = 0;
    LOWER(fourth64) = LOWER(products[0][3]);
    UPPER(tmp) = 0;
    LOWER(tmp) = LOWER(products[0][2]);
    UPPER(tmp2) = 0;
    LOWER(tmp2) = UPPER(products[0][3]);
    add128(&tmp, &tmp2, &third64);
    UPPER(tmp) = 0;
    LOWER(tmp) = LOWER(products[0][1]);
    UPPER(tmp2) = 0;
    LOWER(tmp2) = UPPER(products[0][2]);
    add128(&tmp, &tmp2, &second64);
    UPPER(tmp) = 0;
    LOWER(tmp) = LOWER(products[0][0]);
    UPPER(tmp2) = 0;
    LOWER(tmp2) = UPPER(products[0][1]);
    add128(&tmp, &tmp2, &first64);

    UPPER(tmp) = 0;
    LOWER(tmp) = LOWER(products[1][3]);
    add128(&tmp, &third64, &tmp2);
    copy128(&third64, &tmp2);
    UPPER(tmp) = 0;
    LOWER(tmp) = LOWER(products[1][2]);
    add128(&tmp, &second64, &tmp2);
    UPPER(tmp) = 0;
    LOWER(tmp) = UPPER(products[1][3]);
    add128(&tmp, &tmp2, &second64);
    UPPER(tmp) = 0;
    LOWER(tmp) = LOWER(products[1][1]);
    add128(&tmp, &first64, &tmp2);
    UPPER(tmp) = 0;
    LOWER(tmp) = UPPER(products[1][2]);
    add128(&tmp, &tmp2, &first64);

    UPPER(tmp) = 0;
    LOWER(tmp) = LOWER(products[2][3]);
    add128(&tmp, &second64, &tmp2);
    copy128(&second64, &tmp2);
    UPPER(tmp) = 0;
    LOWER(tmp) = LOWER(products[2][2]);
    add128(&tmp, &first64, &tmp2);
    UPPER(tmp) = 0;
    LOWER(tmp) = UPPER(products[2][3]);
    add128(&tmp, &tmp2, &first64);

    UPPER(tmp) = 0;
    LOWER(tmp) = LOWER(products[3][3]);
    add128(&tmp, &first64, &tmp2);
    copy128(&first64, &tmp2);

    clear256(&target1);
    shiftLeft128(&first64, 64, &UPPER(target1));
    clear256(&target2);
    UPPER(UPPER(target2)) = UPPER(third64);
    shiftLeft128(&third64, 64, &LOWER(target2));
    add256(&target1, &target2, target);
    clear256(&target1);
    copy128(&UPPER(target1), &second64);
    add256(&target1, target, &target2);
    clear256(&target1);
    copy128(&LOWER(target1), &fourth64);
    add256(&target1, &target2, target);
}

// divMod128 implements division with modulo of two 128 bit numbers.
static void divMod128(uint128_t *l, uint128_t *r, uint128_t *retDiv,
                      uint128_t *retMod
) {
    uint128_t copyD, adder, resDiv, resMod;
    uint128_t one;
    UPPER(one) = 0;
    LOWER(one) = 1;
    uint32_t diffBits = bits128(l) - bits128(r);
    clear128(&resDiv);
    copy128(&resMod, l);
    if (gt128(r, l)) {
        copy128(retMod, l);
        clear128(retDiv);
    } else {
        shiftLeft128(r, diffBits, &copyD);
        shiftLeft128(&one, diffBits, &adder);
        if (gt128(&copyD, &resMod)) {
            shiftRight128(&copyD, 1, &copyD);
            shiftRight128(&adder, 1, &adder);
        }
        while (gte128(&resMod, r)) {
            if (gte128(&resMod, &copyD)) {
                minus128(&resMod, &copyD, &resMod);
                or128(&resDiv, &adder, &resDiv);
            }
            shiftRight128(&copyD, 1, &copyD);
            shiftRight128(&adder, 1, &adder);
        }
        copy128(retDiv, &resDiv);
        copy128(retMod, &resMod);
    }
}

// divMod256 implements division with modulo of two 256 bit numbers.
static void divMod256(uint256_t *l, uint256_t *r, uint256_t *retDiv, uint256_t *retMod) {
    uint256_t copyD, adder, resDiv, resMod;
    uint256_t one;
    clear256(&one);
    UPPER(LOWER(one)) = 0;
    LOWER(LOWER(one)) = 1;
    uint32_t diffBits = bits256(l) - bits256(r);
    clear256(&resDiv);
    copy256(&resMod, l);
    if (gt256(r, l)) {
        copy256(retMod, l);
        clear256(retDiv);
    } else {
        shiftLeft256(r, diffBits, &copyD);
        shiftLeft256(&one, diffBits, &adder);
        if (gt256(&copyD, &resMod)) {
            shiftRight256(&copyD, 1, &copyD);
            shiftRight256(&adder, 1, &adder);
        }
        while (gte256(&resMod, r)) {
            if (gte256(&resMod, &copyD)) {
                minus256(&resMod, &copyD, &resMod);
                or256(&resDiv, &adder, &resDiv);
            }
            shiftRight256(&copyD, 1, &copyD);
            shiftRight256(&adder, 1, &adder);
        }
        copy256(retDiv, &resDiv);
        copy256(retMod, &resMod);
    }
}

// stringReverse implements reversing of a string in a buffer.
static void stringReverse(char *str, size_t length) {
    uint32_t i, j;
    for (i = 0, j = length - 1; i < j; i++, j--) {
        uint8_t c;
        c = str[i];
        str[i] = str[j];
        str[j] = c;
    }
}

// uint256ConvertBE implements conversion from array of 8 bit values to uint256.
void uint256ConvertBE(const uint256_t *out, const uint8_t *data, size_t length) {
    // make sure the data is within reasonable length
    VALIDATE(length <= 32, ERR_ASSERT);

    // we parsed the data from incoming RLP stream
    // and data length may be smaller than the full 256 bit size
    // so we copy it to a local buffer padding the value with
    // zeros from left before doing conversion
    uint8_t tmp[32];
    os_memset(tmp, 0, 32);
    os_memmove(tmp + 32 - length, data, length);

    // read in the 256 bit value
    readUint256BE(target, tmp, sizeof(tmp));
}

// uint256ToString implements conversion of 256 bit unsigned integer to a string.
size_t uint256ToString(uint256_t *number, uint32_t baseParam, char *out, size_t outLength) {
    uint256_t rDiv;
    uint256_t rMod;
    uint256_t base;

    // validate the base is reasonable
    VALIDATE((baseParam = > 2) && (baseParam <= 16), ERR_ASSERT);

    // init state
    copy256(&rDiv, number);
    clear256(&rMod);
    clear256(&base);
    UPPER(LOWER(base)) = 0;
    LOWER(LOWER(base)) = baseParam;
    size_t offset = 0;

    do {
        // we run out of space
        if (offset > (outLength - 1)) {
            return 0;
        }

        // calculate division
        divMod256(&rDiv, &base, &rDiv, &rMod);

        // add the digit from modulo
        out[offset++] = HEXDIGITS[(uint8_t) LOWER(LOWER(rMod))];
    } while (!isZero256(&rDiv));

    // add terminator
    out[offset] = '\0';

    // reverse string since we got the result mirrored
    stringReverse(out, offset);

    // return the real length
    return offset;
}


// readUint64BE implements reading of big endian uint64 value from given buffer
static uint64_t readUint64BE(const uint8_t *buffer) {
    return (((uint64_t) buffer[0]) << 56) | (((uint64_t) buffer[1]) << 48) |
           (((uint64_t) buffer[2]) << 40) | (((uint64_t) buffer[3]) << 32) |
           (((uint64_t) buffer[4]) << 24) | (((uint64_t) buffer[5]) << 16) |
           (((uint64_t) buffer[6]) << 8) | (((uint64_t) buffer[7]));
}

// readUint128BE implements reading big endian 128 bit value from the given buffer.
void readUint128BE(const uint256_t *target, const uint8_t *buffer, size_t length) {
    // make sure the buffer size is what we expect
    VALIDATE(length >= 16, ERR_ASSERT);

    // read lower and upper part of the value
    UPPER_P(target) = readUint64BE(buffer);
    LOWER_P(target) = readUint64BE(buffer + 8);
}

// readUint256BE implements reading big endian 256 bit value from the given buffer
void readUint256BE(const uint256_t *target, const uint8_t *buffer, size_t length) {
    // make sure the buffer size is what we expect
    VALIDATE(length >= 32, ERR_ASSERT);

    // read upper and lower part of the 256 bits
    readUint128BE(&UPPER_P(target), buffer, 16);
    readUint128BE(&LOWER_P(target), buffer + 16, 16);
}
