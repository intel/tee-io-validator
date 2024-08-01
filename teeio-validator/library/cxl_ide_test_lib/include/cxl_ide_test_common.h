/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __CXL_IDE_TEST_COMMON_H__
#define __CXL_IDE_TEST_COMMON_H__

//
// CXL_IDE Test Cases
//
// Full case
bool cxl_ide_test_full_ide_stream_setup(void *test_context);
bool cxl_ide_test_full_ide_stream_run(void *test_context);
bool cxl_ide_test_full_ide_stream_teardown(void *test_context);

//
// CXL_IDE Test Config
//

// default config
bool cxl_ide_test_config_default_enable(void *test_context);
bool cxl_ide_test_config_default_disable(void *test_context);
bool cxl_ide_test_config_default_support(void *test_context);
bool cxl_ide_test_config_default_check(void *test_context);

// skid mode
bool cxl_ide_test_config_skid_enable(void *test_context);
bool cxl_ide_test_config_skid_disable(void *test_context);
bool cxl_ide_test_config_skid_support(void *test_context);
bool cxl_ide_test_config_skid_check(void *test_context);

// skid mode
bool cxl_ide_test_config_containment_enable(void *test_context);
bool cxl_ide_test_config_containment_disable(void *test_context);
bool cxl_ide_test_config_containment_support(void *test_context);
bool cxl_ide_test_config_containment_check(void *test_context);

//
// CXL_IDE Test Group
//

// test group
bool cxl_ide_test_group_setup(void *test_context);
bool cxl_ide_test_group_teardown(void *test_context);

#endif
