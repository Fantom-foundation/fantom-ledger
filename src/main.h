#ifndef FANTOM_LEDGER_MAIN_H
#define FANTOM_LEDGER_MAIN_H

// data exchange details
#define CLA 0xE0

// Offsets of various elements inside APDU packet.
#define OFFSET_CLA   0x00
#define OFFSET_INS   0x01
#define OFFSET_P1    0x02
#define OFFSET_P2    0x03
#define OFFSET_LC    0x04
#define OFFSET_CDATA 0x05

// security limits
#define MIN_BIP32_PATH 2
#define MAX_BIP32_PATH 10

#endif //FANTOM_LEDGER_MAIN_H
