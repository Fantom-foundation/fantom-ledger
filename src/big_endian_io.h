/**
 * This implements helpers for reading and writing unsigned int values
 * of different sizes from and to a buffer in Big-Endian byte order.
 * The most significant value in the sequence is stored first,
 * at the lower buffer address.
 */
#ifndef FANTOM_LEDGER_ENDIAN_H
#define FANTOM_LEDGER_ENDIAN_H

#include <stdint.h>
#include "assert.h"

// u1be_write implements writing one byte uint8_t value to a buffer.
inline void u1be_write(uint8_t *outBuffer, uint8_t value) {
    outBuffer[0] = value;
}

// u2be_write implements writing two byte uint16_t value to a buffer.
inline void u2be_write(uint8_t *outBuffer, uint16_t value) {
    // write upper byte first, than the lower byte
    u1be_write(outBuffer, value >> 8);
    u1be_write(outBuffer + 1, value & 0xFF);
}

// u4be_write implements writing 4 byte uint32_t value to a buffer.
inline void u4be_write(uint8_t *outBuffer, uint32_t value) {
    // write upper two bytes first, than the lower two
    u2be_write(outBuffer, value >> 16);
    u2be_write(outBuffer + 2, value & 0xFFff);
}

// u8be_write implements writing 8 byte uint64_t value to a buffer.
inline void u8be_write(uint8_t *outBuffer, uint64_t value) {
    // write upper four bytes first, than the lower four
    u4be_write(outBuffer, (value >> 32));
    u4be_write(outBuffer + 4, value & 0xFFffFFff);
}

// u1be_read implements reading one byte uint8_t value from a buffer.
inline uint8_t u1be_read(const uint8_t *inBuffer) {
    return inBuffer[0];
}

// u2be_read implements reading two byte uint16_t value from a buffer.
inline uint16_t u2be_read(const uint8_t *inBuffer) {
    // make sure the type size is what we expect
    STATIC_ASSERT(sizeof(uint32_t) == sizeof(unsigned), "bad unsigned size");

    // read upper byte first, than the lower byte
    return (uint16_t)(((uint32_t)(u1be_read(inBuffer) << 8)) | ((uint32_t)(u1be_read(inBuffer + 1))));
}

// u4be_read implements reading 4 byte uint32_t value from a buffer.
inline uint32_t u4be_read(const uint8_t *inBuffer) {
    // read upper two bytes first, than the lower two
    return ((uint32_t) u2be_read(inBuffer) << 16) | (uint32_t)(u2be_read(inBuffer + 2));
}

// u8be_read implements reading 8 byte uint64_t value from a buffer.
inline uint64_t u8be_read(const uint8_t *inBuffer) {
    // read upper four bytes first, than the four two
    return ((uint64_t) u4be_read(inBuffer) << 32u) | (uint64_t)(u4be_read(inBuffer + 4));
}

#endif //FANTOM_LEDGER_ENDIAN_H
