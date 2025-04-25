/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __SPDM_TEST_COMMON_H__
#define __SPDM_TEST_COMMON_H__

#define TEEIO_SPDM_TEST_SCRATCH_BUFFER_SIZE 0x1000
typedef struct {
    void *spdm_context;
    /* test case specific scratch buffer between setup and case, avoid writable global variable */
    uint8_t test_scratch_buffer[TEEIO_SPDM_TEST_SCRATCH_BUFFER_SIZE];
    uint32_t test_scratch_buffer_size;
} teeio_spdm_test_context_t;

void* spdm_test_get_spdm_context_from_test_context(void *test_context);

// SPDM Test Config
// default config
bool spdm_test_config_default_enable(void *test_context);
bool spdm_test_config_default_disable(void *test_context);
bool spdm_test_config_default_support(void *test_context);
bool spdm_test_config_default_check(void *test_context);

//
// SPDM Test Group
//
bool spdm_test_group_setup(void *test_context);
bool spdm_test_group_teardown(void *test_context);

//
// SPDM Test Case
//
// Version
bool spdm_test_version_1_setup(void *test_context);
bool spdm_test_version_1_run(void *test_context);
bool spdm_test_version_1_teardown(void *test_context);

// Capabilities
bool spdm_test_capabilities_1_setup(void *test_context);
bool spdm_test_capabilities_1_run(void *test_context);
bool spdm_test_capabilities_1_teardown(void *test_context);
bool spdm_test_capabilities_2_setup(void *test_context);
bool spdm_test_capabilities_2_run(void *test_context);
bool spdm_test_capabilities_2_teardown(void *test_context);
bool spdm_test_capabilities_3_setup(void *test_context);
bool spdm_test_capabilities_3_run(void *test_context);
bool spdm_test_capabilities_3_teardown(void *test_context);
bool spdm_test_capabilities_4_setup(void *test_context);
bool spdm_test_capabilities_4_run(void *test_context);
bool spdm_test_capabilities_4_teardown(void *test_context);
bool spdm_test_capabilities_5_setup(void *test_context);
bool spdm_test_capabilities_5_run(void *test_context);
bool spdm_test_capabilities_5_teardown(void *test_context);
bool spdm_test_capabilities_6_setup(void *test_context);
bool spdm_test_capabilities_6_run(void *test_context);
bool spdm_test_capabilities_6_teardown(void *test_context);

// Algorithms
bool spdm_test_algorithms_1_setup(void *test_context);
bool spdm_test_algorithms_1_run(void *test_context);
bool spdm_test_algorithms_1_teardown(void *test_context);
bool spdm_test_algorithms_2_setup(void *test_context);
bool spdm_test_algorithms_2_run(void *test_context);
bool spdm_test_algorithms_2_teardown(void *test_context);
bool spdm_test_algorithms_3_setup(void *test_context);
bool spdm_test_algorithms_3_run(void *test_context);
bool spdm_test_algorithms_3_teardown(void *test_context);
bool spdm_test_algorithms_4_setup(void *test_context);
bool spdm_test_algorithms_4_run(void *test_context);
bool spdm_test_algorithms_4_teardown(void *test_context);
bool spdm_test_algorithms_5_setup(void *test_context);
bool spdm_test_algorithms_5_run(void *test_context);
bool spdm_test_algorithms_5_teardown(void *test_context);
bool spdm_test_algorithms_6_setup(void *test_context);
bool spdm_test_algorithms_6_run(void *test_context);
bool spdm_test_algorithms_6_teardown(void *test_context);
bool spdm_test_algorithms_7_setup(void *test_context);
bool spdm_test_algorithms_7_run(void *test_context);
bool spdm_test_algorithms_7_teardown(void *test_context);

// Certificate
bool spdm_test_certificate_1_setup(void *test_context);
bool spdm_test_certificate_1_run(void *test_context);
bool spdm_test_certificate_1_teardown(void *test_context);
bool spdm_test_certificate_2_setup(void *test_context);
bool spdm_test_certificate_2_run(void *test_context);
bool spdm_test_certificate_2_teardown(void *test_context);
bool spdm_test_certificate_3_setup(void *test_context);
bool spdm_test_certificate_3_run(void *test_context);
bool spdm_test_certificate_3_teardown(void *test_context);
bool spdm_test_certificate_4_setup(void *test_context);
bool spdm_test_certificate_4_run(void *test_context);
bool spdm_test_certificate_4_teardown(void *test_context);

// Measurements
bool spdm_test_measurements_1_setup(void *test_context);
bool spdm_test_measurements_1_run(void *test_context);
bool spdm_test_measurements_1_teardown(void *test_context);
bool spdm_test_measurements_2_setup(void *test_context);
bool spdm_test_measurements_2_run(void *test_context);
bool spdm_test_measurements_2_teardown(void *test_context);
bool spdm_test_measurements_3_setup(void *test_context);
bool spdm_test_measurements_3_run(void *test_context);
bool spdm_test_measurements_3_teardown(void *test_context);
bool spdm_test_measurements_4_setup(void *test_context);
bool spdm_test_measurements_4_run(void *test_context);
bool spdm_test_measurements_4_teardown(void *test_context);
bool spdm_test_measurements_5_setup(void *test_context);
bool spdm_test_measurements_5_run(void *test_context);
bool spdm_test_measurements_5_teardown(void *test_context);
bool spdm_test_measurements_6_setup(void *test_context);
bool spdm_test_measurements_6_run(void *test_context);
bool spdm_test_measurements_6_teardown(void *test_context);
bool spdm_test_measurements_7_setup(void *test_context);
bool spdm_test_measurements_7_run(void *test_context);
bool spdm_test_measurements_7_teardown(void *test_context);
bool spdm_test_measurements_8_setup(void *test_context);
bool spdm_test_measurements_8_run(void *test_context);
bool spdm_test_measurements_8_teardown(void *test_context);
bool spdm_test_measurements_9_setup(void *test_context);
bool spdm_test_measurements_9_run(void *test_context);
bool spdm_test_measurements_9_teardown(void *test_context);
bool spdm_test_measurements_10_setup(void *test_context);
bool spdm_test_measurements_10_run(void *test_context);
bool spdm_test_measurements_10_teardown(void *test_context);

// KeyExchange
bool spdm_test_key_exchange_1_setup(void *test_context);
bool spdm_test_key_exchange_1_run(void *test_context);
bool spdm_test_key_exchange_1_teardown(void *test_context);
bool spdm_test_key_exchange_2_setup(void *test_context);
bool spdm_test_key_exchange_2_run(void *test_context);
bool spdm_test_key_exchange_2_teardown(void *test_context);
bool spdm_test_key_exchange_3_setup(void *test_context);
bool spdm_test_key_exchange_3_run(void *test_context);
bool spdm_test_key_exchange_3_teardown(void *test_context);
bool spdm_test_key_exchange_4_setup(void *test_context);
bool spdm_test_key_exchange_4_run(void *test_context);
bool spdm_test_key_exchange_4_teardown(void *test_context);
bool spdm_test_key_exchange_5_setup(void *test_context);
bool spdm_test_key_exchange_5_run(void *test_context);
bool spdm_test_key_exchange_5_teardown(void *test_context);
bool spdm_test_key_exchange_6_setup(void *test_context);
bool spdm_test_key_exchange_6_run(void *test_context);
bool spdm_test_key_exchange_6_teardown(void *test_context);
bool spdm_test_key_exchange_7_setup(void *test_context);
bool spdm_test_key_exchange_7_run(void *test_context);
bool spdm_test_key_exchange_7_teardown(void *test_context);
bool spdm_test_key_exchange_8_setup(void *test_context);
bool spdm_test_key_exchange_8_run(void *test_context);
bool spdm_test_key_exchange_8_teardown(void *test_context);

// Finish
bool spdm_test_finish_1_setup(void *test_context);
bool spdm_test_finish_1_run(void *test_context);
bool spdm_test_finish_1_teardown(void *test_context);
bool spdm_test_finish_2_setup(void *test_context);
bool spdm_test_finish_2_run(void *test_context);
bool spdm_test_finish_2_teardown(void *test_context);
bool spdm_test_finish_3_setup(void *test_context);
bool spdm_test_finish_3_run(void *test_context);
bool spdm_test_finish_3_teardown(void *test_context);
bool spdm_test_finish_4_setup(void *test_context);
bool spdm_test_finish_4_run(void *test_context);
bool spdm_test_finish_4_teardown(void *test_context);
bool spdm_test_finish_5_setup(void *test_context);
bool spdm_test_finish_5_run(void *test_context);
bool spdm_test_finish_5_teardown(void *test_context);
bool spdm_test_finish_6_setup(void *test_context);
bool spdm_test_finish_6_run(void *test_context);
bool spdm_test_finish_6_teardown(void *test_context);
bool spdm_test_finish_7_setup(void *test_context);
bool spdm_test_finish_7_run(void *test_context);
bool spdm_test_finish_7_teardown(void *test_context);
bool spdm_test_finish_8_setup(void *test_context);
bool spdm_test_finish_8_run(void *test_context);
bool spdm_test_finish_8_teardown(void *test_context);
bool spdm_test_finish_9_setup(void *test_context);
bool spdm_test_finish_9_run(void *test_context);
bool spdm_test_finish_9_teardown(void *test_context);
bool spdm_test_finish_10_setup(void *test_context);
bool spdm_test_finish_10_run(void *test_context);
bool spdm_test_finish_10_teardown(void *test_context);
bool spdm_test_finish_11_setup(void *test_context);
bool spdm_test_finish_11_run(void *test_context);
bool spdm_test_finish_11_teardown(void *test_context);

// EndSession
bool spdm_test_end_session_1_setup(void *test_context);
bool spdm_test_end_session_1_run(void *test_context);
bool spdm_test_end_session_1_teardown(void *test_context);
bool spdm_test_end_session_2_setup(void *test_context);
bool spdm_test_end_session_2_run(void *test_context);
bool spdm_test_end_session_2_teardown(void *test_context);
bool spdm_test_end_session_3_setup(void *test_context);
bool spdm_test_end_session_3_run(void *test_context);
bool spdm_test_end_session_3_teardown(void *test_context);
bool spdm_test_end_session_4_setup(void *test_context);
bool spdm_test_end_session_4_run(void *test_context);
bool spdm_test_end_session_4_teardown(void *test_context);

#endif
