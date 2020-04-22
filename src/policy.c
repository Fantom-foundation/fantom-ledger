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
security_policy_t policyForGetAddress(const bip44_path_t *path, const bool isShowAddress) {
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

    // warn if the path has more fields than defined by BIP44 standard
    WARN_IF(bip44_containsMoreThanAddress(path));

    // display prompt by default
    if (isShowAddress) {
        PROMPT_IF(true);
    } else {
        ALLOW_IF(true);
    }
}

// policyForSignTxInit implements policy test for new transaction being signed.
security_policy_t policyForSignTxInit(const bip44_path_t *path) {
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

    // warn if the path has more fields than defined by BIP44 standard
    WARN_IF(bip44_containsMoreThanAddress(path));

    // we ask user if it's ok to start a new transaction signing process
    PROMPT_IF(true);
}

// policyForSignTxFinalize implements policy test for transaction signature being provided.
security_policy_t policyForSignTxFinalize() {
    // we ask user if it's ok to sign the transaction
    PROMPT_IF(true);
}
