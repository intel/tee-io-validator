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
void cxl_ide_test_full_ide_stream_run(void *test_context);
void cxl_ide_test_full_ide_stream_teardown(void *test_context);

// Key Refresh
bool cxl_ide_test_keyrefresh_setup(void *test_context);
void cxl_ide_test_keyrefresh_run(void *test_context);
void cxl_ide_test_keyrefresh_teardown(void *test_context);

// Query
bool cxl_ide_test_query_1_setup(void *test_context);
void cxl_ide_test_query_1_run(void *test_context);
void cxl_ide_test_query_1_teardown(void *test_context);

bool cxl_ide_test_query_2_setup(void *test_context);
void cxl_ide_test_query_2_run(void *test_context);
void cxl_ide_test_query_2_teardown(void *test_context);

// KEY_PROG
bool cxl_ide_test_key_prog_1_setup(void *test_context);
void cxl_ide_test_key_prog_1_run(void *test_context);
void cxl_ide_test_key_prog_1_teardown(void *test_context);

bool cxl_ide_test_key_prog_2_setup(void *test_context);
void cxl_ide_test_key_prog_2_run(void *test_context);
void cxl_ide_test_key_prog_2_teardown(void *test_context);

bool cxl_ide_test_key_prog_3_setup(void *test_context);
void cxl_ide_test_key_prog_3_run(void *test_context);
void cxl_ide_test_key_prog_3_teardown(void *test_context);

bool cxl_ide_test_key_prog_4_setup(void *test_context);
void cxl_ide_test_key_prog_4_run(void *test_context);
void cxl_ide_test_key_prog_4_teardown(void *test_context);

bool cxl_ide_test_key_prog_5_setup(void *test_context);
void cxl_ide_test_key_prog_5_run(void *test_context);
void cxl_ide_test_key_prog_5_teardown(void *test_context);

bool cxl_ide_test_key_prog_6_setup(void *test_context);
void cxl_ide_test_key_prog_6_run(void *test_context);
void cxl_ide_test_key_prog_6_teardown(void *test_context);

bool cxl_ide_test_key_prog_7_setup(void *test_context);
void cxl_ide_test_key_prog_7_run(void *test_context);
void cxl_ide_test_key_prog_7_teardown(void *test_context);

bool cxl_ide_test_key_prog_8_setup(void *test_context);
void cxl_ide_test_key_prog_8_run(void *test_context);
void cxl_ide_test_key_prog_8_teardown(void *test_context);

bool cxl_ide_test_key_prog_9_setup(void *test_context);
void cxl_ide_test_key_prog_9_run(void *test_context);
void cxl_ide_test_key_prog_9_teardown(void *test_context);

// KSET_GO
bool cxl_ide_test_kset_go_1_setup(void *test_context);
void cxl_ide_test_kset_go_1_run(void *test_context);
void cxl_ide_test_kset_go_1_teardown(void *test_context);

// KSET_STOP
bool cxl_ide_test_kset_stop_1_setup(void *test_context);
void cxl_ide_test_kset_stop_1_run(void *test_context);
void cxl_ide_test_kset_stop_1_teardown(void *test_context);

// GET_KEY
// KSET_GO
bool cxl_ide_test_get_key_1_setup(void *test_context);
void cxl_ide_test_get_key_1_run(void *test_context);
void cxl_ide_test_get_key_1_teardown(void *test_context);

//
// CXL_IDE Test Config
//

// default config
bool cxl_ide_test_config_default_enable(void *test_context);
bool cxl_ide_test_config_default_disable(void *test_context);
bool cxl_ide_test_config_default_support(void *test_context);
bool cxl_ide_test_config_default_check(void *test_context);

// pcrc config
bool cxl_ide_test_config_unset_pcrc_disable(void *test_context);
bool cxl_ide_test_config_set_pcrc_disable(void *test_context);
bool cxl_ide_test_config_pcrc_disable_support(void *test_context);
bool cxl_ide_test_config_pcrc_disable_check(void *test_context);

// ide_stop config
bool cxl_ide_test_config_ide_stop_enable(void *test_context);
bool cxl_ide_test_config_ide_stop_disable(void *test_context);
bool cxl_ide_test_config_ide_stop_support(void *test_context);
bool cxl_ide_test_config_ide_stop_check(void *test_context);

// get_key
bool cxl_ide_test_config_get_key_enable(void *test_context);
bool cxl_ide_test_config_get_key_disable(void *test_context);
bool cxl_ide_test_config_get_key_support(void *test_context);
bool cxl_ide_test_config_get_key_check(void *test_context);

//
// CXL_IDE Test Group
//

// test group
bool cxl_ide_test_group_setup(void *test_context);
bool cxl_ide_test_group_teardown(void *test_context);

#endif
