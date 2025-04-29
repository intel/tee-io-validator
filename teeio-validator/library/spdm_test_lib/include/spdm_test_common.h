/**
 *  Copyright Notice:
 *  Copyright 2025 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __SPDM_TEST_COMMON_H__
#define __SPDM_TEST_COMMON_H__

void spdm_test_lib_init_test_cases();
extern TEEIO_TEST_CASES m_spdm_test_case_funcs[];
extern ide_test_case_name_t m_spdm_test_case_names[];

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

#endif
