#include "common.h"
#include "handlers.h"
#include "ui_helpers.h"
#include "get_version.h"

// FLAG_DEVELOPMENT_VERSION defines the flag we use for marking
// development version of the application.
// See dec/cmd_get_version.md for the instruction handling details.
enum {
    FLAG_DEVELOPMENT_VERSION = 1
};

// handleGetVersion implements handler function for Get Version APDU instruction.
// For the handler responsibility and response format please check the documentation.
void handleGetVersion(
        uint8_t p1,
        uint8_t p2,
        uint8_t *wireDataBuffer MARK_UNUSED,
        size_t wireDataSize,
        bool isNewCall MARK_UNUSED
) {
    // Validate the version size and format.
    // The expected format is <single digit>.<single digit>.<single digit>
    // and total length of the version string is than 5 glyphs plus string terminator.
    ASSERT_IS_DIGIT(0);
    ASSERT_IS_DIGIT(2);
    ASSERT_IS_DIGIT(4);

    // Make sure the request has expected parameters.
    // We do not allow different value for security reasons.
    VALIDATE(p1 == 0, ERR_INVALID_PARAMETERS);
    VALIDATE(p2 == 0, ERR_INVALID_PARAMETERS);
    VALIDATE(wireDataSize == 0, ERR_INVALID_DATA);

    // construct the response structure
    // it will be used by i/o exchange as a source buffer
    // for response APDU
    struct {
        uint8_t major;
        uint8_t minor;
        uint8_t patch;
        uint8_t flags;
    } response = {
            // convert numeric glyphs to version values by ASCII math
            .major = APPVERSION[0] - '0',
            .minor = APPVERSION[2] - '0',
            .patch = APPVERSION[4] - '0',
            .flags = 0,
    };

#ifdef DEVEL
    // apply development flag to the response if needed
    response.flags |= FLAG_DEVELOPMENT_VERSION;
#endif

    // send the structure to host by i/o exchange helper
    io_send_buf(SUCCESS, (uint8_t * ) & response, sizeof(response));

    // go back to app idle state, the instruction has been served
    ui_idle();
}
