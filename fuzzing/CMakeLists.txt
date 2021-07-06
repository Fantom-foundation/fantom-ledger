cmake_minimum_required(VERSION 3.10)

project(fantom_fuzzer C)

set(CMAKE_C_STANDARD 11)

set (SDK_PATH $ENV{BOLOS_SDK})

include_directories(. ../src)
include_directories(
	${SDK_PATH}/include/
	${SDK_PATH}/lib_cxng/include/
	${SDK_PATH}/lib_cxng/src/
	${SDK_PATH}/lib_ux/include/
	)

add_compile_options(-g -ggdb2)
if (FUZZ)
add_compile_options(-O3)
add_compile_options(-fsanitize=fuzzer,address)
add_link_options(-fsanitize=fuzzer,address)
add_compile_definitions(HAVE_PRINTF PRINTF=printf)
endif()

add_compile_definitions(
		FUZZING
        OS_IO_SEPROXYHAL
        IO_SEPROXYHAL_BUFFER_SIZE_B=300
		RESET_ON_CRASH
        IO_HID_EP_LENGTH=64
        HAVE_UX_FLOW
        HAVE_BAGL
        APPVERSION="0.0.0"
)

# hack so that core_sc000.h compiles
if (WIN32)
add_compile_definitions(__GNUC__)
endif()

add_compile_definitions(
    HAVE_AES
    HAVE_DES
    HAVE_ECC
    HAVE_ECC_WEIERSTRASS
    HAVE_SECP256K1_CURVE
    HAVE_SECP256R1_CURVE
    HAVE_ECC_TWISTED_EDWARDS
    HAVE_ED25519_CURVE
    HAVE_ECDSA
    HAVE_EDDSA
    HAVE_HASH
    HAVE_SHA256
    HAVE_SHA3
    HAVE_BLAKE2
)

set (SDK_SRCS
	# ${SDK_PATH}/src/syscalls.c
)

set(LIBUX_PATH ${SDK_PATH}/lib_ux)

set (LIBUX_SRCS
	${LIBUX_PATH}/src/ux_flow_engine.c
	${LIBUX_PATH}/src/ux_layout_bb.c
	${LIBUX_PATH}/src/ux_layout_bn.c
	${LIBUX_PATH}/src/ux_layout_bnn.c
	${LIBUX_PATH}/src/ux_layout_bnnn.c
	${LIBUX_PATH}/src/ux_layout_nn.c
	${LIBUX_PATH}/src/ux_layout_paging.c
	${LIBUX_PATH}/src/ux_layout_paging_compute.c
	${LIBUX_PATH}/src/ux_layout_pb.c
	${LIBUX_PATH}/src/ux_layout_pbb.c
	${LIBUX_PATH}/src/ux_layout_pn.c
	${LIBUX_PATH}/src/ux_layout_pnn.c
	${LIBUX_PATH}/src/ux_layout_utils.c
	${LIBUX_PATH}/src/ux_stack.c
)

set(SOURCES
        fuzztest.c
		os_mocks.c

		../src/address_utils.c
		../src/assert.c
		../src/bip44.c
		../src/derive_key.c
		../src/get_tx_sign.c
		../src/glyphs.c
		../src/io.c
		../src/menu.c
		../src/policy.c
		../src/rlp_utils.c
		../src/state.c
		../src/transaction.c
		../src/tx_stream.c
		../src/uint256.c
		../src/ui_basic_flows.c
		../src/ui_helpers.c
		${LIBUX_SRCS} ${SDK_SRCS})

add_executable(fuzzer ${SOURCES})
add_executable(fuzzer_coverage ${SOURCES})

target_compile_options(fuzzer_coverage PRIVATE -fprofile-instr-generate -fcoverage-mapping)

target_link_options(fuzzer PRIVATE)
target_link_options(fuzzer_coverage PRIVATE)