/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __CXL_TSP_TEST_COMMON_H__
#define __CXL_TSP_TEST_COMMON_H__

//
// CXL_TSP Test Cases
//
// GetVersion
bool cxl_tsp_test_get_version_setup(void *test_context);
bool cxl_tsp_test_get_version_run(void *test_context);
bool cxl_tsp_test_get_version_teardown(void *test_context);

// GetCaps
bool cxl_tsp_test_get_caps_setup(void *test_context);
bool cxl_tsp_test_get_caps_run(void *test_context);
bool cxl_tsp_test_get_caps_teardown(void *test_context);

// SetConfiguration
bool cxl_tsp_test_set_configuration_setup(void *test_context);
bool cxl_tsp_test_set_configuration_run(void *test_context);
bool cxl_tsp_test_set_configuration_teardown(void *test_context);

// GetConfiguration
bool cxl_tsp_test_get_configuration_setup(void *test_context);
bool cxl_tsp_test_get_configuration_run(void *test_context);
bool cxl_tsp_test_get_configuration_teardown(void *test_context);

// GetConfigurationReport
bool cxl_tsp_test_get_configuration_report_setup(void *test_context);
bool cxl_tsp_test_get_configuration_report_run(void *test_context);
bool cxl_tsp_test_get_configuration_report_teardown(void *test_context);

// LockConfiguration
bool cxl_tsp_test_lock_configuration_1_setup(void *test_context);
bool cxl_tsp_test_lock_configuration_1_run(void *test_context);
bool cxl_tsp_test_lock_configuration_1_teardown(void *test_context);

bool cxl_tsp_test_lock_configuration_2_setup(void *test_context);
bool cxl_tsp_test_lock_configuration_2_run(void *test_context);
bool cxl_tsp_test_lock_configuration_2_teardown(void *test_context);

//
// CXL_TSP Test Config
//

bool cxl_tsp_test_config_common_enable(void *test_context);
bool cxl_tsp_test_config_common_disable(void *test_context);
bool cxl_tsp_test_config_common_support(void *test_context);
bool cxl_tsp_test_config_common_check(void *test_context);

//
// CXL_TSP Test Group
//

// test group
bool cxl_tsp_test_group_setup(void *test_context);
bool cxl_tsp_test_group_teardown(void *test_context);

#endif
