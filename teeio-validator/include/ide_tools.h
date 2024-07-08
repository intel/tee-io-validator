/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __LS_SET_IDE_H__
#define __LS_SET_IDE_H__

#include "ide_test.h"

#define SET_IDE_NAME "setide"
#define SET_IDE_VERSION "0.2.0"

#define LS_IDE_NAME "lside"
#define LS_IDE_VERSION "0.2.0"

#define NUM_SEL_IDE_ISSUE

typedef enum
{
    IDE_OPERATION_LIST = 0,
    IDE_OPERATION_CLEAR
} IDE_OPERATION;

typedef struct
{
    ide_common_test_port_context_t *root_port_context;
    ide_common_test_port_context_t *upper_port_context;
    ide_common_test_port_context_t *lower_port_context;
    ide_common_test_switch_internal_conn_context_t *sw_conn1;
    ide_common_test_switch_internal_conn_context_t *sw_conn2;
} DEVCIES_CONTEXT;

ide_common_test_switch_internal_conn_context_t *alloc_switch_internal_conn_context(IDE_TEST_CONFIG *test_config, IDE_TEST_TOPOLOGY *top, IDE_SWITCH_INTERNAL_CONNECTION *conn);
bool scan_open_devices_in_top(IDE_TEST_CONFIG *test_config, int top_id, DEVCIES_CONTEXT *devices_context);
bool read_ide_cap_ctrl_register(IDE_PORT* port, uint32_t *ide_cap, uint32_t *ide_ctrl);
bool parse_ide_test_init(IDE_TEST_CONFIG *test_config, const char *ide_test_ini);
ide_test_case_name_t *get_test_case_from_string(const char *test_case_name, int *index, TEEIO_TEST_CATEGORY test_category);

#endif
