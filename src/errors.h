#ifndef H_FANTOM_LAP_ERRORS
#define H_FANTOM_LAP_ERRORS

enum {
    // Instruction processed
    SUCCESS                        = 0x9000,

    // Bad request header
    ERR_BAD_REQUEST_HEADER         = 0x6E01,

    // Unknown CLA
    ERR_UNKNOWN_CLA                = 0x6E02,

    // Unknown INS
    ERR_UNKNOWN_INS                = 0x6E03,

    // Request is not valid in the current context
    ERR_INVALID_STATE              = 0x6E04,

    // Request contains invalid P1, P2, or Data
    ERR_INVALID_DATA               = 0x6E05,

    // Action has been rejected by user
    ERR_REJECTED_BY_USER           = 0x6E06,

    // Action rejected by security policy
    ERR_REJECTED_BY_POLICY         = 0x6E07,

    // Device is locked
    ERR_DEVICE_LOCKED              = 0x6E08,
};

#endif
