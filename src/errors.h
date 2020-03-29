#ifndef H_FANTOM_LAP_ERRORS
#define H_FANTOM_LAP_ERRORS

enum {
    // Instruction processed
    SUCCESS = 0x9000,

    // Error codes below this value are ok to be passed to host.
    // We don't want to pass unknown error though, so any other error code will
    // be handled by resetting the device.
    // Note that any error will reset multi-step instruction processing.
    _ERR_PASS_FROM = 0x6E00,

    // Bad request header.
    ERR_BAD_REQUEST_HEADER = 0x6E01,

    // Unknown service class CLA received.
    ERR_UNKNOWN_CLA = 0x6E02,

    // Unknown instruction arrived.
    ERR_UNKNOWN_INS = 0x6E03,

    // Request is not valid in the current context.
    ERR_INVALID_STATE = 0x6E04,

    // Request contains invalid parameters P1, P2, or Lc.
    ERR_INVALID_PARAMETERS = 0x6E05,

    // Request contains invalid payload structure, or content.
    ERR_INVALID_DATA = 0x6E06,

    // Action has been rejected by user.
    ERR_REJECTED_BY_USER = 0x6E07,

    // Action rejected by security policy.
    ERR_REJECTED_BY_POLICY = 0x6E08,

    // Device is locked.
    ERR_DEVICE_LOCKED = 0x6E09,

    // Error codes above this value are ok to be passed to host.
    // Any other error will trigger SEPROXYHAL reset.
    _ERR_PASS_TO = 0x6E10
};

#endif
