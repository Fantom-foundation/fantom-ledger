#ifndef FANTOM_LEDGER_HANDLERS_H
#define FANTOM_LEDGER_HANDLERS_H

// get default libs
#include "common.h"

// declare the instruction handler
typedef void handler_fn_t(
        uint8_t p1,
        uint8_t p2,
        uint8_t *wireBuffer,
        size_t wireSize,
        bool isNewCall
);

// getHandler consumes incoming instruction and returns corresponding handler
// responsible for processing the instruction.
// NULL is returned if the instruction is not recognized,
// in that case the main loop throws ERR_UNKNOWN_INS.
handler_fn_t *getHandler(uint8_t ins);

#endif
