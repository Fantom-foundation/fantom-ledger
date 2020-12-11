#include "common.h"
#include "errors.h"
#include "bip44.h"
#include "big_endian_io.h"

// MAX_REASONABLE_ACCOUNT is maximal number of accounts managed by the ledger app (index 2)
static const uint32_t MAX_REASONABLE_ACCOUNT = 10;

// OPERA_CHANGE_INTERNAL is index of internal Opera chain (internal change in terms of BIP44 specs)
static const uint32_t OPERA_CHANGE_INTERNAL = 1;

// OPERA_CHANGE_EXTERNAL is index of public Opera chain (external change in terms of BIP44 specs)
static const uint32_t OPERA_CHANGE_EXTERNAL = 0;

// MAX_REASONABLE_ADDRESS is the maximal number of addresses derived for single account (index 4)
static const uint32_t MAX_REASONABLE_ADDRESS = 1000000;

// bip44_parseFromWire implements parsing of BIP44 path from incoming APDU buffer.
size_t bip44_parseFromWire(
        bip44_path_t *path,
        const uint8_t *dataBuffer, size_t dataSize
) {
    // make sure the buffer length make sense
    VALIDATE(dataSize >= 1, ERR_INVALID_DATA);

    // get the number of BIP32 path elements signaled to be in the buffer
    // see INS specification for incoming data payload ([0] = Number of BIP32 Derivations)
    size_t length = dataBuffer[0];

    // make sure the number of BIP32 derivations corresponds with expected path length
    VALIDATE(length <= ARRAY_LEN(path->path), ERR_INVALID_DATA);

    // make sure the buffer size is in pair with the announced BIP32 derivations
    // each derivation is 4 bytes long, the first byte is the number of derivations included
    VALIDATE(length * 4 + 1 <= dataSize, ERR_INVALID_DATA);

    // keep the number of derivations
    path->length = length;

    // loop to extract derivations out of the buffer to the path
    size_t offset = 1;
    for (size_t i = 0; i < length; i++) {
        path->path[i] = u4be_read(dataBuffer + offset);
        offset += 4;
    }

    return offset;
}

// bip44_isHardened implements check if the given index value is hardened.
bool bip44_isHardened(uint32_t value) {
    return value == (value | HARDENED_BIP32);
}

// bip44_hasValidFantomPrefix implements test for valid BIP44 prefix in the BIP44 path
// for Fantom address derivation.
bool bip44_hasValidFantomPrefix(const bip44_path_t *path) {
#define CHECK(cond) if (!(cond)) return false
    // we should have at least the account number
    CHECK(path->length > BIP44_I_COIN_TYPE);

    // the purpose should be #44 (BIP44 specification) and should be hardened
    CHECK(path->path[BIP44_I_PURPOSE] == (BIP44 | HARDENED_BIP32));

    // the coin type has to be valid Fantom coin type and should be hardened
    CHECK(path->path[BIP44_I_COIN_TYPE] == (FANTOM_COIN_TYPE | HARDENED_BIP32));
    return true;
#undef CHECK
}

// bip44_containsAccount implements check if the BIP44 path contains account number (index 2).
bool bip44_containsAccount(const bip44_path_t *path) {
    return path->length > BIP44_I_ACCOUNT;
}

// bip44_getAccount returns the account index of the BIP44 path (index 2).
uint32_t bip44_getAccount(const bip44_path_t *path) {
    ASSERT(path->length > BIP44_I_ACCOUNT);
    return path->path[BIP44_I_ACCOUNT];
}

// bip44_hasReasonableAccount implements check if the BIP44
// path contains reasonable account number (index 2).
bool bip44_hasReasonableAccount(const bip44_path_t *path) {
    // account must be present
    if (!bip44_containsAccount(path)) return false;

    // account index must be hardened
    uint32_t account = bip44_getAccount(path);
    if (!bip44_isHardened(account)) return false;

    // remove hardening from the account index
    // so we can compare it to the max reasonable account
    account = account & (~HARDENED_BIP32);
    return account < MAX_REASONABLE_ACCOUNT;
}

// bip44_containsChangeType implements check if the BIP44 path includes chain type (index 3).
bool bip44_containsChangeType(const bip44_path_t *path) {
    return path->length > BIP44_I_CHANGE;
}

// bip44_getChangeType returns the chain type value of the BIP44 path (index 3).
bool bip44_getChangeType(const bip44_path_t *path) {
    // must have the change / chain index
    ASSERT(path->length > BIP44_I_CHANGE);
    return path->path[BIP44_I_CHANGE];
}

// bip44_hasValidChangeType implements check for valid chain type in the BIP44 path (index 3).
bool bip44_hasValidChangeType(const bip44_path_t *path) {
    // change type is present in the path
    if (!bip44_containsChangeType(path)) return false;

    // get the change type value and make sure it is valid
    uint32_t type = bip44_getChangeType(path);
    return (type == OPERA_CHANGE_EXTERNAL) || (type == OPERA_CHANGE_INTERNAL);
}

// bip44_containsAddress implements check if the BIP44 path
// includes address index within the account (index 4)
bool bip44_containsAddress(const bip44_path_t *path) {
    return path->length > BIP44_I_ADDRESS;
}

// bip44_getAddress returns the address index of the BIP44 path.
uint32_t bip44_getAddress(const bip44_path_t *path) {
    // must have the address index
    ASSERT(path->length > BIP44_I_ADDRESS);
    return path->path[BIP44_I_ADDRESS];
}

// bip44_hasReasonableAddress implements check if the BIP44 path
// includes reasonable address index within the account (index 4)
bool bip44_hasReasonableAddress(const bip44_path_t *path) {
    // the address has to be present
    if (!bip44_containsAddress(path)) return false;

    // get the address index and compare it to the reasonable limit
    uint32_t address = bip44_getAddress(path);
    return (address <= MAX_REASONABLE_ADDRESS);
}

// bip44_containsMoreThanAddress implements check if the BIP44 path contains
// indexes beyond address index (index 5+)
bool bip44_containsMoreThanAddress(const bip44_path_t *path) {
    return (path->length > BIP44_I_REST);
}

// bip44_pathToStr converts BIP44 path to human readable form for displaying.
void bip44_pathToStr(const bip44_path_t *path, char *out, size_t outSize) {
    // make sure the buffer is not exceeding sane size (we have only 4kB of memory after all)
    ASSERT(outSize < MAX_BUFFER_SIZE);

    // we need to have at least space for string terminator
    ASSERT(outSize > 0);

    char *ptr = out;
    char *end = (out + outSize);

#define WRITE(fmt, ...) \
    { \
        /* make sure we have enough space left */ \
        ASSERT(ptr <= end); \
        /* make sure the buffer is of the right type */ \
        ASSERT(sizeof(end - ptr) == sizeof(size_t)); \
        /* how much space do we have left */ \
        size_t availableSize = (size_t) (end - ptr); \
        /* push the formatted string */ \
        snprintf(ptr, availableSize, fmt, ##__VA_ARGS__); \
        /* how much did we spend */ \
        size_t res = strlen(ptr); \
        /* make sure we are well below the buffer size */ \
        ASSERT(res + 1 < availableSize); \
        /* advance buffer position */ \
        ptr += res; \
    }

    // path textual representation starts with static char <m>
    // see https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki
    // but since it's always there, we may not need it.
    // WRITE("m");

    // make sure the number of elements in the path does not exceed
    // total available positions. This is sanity check, the parser
    // should have already checked the value.
    ASSERT(path->length < ARRAY_LEN(path->path));

    // parse each individual index in the BIP32 path
    for (size_t i = 0; i < path->length; i++) {
        uint32_t value = path->path[i];

        // hardened values are marked with apostrophe after the value
        if ((value & HARDENED_BIP32) == HARDENED_BIP32) {
            WRITE("/%d'", (int) (value & ~HARDENED_BIP32));
        } else {
            WRITE("/%d", (int) value);
        }
    }

    // make sure we did not leaked
    ASSERT(ptr < end);
}