#include "policy.h"
#include "bip44.h"

// defines macros for policy conditions to make policy related function more readable.
#define DENY_IF(expr)   if (expr) return POLICY_DENY;
#define WARN_IF(expr)   if (expr) return POLICY_WARN;
#define PROMPT_IF(expr) if (expr) return POLICY_PROMPT;
#define SHOW_IF(expr)   if (expr) return POLICY_SHOW;
#define ALLOW_IF(expr)  if (expr) return POLICY_ALLOW;

// policyForGetPublicKey implements policy test for public key extraction.
security_policy_t policyForGetPublicKey(const bip44_path_t *path) {
    // deny if the path does not contain valid Fantom prefix
    DENY_IF(!bip44_hasValidFantomPrefix(path));

    // deny if the path does not contain account index
    DENY_IF(!bip44_containsAccount(path));

    // warn if the path has weird account depth
    WARN_IF(!bip44_hasReasonableAccount(path));

    // display prompt by default
    PROMPT_IF(true);
}

// policyForGetPublicKey implements policy test for address derivation.
security_policy_t policyForGetAddress(const bip44_path_t *path) {
    // deny if the path does not contain valid Fantom prefix
    DENY_IF(!bip44_hasValidFantomPrefix(path));

    // deny if the path does not contain valid chain / change index
    DENY_IF(!bip44_containsChangeType(path));

    // deny if the path does not contain valid address index
    DENY_IF(!bip44_containsAddress(path));

    // warn if the path has weird account depth
    WARN_IF(!bip44_hasReasonableAccount(path));

    // warn if the path has weird address depth
    WARN_IF(!bip44_hasReasonableAddress(path));

    // warn if the path has more than defined BIP44 fields
    WARN_IF(!bip44_containsMoreThanAddress(path));

    // display prompt by default
    PROMPT_IF(true);
}

// policyForSignTxInit implements policy test for new transaction being signed.
security_policy_t policyForSignTxInit() {
    // we ask user if it's ok to start a new transaction signing process
    PROMPT_IF(true);
}

// policyForSignTxOutputPath implements policy test for outgoing address path.
security_policy_t policyForSignTxOutputPath(const bip44_path_t *path) {
    // deny if the path does not contain valid Fantom prefix
    DENY_IF(!bip44_hasValidFantomPrefix(path));

    // deny if the path does not contain valid chain / change index
    DENY_IF(!bip44_containsChangeType(path));

    // deny if the path does not contain valid address index
    DENY_IF(!bip44_containsAddress(path));

    // warn if the path has weird account depth
    WARN_IF(!bip44_hasReasonableAccount(path));

    // warn if the path has weird address depth
    WARN_IF(!bip44_hasReasonableAddress(path));

    // warn if the path has more than defined BIP44 fields
    WARN_IF(!bip44_containsMoreThanAddress(path));

    // if path is given and is ok, we pass the step by default
    // no need to display outgoing address specified by valid path
    ALLOW_IF(true);
}

// policyForGetPublicKey implements policy test for outgoing address validation on tx signing process.
security_policy_t policyForSignTxOutputAddress(const uint8_t *addressBuffer, size_t addressSize) {
    // we always display output address on outgoing transaction if not specified by detailed path
    SHOW_IF(true);
}

// policyForGetPublicKey implements policy test for transaction fee validation on tx signing process.
security_policy_t policyForSignTxFee(uint64_t fee) {
    // we always inform user about transaction fee
    SHOW_IF(true);
}
