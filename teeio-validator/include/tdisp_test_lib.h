/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __TDISP_TEST_LIB_H__
#define __TDISP_TEST_LIB_H__

#include "ide_test.h"

typedef enum {
	TDISP_TEST_CASE_VERSION = 0,
	TDISP_TEST_CASE_CAPABILITIES,
	TDISP_TEST_CASE_LOCK_INTERFACE,
	TDISP_TEST_CASE_DEVICE_REPORT,
	TDISP_TEST_CASE_DEVICE_STATE,
	TDISP_TEST_CASE_START_INTERFACE,
	TDISP_TEST_CASE_STOP_INTERFACE,
	TDISP_TEST_CASE_NUM,
} TDISP_TEST_CASE;

// Follow doc/tdisp_test/TdispTestCase
#define MAX_TDISP_VERSION_CASE_ID               1
#define MAX_TDISP_CAPABILITIES_CASE_ID          1
#define MAX_LOCK_INTERFACE_RESPONSE_CASE_ID     4
#define MAX_DEVICE_INTERFACE_REPORT_CASE_ID     5
#define MAX_DEVICE_INTERFACE_STATE_CASE_ID      4
#define MAX_START_INTERFACE_RESPONSE_CASE_ID    4
#define MAX_STOP_INTERFACE_RESPONSE_CASE_ID     3

// tdisp_test_lib header file

bool tdisp_test_lib_register_test_suite_funcs (teeio_test_funcs_t *funcs);


#endif
