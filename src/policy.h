#ifndef FANTOM_LEDGER_POLICY_H
#define FANTOM_LEDGER_POLICY_H

#include "errors.h"

// security_policy_t defines levels of security policy enforcement
typedef enum {
    // POLICY_DENY specifies an action that is denied by security policy
    POLICY_DENY = 1,

    // POLICY_PROMPT specifies an action that is allowed
    // by security policy, but only if confirmed by user
    POLICY_PROMPT = 2,

    // POLICY_WARN specifies an action that is allowed
    // by security policy, but user is warned on screen
    POLICY_WARN = 3,

    // POLICY_ALLOW specifies an action that is allowed
    // by security policy, user may not be notified about
    // this action since it's considered safe
    POLICY_ALLOW = 4,
} security_policy_t;

// policyForGetPublicKey implements policy test for public key extraction.
security_policy_t policyForGetPublicKey();

// policyForGetPublicKey implements policy test for address derivation.
security_policy_t policyForGetAddress();

// policyForGetPublicKey implements policy test for outgoing address validation on tx signing process.
security_policy_t policyForSignTxOutputAddress(const uint8_t *addressBuffer, size_t addressSize);

// policyForGetPublicKey implements policy test for transaction fee validation on tx signing process.
security_policy_t policyForSignTxFee(uint64_t fee);

// ENSURE_NOT_DENIED tests given policy for denied status and throws
// an exception if the action has been denied.
static inline void ENSURE_NOT_DENIED(security_policy_t policy) {
    if (policy == POLICY_DENY) {
        THROW(ERR_REJECTED_BY_POLICY);
    }
}

#endif //FANTOM_LEDGER_POLICY_H
