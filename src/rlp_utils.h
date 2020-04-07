#ifndef FANTOM_LEDGER_RLP_UTILS_H
#define FANTOM_LEDGER_RLP_UTILS_H

#include <stdint.h>
#include "cx.h"

// rlpCanDecode implements RLP field length detection in given buffer.
// @see https://github.com/ethereum/wiki/wiki/RLP
bool rlpCanDecode(uint8_t *buffer, uint32_t length, bool *isValid);

// rlpDecodeLength implements field length decoder.
// It collects field length information from the field buffer and populates
// some details about the field itself.
bool rlpDecodeLength(uint8_t *buffer, uint32_t bufferLength, uint32_t *fieldLength, uint32_t *offset, bool *isList);

#endif //FANTOM_LEDGER_RLP_UTILS_H
