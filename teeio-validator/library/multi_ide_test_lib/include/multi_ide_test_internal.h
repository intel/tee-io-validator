/**
 *  Copyright Notice:
 *  Copyright 2026 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __MULTI_IDE_TEST_INTERNAL_H__
#define __MULTI_IDE_TEST_INTERNAL_H__

#include "ide_test.h"

#define MAX_MULTI_IDE_STREAM_CASE_ID 1

typedef enum {
  MULTI_IDE_CONFIGURATION_TYPE_DEFAULT = 0,
  MULTI_IDE_CONFIGURATION_TYPE_NUM
} MULTI_IDE_CONFIGURATION_TYPE;

typedef enum {
  MULTI_IDE_TEST_CASE_MULTI_STREAM = 0,
  MULTI_IDE_TEST_CASE_NUM
} MULTI_IDE_TEST_CASE;

extern const char *IDE_TEST_STREAM_TYPE_NAMES[];

// test group
bool multi_ide_test_group_setup(void *test_context);
bool multi_ide_test_group_teardown(void *test_context);

// test config
bool multi_ide_test_config_default_enable(void *test_context);
bool multi_ide_test_config_default_disable(void *test_context);
bool multi_ide_test_config_default_support(void *test_context);
bool multi_ide_test_config_default_check(void *test_context);

// test case
bool multi_ide_test_multi_stream_1_setup(void *test_context);
void multi_ide_test_multi_stream_1_run(void *test_context);
void multi_ide_test_multi_stream_1_teardown(void *test_context);

// open/close rootport and device port for all streams in group_context
bool multi_ide_open_ports(multi_ide_test_group_context_t *group_context);
void multi_ide_close_ports(multi_ide_test_group_context_t *group_context);

// CXL IDE mode set in [Configuration_x]
CXL_IDE_MODE multi_ide_get_cxl_ide_mode(multi_ide_test_group_context_t *group_context);

// check if there is a cxl_cm_ide (cxl_cm is true) or a PCIE/CXL.io stream (cxl_cm is false)
bool multi_ide_has_stream_type(multi_ide_test_group_context_t *group_context, bool cxl_cm);

// topology type used by the PCIE IDE helpers for a link_ide or selective_ide stream
IDE_TEST_TOPOLOGY_TYPE multi_ide_get_stream_top_type(multi_ide_stream_t *stream);

// setup/teardown streams in the order of the streams list
bool multi_ide_setup_stream(multi_ide_test_group_context_t *group_context, multi_ide_stream_t *stream);
bool multi_ide_setup_all_streams(multi_ide_test_group_context_t *group_context);
// returns true if both ends of the stream are in Insecure State after teardown
bool multi_ide_teardown_stream(multi_ide_test_group_context_t *group_context, multi_ide_stream_t *stream);
void multi_ide_teardown_all_streams(multi_ide_test_group_context_t *group_context);

// check if both ends of the stream are in Secure (PCIE) or Active (CXL) State
bool multi_ide_is_stream_active(multi_ide_test_group_context_t *group_context, multi_ide_stream_t *stream);
void multi_ide_dump_stream(multi_ide_test_group_context_t *group_context, multi_ide_stream_t *stream);

#endif
