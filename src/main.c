/*******************************************************************************
* Fantom Ledger App
* (c) 2020 Fantom Foundation
*
* The software is distributed under MIT license. Please check the project
* repository to obtain a copy of the license.
********************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <os_io_seproxyhal.h>

// internal declarations
#include "errors.h"
#include "glyphs.h"
#include "main.h"

// ui_idle displays the main menu. Note that your app isn't required to use a
// menu as its idle screen; you can define your own completely custom screen.
void ui_idle(void) {
    currentInstruction = INS_NONE;
    // The first argument is the starting index within menu_main, and the last
    // argument is a preprocessor; I've never seen an app that uses either
    // argument.
#if defined(TARGET_NANOS)
    nanos_clear_timer();
    UX_MENU_DISPLAY(0, menu_main, NULL);
#elif defined(TARGET_NANOX)
    // reserve a display stack slot if none yet
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_idle_flow, NULL);
#else
    STATIC_ASSERT(false);
#endif
}

// fantom_main implements main application loop.
// It reads APDU messages and process them through corresponding message handlers.
static void fantom_main(void) {
    // exchange buffer pointers and flags
    volatile size_t rx = 0;
    volatile size_t tx = 0;
    volatile uint8_t flags = 0;

    // loop around and exchange APDU packets with the host
    // until EXCEPTION_IO_RESET is thrown.
    for (;;) {
        // next response code
        volatile unsigned short sw = 0;

        // Ledger SDK implements extension to exceptions handling.
        // We need to handle all thrown exceptions and convert them into
        // APDU response codes. EXCEPTION_IO_RESET is re-thrown
        // and captured by the main function.
        BEGIN_TRY
        {
            TRY
            {
                // remember the buffer position
                // ensure no race in CATCH_OTHER if io_exchange throws an error
                rx = tx;
                tx = 0;

                // make sure we were safely inside the buffer last time we parsed the data
                ASSERT((unsigned int) rx < sizeof(G_io_apdu_buffer));

                // exchange the last buffer
                rx = (unsigned int) io_exchange((uint8_t)(CHANNEL_APDU | flags), (uint16_t) rx);
                flags = 0;

                // make sure we do expect APDU
                ASSERT(io_state == IO_EXPECT_IO);
                io_state = IO_EXPECT_NONE;

                // did we receive an APDU? if not, trigger reset
                if (rx == 0) {
                    THROW(EXCEPTION_IO_RESET);
                }

                // make sure the device is ready to handle user input
                // we don't allow processing on locked device, even for non-interactive instructions
                VALIDATE(device_is_unlocked(), ERR_DEVICE_LOCKED);

                // read request header elements so we can validate the header
                // fields in the current processing context
                struct {
                    uint8_t cla;
                    uint8_t ins;
                    uint8_t p1;
                    uint8_t p2;
                    uint8_t lc;
                } *header = (void *) G_io_apdu_buffer;

                // validate that we received at least the header and we are not reading
                // remnants of a previous APDU conversation from the buffer, or some random memory content
                VALIDATE(rx >= SIZEOF(*header), ERR_BAD_REQUEST_HEADER);

                // validate that received data length corresponds with the lc value of the header
                VALIDATE(rx == header->lc + SIZEOF(*header), ERR_BAD_REQUEST_HEADER);

                // validate that the CLA is what we expect; the app uses fixed service class identifier
                VALIDATE(header->cla == CLA, ERR_UNKNOWN_CLA);

                // get the payload pointer (data payload starts just after the header)
                uint8_t *data = G_io_apdu_buffer + SIZEOF(*header);

                // find the handler we will be using to process the incoming instruction
                handler_fn_t *handlerFn = lookupHandler(header->ins);

                // validate that the instruction has been recognized and we do have a handler for it
                VALIDATE(handlerFn != NULL, ERR_UNKNOWN_INS);

                // validate the instruction context for the current call
                // do we start a new instruction, or is this a next step
                // in previously started multi-step instruction we are in?
                bool isNewIns = false;
                if (currentIns == INS_NONE) {
                    // reset the instruction state to be sure there is nothing left from previous one
                    os_memset(&insState, 0, SIZEOF(insState));

                    // remember what instruction we process now; start a new instruction
                    currentIns = header->ins;
                    isNewIns = true;
                } else {
                    // validate that the incoming instruction is the one we handle now
                    // we reject a new instruction in the middle of processing previous one
                    VALIDATE(header->ins == currentIns, ERR_INVALID_STATE);
                }

                // handlerFn is responsible for calling io_send either during its call
                // or during subsequent UI actions
                handlerFn(header->p1,
                          header->p2,
                          data,
                          header->lc,
                          isNewIns);

                // If io_exchange is the only call that blocks, how can we tell it
                // to wait for user input? The answer is a special flag, IO_ASYNC_REPLY. When
                // io_exchange is called with this flag, it blocks, but it doesn't send a
                // response; instead, it just waits for a new request.
                flags = IO_ASYNCH_REPLY;
            }
            CATCH(EXCEPTION_IO_RESET)
            {
                // re-throw the exception so we can capture
                // it in the main function and terminate the app gracefully.
                THROW(EXCEPTION_IO_RESET);
            }
            CATCH(ERR_ASSERT)
            {
                // Reset device on assertion exception
#ifdef RESET_ON_CRASH
                io_seproxyhal_se_reset();
#endif
            }
            CATCH_OTHER(e)
            {
                // Convert the exception captured here to a response code
                // sent in APDU. All error codes start with 0x6000; Success is 0x9000.
                // See errors.h for specific errors meaning.
            }
        }
        END_TRY;
    }
}

// ---------------------------------------------
// Everything below this point is Ledger magic.
// ---------------------------------------------
unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

// io_seproxyhal_display overrides the display entry point; nothing more to do
void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *) element);
}

// io_event processes I/O event interrupt processing
unsigned char io_event(unsigned char channel) {
    // can't have more than one tag in the reply, not supported yet.
    switch (G_io_seproxyhal_spi_buffer[0]) {
        case SEPROXYHAL_TAG_FINGER_EVENT:
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
            UX_DEFAULT_EVENT();
            break;

        case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
            UX_DISPLAYED_EVENT({});
            break;

        case SEPROXYHAL_TAG_TICKER_EVENT:
            UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
#ifndef TARGET_NANOX
                    if (UX_ALLOWED) {
                        if (ux_step_count) {
                            // prepare next screen
                            ux_step = (ux_step + 1) % ux_step_count;
                            // redisplay screen
                            UX_REDISPLAY();
                        }
                    }
#endif // TARGET_NANOX
            });
            break;

        default:
            UX_DEFAULT_EVENT();
            break;
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    // command has been processed, DO NOT reset the current APDU transport
    return 1;
}

// io_exchange_al processes I/O exchange
unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
        case CHANNEL_KEYBOARD:
            break;
            // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
        case CHANNEL_SPI:
            if (tx_len) {
                io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);
                if (channel & IO_RESET_AFTER_REPLIED) {
                    reset();
                }
                return 0; // nothing received from the master so far (it's a tx transaction)
            } else {
                return io_seproxyhal_spi_recv(G_io_apdu_buffer, sizeof(G_io_apdu_buffer), 0);
            }
        default:
            THROW(INVALID_PARAMETER);
    }
    return 0;
}

// app_exit passes the termination intent to system
static void app_exit(void) {
    BEGIN_TRY_L(exit)
    {
        TRY_L(exit)
        {
            os_sched_exit(-1);
        }
        FINALLY_L(exit)
        {
        }
    }
    END_TRY_L(exit);
}

// main implements the real application entry point.
__attribute__((section(".boot"))) int main(void) {
    // exit critical section
    __asm volatile("cpsie i");

    // ensure exception will work as planned
    os_boot();

    for (;;) {
        UX_INIT();

        BEGIN_TRY
        {
            TRY
            {
                io_seproxyhal_init();

                USB_power(0);
                USB_power(1);

                // render idle user interface
                ui_idle();

#if defined(HAVE_BLE)
                BLE_power(0, NULL);
                BLE_power(1, "Nano X ADA");
#endif

                // set initial state and start the main loop
                io_state = IO_EXPECT_IO;
                fantom_main();
            }
            CATCH(EXCEPTION_IO_RESET)
            {
                // reset IO and UX before continuing
                continue;
            }
            CATCH_ALL
            {
                break;
            }
            FINALLY
            {
            }
        }
        END_TRY;
    }
    app_exit();
    return 0;
}
