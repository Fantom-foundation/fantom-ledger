#ifndef FANTOM_LEDGER_IO_H
#define FANTOM_LEDGER_IO_H

#include <os_io_seproxyhal.h>
#include <ux.h>
#include <stdint.h>

// The program is always waiting for either APDU i/o, or user input.
// We want to make sure the flow corresponds with the expected order
// of things.
typedef enum {
    // we expect host to send us an APDU message
    IO_EXPECT_IO,

    // we expect user to interact with displayed stuff
    IO_EXPECT_UI,

    // we are at a point where neither APDU, or user input is expected
    IO_EXPECT_NONE = 49,
} io_state_t;

// io_state keeps the state of the expected i/o exchange.
extern io_state_t io_state;

// CHECK_RESPONSE_SIZE implements buffer size validation to prevent buffer overflow.
void CHECK_RESPONSE_SIZE(unsigned int tx);

// _io_send_G_io_apdu_buffer is a helper function for sending response APDUs from button handlers.
// The IO_RETURN_AFTER_TX flag is set to receive next APDU message.
void _io_send_G_io_apdu_buffer(uint16_t code, uint16_t tx);

// io_send_buf implements sending APDU response from an internal buffer.
void io_send_buf(uint16_t code, const uint8_t *buffer, size_t bufferSize);

// ----------------------------------------------
// Everything below this point is Ledger magic.
// ----------------------------------------------

// io_seproxyhal_display implements display function proxy (see boilerplate).
void io_seproxyhal_display(const bagl_element_t *element);

// io_event handles events based on seproxyhal spi buffer content.
unsigned char io_event(unsigned char channel);

#if defined(TARGET_NANOS)
// timeout_callback_fn_t declares timer callback function header.
typedef void timeout_callback_fn_t(bool ux_allowed);

// nanos_set_timer sets a callback function to be triggered after specified time interval.
void nanos_set_timer(int ms, timeout_callback_fn_t* cb);

// nanos_clear_timer removes timeout callback from the timer.
void nanos_clear_timer();

#elif defined(TARGET_NANOX)
// On Nano X the new SDK UX_STEP_CB/UX_STEP_NOCB macros automatically push a confirm callback
// to G_ux.stack[].ticker_callback with timeout zero which causes our callback
// to be ignored in UX_TICKER_EVENT, so set_timer does not work on Nano X.
#endif

// device_is_unlocked implements device unlocked status check.
bool device_is_unlocked();

#endif //FANTOM_LEDGER_IO_H
