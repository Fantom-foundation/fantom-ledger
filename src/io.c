#include <os_io_seproxyhal.h>
#include "conf.h"
#include "utils.h"
#include "io.h"
#include "assert.h"
#include "errors.h"

// io_state keeps the state of the expected i/o exchange.
io_state_t io_state;

// CHECK_RESPONSE_SIZE checks is response is within the size of the buffer
// to prevent unwanted overflows
void CHECK_RESPONSE_SIZE(unsigned int tx) {
    // keep 2 bytes of extra space for sending the uint16 response code
    ASSERT(tx + 2u < sizeof(G_io_apdu_buffer));
}

// _io_send_G_io_apdu_buffer is a helper function for sending response APDUs from
// button handlers. Note that the IO_RETURN_AFTER_TX flag is set. 'tx' is the
// conventional name for the size of the response APDU, i.e. the write-offset
// within G_io_apdu_buffer.
void _io_send_G_io_apdu_buffer(uint16_t code, uint16_t tx) {
    // validate the buffer size
    CHECK_RESPONSE_SIZE(tx);

    // this is why we keep those 2 extra bytes in the check above
    G_io_apdu_buffer[tx++] = code >> 8;
    G_io_apdu_buffer[tx++] = code & 0xFF;

    // do the actual sending
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);

    // we sent a response to host and now we expect it to send us new APDU
    io_state = IO_EXPECT_IO;
}

#ifndef FUZZING
// io_send_buf implements sending from an internal buffer by copying the data
// to G_io_apdu_buffer and than calling the i/o exchange
void io_send_buf(uint16_t code, const uint8_t *buffer, size_t bufferSize) {
    // make sure our buffer if on the safe side
    CHECK_RESPONSE_SIZE(bufferSize);

    // copy data and do the i/o exchange
    memcpy(G_io_apdu_buffer, buffer, bufferSize);
    _io_send_G_io_apdu_buffer(code, bufferSize);
}
#endif

// --------------------------------------------
// Everything below this point is Ledger magic.
// --------------------------------------------

// io_seproxyhal_display implements display function proxy.
void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *) element);
}

// G_io_seproxyhal_spi_buffer defines the buffer
unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

// io_event handles events based on seproxyhal spi buffer content.
unsigned char io_event(unsigned char channel MARK_UNUSED) {
    // can't have more than one tag in the reply, not supported yet.
    switch (G_io_seproxyhal_spi_buffer[0]) {
        case SEPROXYHAL_TAG_FINGER_EVENT:
            // this app is not supposed to work with Blue so we trigger reset
            ASSERT(false);
            UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
            break;

        case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
            UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
            break;

        case SEPROXYHAL_TAG_STATUS_EVENT:
            if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID &&
                !(U4BE(G_io_seproxyhal_spi_buffer, 3) &
                  SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
                THROW(EXCEPTION_IO_RESET);
            }
            // we fall through to the default event handling

        default:
            UX_DEFAULT_EVENT();
            break;

        case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
            UX_DISPLAYED_EVENT({});
            break;

        case SEPROXYHAL_TAG_TICKER_EVENT:
            UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
                    // the ticker is handled by a macro defined above
                    // Disabled for Nano X due to new SDK ignoring this callback on UX_TICKER_EVENT.
                    // HANDLE_UX_TICKER_EVENT(UX_ALLOWED);
            });
            break;
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    // command has been processed, DO NOT reset the current APDU transport
    return 1;
}

// io_exchange_al implements i/o exchange on SPI channels.
unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
        case CHANNEL_KEYBOARD:
            break;

        case CHANNEL_SPI:
            // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
            if (tx_len) {
                io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);
                if (channel & IO_RESET_AFTER_REPLIED) {
                    reset();
                }

                // nothing received from the master so far (it's a tx transaction)
                return 0;
            } else {
                // receive data to the buffer
                return io_seproxyhal_spi_recv(G_io_apdu_buffer, sizeof(G_io_apdu_buffer), 0);
            }
        default:
            THROW(INVALID_PARAMETER);
    }
    return 0;
}

// The app is designed for specific Ledger API level.
STATIC_ASSERT(CX_APILEVEL >= API_LEVEL_MIN || CX_APILEVEL <= API_LEVEL_MAX, "bad api level");

// PIN_VERIFIED specifies the expected value of global PIN validation status in validated state.
// This way of checking for device unlocked status seems to work for the current API level.
static const unsigned PIN_VERIFIED = BOLOS_UX_OK;

// device_is_unlocked implements device unlocked status verification.
// We do not allow any instruction processing on a locked device (even if it would be no-interaction one)
bool device_is_unlocked() {
    return os_global_pin_is_validated() == PIN_VERIFIED;
}
