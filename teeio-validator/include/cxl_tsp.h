/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __CXL_TSP_H__
#define __CXL_TSP_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/param.h>
#include "cxl.h"
#include "intel_keyp.h"

#define MAX_CXL_TSP_GET_VERSION_CASE_ID 1
#define MAX_CXL_TSP_GET_CAPS_CASE_ID 1
#define MAX_CXL_TSP_SET_CFG_CASE_ID 1
#define MAX_CXL_TSP_GET_CFG_CASE_ID 1
#define MAX_CXL_TSP_GET_CFG_REPORT_CASE_ID 1
#define MAX_CXL_TSP_LOCK_CFG_CASE_ID 2
#define MAX_CXL_TSP_CASE_ID \
  (MAX(MAX_CXL_TSP_GET_VERSION_CASE_ID, MAX(MAX_CXL_TSP_GET_CAPS_CASE_ID, MAX(MAX_CXL_TSP_SET_CFG_CASE_ID, MAX(MAX_CXL_TSP_GET_CFG_CASE_ID, MAX(MAX_CXL_TSP_GET_CFG_REPORT_CASE_ID, MAX_CXL_TSP_LOCK_CFG_CASE_ID))))))

typedef enum {
  CXL_TSP_TEST_CASE_GET_VERSION = 0,
  CXL_TSP_TEST_CASE_GET_CAPS,
  CXL_TSP_TEST_CASE_SET_CFG,
  CXL_TSP_TEST_CASE_GET_CFG,
  CXL_TSP_TEST_CASE_GET_CFG_REPORT,
  CXL_TSP_TEST_CASE_LOCK_CFG,
  CXL_TSP_TEST_CASE_MAX
} CXL_TSP_TEST_CASE;

typedef enum {
  CXL_TSP_CONFIGURATION_TYPE_DEFAULT = 0,
} CXL_TSP_CONFIGURATION_TYPE;

#define CXL_TSP_CONFIGURATION_BITMASK 0x1

#endif
