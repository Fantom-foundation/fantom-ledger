/*******************************************************************************
* Fantom Ledger App
* (c) 2020 Fantom Foundation
*
* The software is distributed under MIT license. Please check the project
* repository to obtain a copy of the license.
********************************************************************************/
// system libs
#include <stdint.h>
#include <stdbool.h>
#include <os_io_seproxyhal.h>
#include <os.h>

// internal declarations
#include "conf.h"
#include "handlers.h"
#include "errors.h"
#include "assert.h"
#include "menu.h"
#include "io.h"
#include "main.h"

// The app is designed for specific Ledger API level.
STATIC_ASSERT(CX_APILEVEL >= API_LEVEL_MIN || CX_APILEVEL <= API_LEVEL_MAX, "bad api level");

// ui_idle displays the main menu. Note that your app isn't required to use a
// menu as its idle screen; you can define your own completely custom screen.
void ui_idle(void) {
    // no instruction is being processed; the last one called idle
    currentIns = INS_NONE;

    // we support only Nano S and Nano X devices
#if defined(TARGET_NANOS) || defined(TARGET_NANOX)
    // reserve a display stack slot if none yet
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }

    // initiate the idle flow
    ux_flow_init(0, ux_idle_flow, NULL);
#else
    // unknown device?
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
        // Ledger SDK implements try/ catch system
        // https://ledger.readthedocs.io/en/latest/userspace/syscalls.html
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
                // we don't process instructions on locked device, not even non-interactive
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

                // find the handler we will be using to process the incoming instruction (handlers.h)
                handler_fn_t *handlerFn = getHandler(header->ins);

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
                // re-throw the exception so we can capture it in the main function.
                THROW(EXCEPTION_IO_RESET);
            }
            CATCH(ERR_ASSERT)
            {
                // reset device on assertion exception
                // this should be _enabled_ for production build
                #ifdef RESET_ON_CRASH
                io_seproxyhal_se_reset();
                #endif
            }
            CATCH_OTHER(e)
            {
                // Pass valid error codes to host and reset the state to idle.
                // See errors.h for specific errors meaning.
                if (e > _ERR_PASS_FROM && e < _ERR_PASS_TO) {
                    // pass the error code
                    io_send_buf(e, NULL, 0);

                    // io_exchange on the start of the loop will block without sending response
                    flags = IO_ASYNCH_REPLY;
                    ui_idle();
                } else {
                    // unknown error happened; reset the device
                    #ifdef RESET_ON_CRASH
                    io_seproxyhal_se_reset();
                    #endif
                }
            }
        }
        END_TRY;
    }
}

// ---------------------------------------------
// Everything below this point is Ledger magic.
// ---------------------------------------------

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

    for (;;) {
        // ensure exception will work as planned
        UX_INIT();
        os_boot();

        // capture exceptions from main loop
        BEGIN_TRY
        {
            TRY
            {
                io_seproxyhal_init();

                #if defined(TARGET_NANOX)
                // grab the current plane mode setting
                G_io_app.plane_mode = os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
                #endif

                USB_power(0);
                USB_power(1);

                // setup idle user interface
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
                // Maybe we should terminate the TRY / CATCH macro first to be clean?
                // I guess they just leave it be since the app is terminating anyway.
                // https://ledger.readthedocs.io/en/latest/userspace/syscalls.html
                // If using a return, break, continue or goto statement that jumps
                // out of the TRY clause you MUST manually close it, or it could lead
                // to a crash of the application in a later THROW.
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
