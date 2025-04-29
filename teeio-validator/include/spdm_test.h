/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __SPDM_TEST_H__
#define __SPDM_TEST_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/param.h>

// refer to https://github.com/DMTF/SPDM-Responder-Validator/tree/main/doc
typedef enum {
  SPDM_TEST_CASE_VERSION = 0,
  SPDM_TEST_CASE_CAPABILITIES,
  SPDM_TEST_CASE_ALGORITHMS,
  SPDM_TEST_CASE_CERTIFICATE,
  SPDM_TEST_CASE_MEASUREMENTS,
  SPDM_TEST_CASE_KEY_EXCHANGE_RSP,
  SPDM_TEST_CASE_FINISH_RSP,
  SPDM_TEST_CASE_END_SESSION_ACK,
  SPDM_TEST_CASE_NUM
} SPDM_TEST_CASE;

typedef enum {
  SPDM_TEST_CONFIGURATION_TYPE_DEFAULT = 0,
  SPDM_TEST_CONFIGURATION_TYPE_NUM
} SPDM_TEST_CONFIGURATION_TYPE;

#define SPDM_TEST_CONFIGURATION_BITMASK 1

#endif