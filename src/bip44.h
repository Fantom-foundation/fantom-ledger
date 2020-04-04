#ifndef FANTOM_LEDGER_BIP44_H
#define FANTOM_LEDGER_BIP44_H

#include "common.h"
#include "conf.h"

// bip44_path_t declares full BIP44 path type
// see BIP44 specification https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki
typedef struct {
    uint32_t path[BIP32_MAX_PATH_LENGTH];
    uint32_t length;
} bip44_path_t;

// BIP_44 defines BIP44 purpose index value (path index 0)
static const uint32_t BIP44 = 44;

// FANTOM_COIN_TYPE defines BIP44 coin type for Fantom/Ether (path index 1).
// This may need to be changed in the future if Fantom will opt for their own registered coin type.
static const uint32_t FANTOM_COIN_TYPE = 60;

// HARDENED_BIP32 defines a mask for hardened path index (apostrophe in the path value)
static const uint32_t HARDENED_BIP32 = ((uint32_t) 1 << 31);

// Specific indexes of elements of the BIP44 path
enum {
    BIP44_I_PURPOSE = 0,
    BIP44_I_COIN_TYPE = 1,
    BIP44_I_ACCOUNT = 2,
    BIP44_I_CHANGE = 3,
    BIP44_I_ADDRESS = 4,
    BIP44_I_REST = 5,
};

// bip44_parseFromWire implements parsing of BIP44 path from incoming APDU buffer.
size_t bip44_parseFromWire(
        bip44_path_t *path,
        const uint8_t *dataBuffer, size_t dataSize
);

// bip44_hasValidFantomPrefix implements test for valid BIP44 prefix in the BIP44 path
// for Fantom address derivation.
bool bip44_hasValidFantomPrefix(const bip44_path_t *path);

// bip44_containsAccount implements check if the BIP44 path contains account number (index 2).
bool bip44_containsAccount(const bip44_path_t *path);

// bip44_hasReasonableAccount implements check if the BIP44
// path contains reasonable account number (index 2).
bool bip44_hasReasonableAccount(const bip44_path_t *path);

// bip44_containsChangeType implements check if the BIP44 path includes chain type (index 3).
bool bip44_containsChangeType(const bip44_path_t *path);

// bip44_hasValidChangeType implements check for valid chain type in the BIP44 path (index 3).
bool bip44_hasValidChangeType(const bip44_path_t *path);

// bip44_containsAddress implements check if the BIP44 path
// includes address index within the account (index 4)
bool bip44_containsAddress(const bip44_path_t *path);

// bip44_hasReasonableAddress implements check if the BIP44 path
// includes reasonable address index within the account (index 4)
bool bip44_hasReasonableAddress(const bip44_path_t *path);

// bip44_containsMoreThanAddress implements check if the BIP44 path contains
// indexes beyond address index (index 5+)
bool bip44_containsMoreThanAddress(const bip44_path_t *path);

// bip44_isHardened implements check if the given index value is hardened.
bool bip44_isHardened(uint32_t value);

// bip44_pathToStr converts BIP44 path to human readable form for displaying.
void bip44_pathToStr(const bip44_path_t*, char* out, size_t outSize);

#endif //FANTOM_LEDGER_BIP44_H
