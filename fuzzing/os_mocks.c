#include <inttypes.h>
#include "os.h"
#include "cx.h"
#include "menu.h"
#include "os_io_seproxyhal.h"
#include "state.h"
#include "ux.h"

io_seph_app_t G_io_app;
void explicit_bzero(void *b, size_t len) {
    memset(b, 0, len);
}

void os_longjmp(unsigned int exception) {
    longjmp(try_context_get()->jmp_buf, exception);
}

try_context_t *current_context = NULL;
try_context_t *try_context_get(void) {
    return current_context;
}

try_context_t *try_context_set(try_context_t *ctx) {
    try_context_t *previous_ctx = current_context;
    current_context = ctx;
    return previous_ctx;
}

bolos_task_status_t os_sched_last_status(unsigned int task_idx) {return 1;};
unsigned int os_ux(bolos_ux_params_t * params) {return 0;};
void io_seproxyhal_init_ux(void) {};
unsigned int io_seph_is_status_sent (void) {return 0;};
bolos_bool_t os_perso_isonboarded(void) {return (bolos_bool_t)BOLOS_UX_OK;};
void io_seph_send(const unsigned char * buffer, unsigned short length) {};
unsigned short io_seph_recv ( unsigned char * buffer, unsigned short maxlength, unsigned int flags ) {return 0;};
void halt() { for(;;); };
bolos_bool_t os_global_pin_is_validated(void) {return (bolos_bool_t)BOLOS_UX_OK;};
cx_err_t cx_hash_no_throw(cx_hash_t *hash, uint32_t mode, const uint8_t *in, size_t len, uint8_t *out, size_t out_len) { 
    return 0;
}
size_t cx_hash_get_size(const cx_hash_t *ctx) { return 32;};

unsigned short io_exchange(unsigned char chan, unsigned short tx_len) 
{
    return 0;
}

void * pic(void * linked_addr) {
    return linked_addr;
}

void io_seproxyhal_display_default(const bagl_element_t * bagl) {
    if (bagl->text) {
        printf("[-] %s\n", bagl->text);
    }
}

void ui_idle(void) {
    currentIns = INS_NONE;
}

void os_sched_exit(bolos_task_status_t exit_code) {
    THROW(0xff);
}
cx_err_t cx_keccak_init_no_throw(cx_sha3_t *hash, size_t size) {return 0;};

void io_seproxyhal_io_heartbeat(void) {};
void io_seproxyhal_init_button(void) {};
void io_seproxyhal_button_push(button_push_callback_t button_push_callback, unsigned int new_button_mask) {
    // always validate
    button_push_callback(0x80000003,0);
}

void fake_validate() {
    G_io_seproxyhal_spi_buffer[0] = SEPROXYHAL_TAG_BUTTON_PUSH_EVENT;
    G_io_seproxyhal_spi_buffer[1] = 0;
    G_io_seproxyhal_spi_buffer[2] = 0;
    G_io_seproxyhal_spi_buffer[3] = 0x3<<1;
    io_event(0);
}

void io_send_buf(uint16_t code, const uint8_t * buf, size_t bufferSize) {
    printf("[out:] %x ", code);
    for(int i=0;i<bufferSize;i++) {
        printf("%02x", buf[i]);
    }
    printf("\n");
}
void io_seproxyhal_general_status(void) {};

cx_err_t cx_ecfp_init_private_key_no_throw(cx_curve_t             curve,
                                  const uint8_t *        rawkey,
                                  size_t                 key_len,
                                  cx_ecfp_private_key_t *pvkey) 
{
    return 0;
}

cx_err_t cx_ecfp_generate_pair_no_throw(cx_curve_t             curve,
                               cx_ecfp_public_key_t * pubkey,
                               cx_ecfp_private_key_t *privkey,
                               bool                   keepprivate) 
{
    return 0;
}

cx_err_t cx_ecdsa_sign_no_throw(const cx_ecfp_private_key_t *pvkey,
                       uint32_t                     mode,
                       cx_md_t                      hashID,
                       const uint8_t *              hash,
                       size_t                       hash_len,
                       uint8_t *                    sig,
                       size_t *                     sig_len,
                       uint32_t *                   info) 
{
    return 0;
}

void os_perso_derive_node_bip32 ( cx_curve_t curve, const unsigned int * path, unsigned int pathLength, unsigned char * privateKey, unsigned char * chain ) {}

void io_seproxyhal_se_reset() {
    THROW(0x99);
}
