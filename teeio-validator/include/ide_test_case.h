/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __IDE_TEST_CASE_H__
#define __IDE_TEST_CASE_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/param.h>

#pragma pack(1)
typedef struct {
    pci_ide_km_header_t header;
    uint8_t reserved[2];
    uint8_t stream_id;
    uint8_t reserved2;
    uint8_t key_sub_stream;
    uint8_t port_index;
    pci_ide_km_aes_256_gcm_key_buffer_t key_buffer;
} test_pci_ide_km_key_prog_t;
#pragma pack()

// Query Case 1.1
bool test_query_1_setup(void *test_context);
bool test_query_1_run(void *test_context);
bool test_query_1_teardown(void *test_context);

// Query Case 1.2
bool test_query_2_setup(void *test_context);
bool test_query_2_run(void *test_context);
bool test_query_2_teardown(void *test_context);

// KSetGo Case 3.1
bool test_ksetgo_1_setup(void *test_context);
bool test_ksetgo_1_run(void *test_context);
bool test_ksetgo_1_teardown(void *test_context);

// KSetGo Case 3.2
bool test_ksetgo_2_setup(void *test_context);
bool test_ksetgo_2_run(void *test_context);
bool test_ksetgo_2_teardown(void *test_context);

// KSetGo Case 3.3
bool test_ksetgo_3_setup(void *test_context);
bool test_ksetgo_3_run(void *test_context);
bool test_ksetgo_3_teardown(void *test_context);

// KSetGo Case 3.4
bool test_ksetgo_4_setup(void *test_context);
bool test_ksetgo_4_run(void *test_context);
bool test_ksetgo_4_teardown(void *test_context);

// KeyProg Case 2.1
bool test_keyprog_1_setup(void *test_context);
bool test_keyprog_1_run(void *test_context);
bool test_keyprog_1_teardown(void *test_context);

// KeyProg Case 2.2
bool test_keyprog_2_setup(void *test_context);
bool test_keyprog_2_run(void *test_context);
bool test_keyprog_2_teardown(void *test_context);

// KeyProg Case 2.3
bool test_keyprog_3_setup(void *test_context);
bool test_keyprog_3_run(void *test_context);
bool test_keyprog_3_teardown(void *test_context);

// KeyProg Case 2.4
bool test_keyprog_4_setup(void *test_context);
bool test_keyprog_4_run(void *test_context);
bool test_keyprog_4_teardown(void *test_context);

// KeyProg Case 2.5
bool test_keyprog_5_setup(void *test_context);
bool test_keyprog_5_run(void *test_context);
bool test_keyprog_5_teardown(void *test_context);

// KeyProg Case 2.6
bool test_keyprog_6_setup(void *test_context);
bool test_keyprog_6_run(void *test_context);
bool test_keyprog_6_teardown(void *test_context);

// KSetStop Case 4.1
bool test_ksetstop_1_setup(void *test_context);
bool test_ksetstop_1_run(void *test_context);
bool test_ksetstop_1_teardown(void *test_context);

// KSetStop Case 4.2
bool test_ksetstop_2_setup(void *test_context);
bool test_ksetstop_2_run(void *test_context);
bool test_ksetstop_2_teardown(void *test_context);

// KSetStop Case 4.3
bool test_ksetstop_3_setup(void *test_context);
bool test_ksetstop_3_run(void *test_context);
bool test_ksetstop_3_teardown(void *test_context);

// KSetStop Case 4.4
bool test_ksetstop_4_setup(void *test_context);
bool test_ksetstop_4_run(void *test_context);
bool test_ksetstop_4_teardown(void *test_context);

// Full case
bool test_full_1_setup(void *test_context);
bool test_full_1_run(void *test_context);
bool test_full_1_teardown(void *test_context);

// Full case - KeyRefresh
bool test_full_keyrefresh_setup(void *test_context);
bool test_full_keyrefresh_run(void *test_context);
bool test_full_keyrefresh_teardown(void *test_context);


#endif