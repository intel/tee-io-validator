/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdint.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "pcie.h"
#include "ide_test.h"
#include "teeio_debug.h"
#include "helperlib.h"
#include "helper_internal.h"

extern ide_run_test_group_result_t* g_current_group_result;
extern ide_run_test_config_result_t* g_current_config_result;
extern ide_run_test_case_result_t* g_current_case_result;

/**
 * Check the test result of a case.
 * Failure or Pass of a case is decided by its assertions result.
 */
teeio_test_result_t teeio_test_case_result(
  int case_class,
  int case_id
  )
{
  if(g_current_case_result == NULL) {
    return TEEIO_TEST_RESULT_NOT_TESTED;
  }

  ide_run_test_case_result_t* case_result = g_current_case_result;
  if(case_result->class_id != case_class || case_result->case_id != case_id) {
    TEEIO_ASSERT(false);
    return TEEIO_TEST_RESULT_NOT_TESTED;
  }

  teeio_test_result_t result = TEEIO_TEST_RESULT_NOT_TESTED;
  int pass = 0, failed = 0;
  ide_run_test_case_assertion_result_t* assertion = case_result->assertion_result;
  while(assertion) {
    if(assertion->type == IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST) {
      if(assertion->result == TEEIO_TEST_RESULT_PASS) {
        pass++;
      } else if(assertion->result == TEEIO_TEST_RESULT_FAILED) {
        failed++;
      }
    }
    assertion = assertion->next;
  }

  if(failed > 0) {
    result = TEEIO_TEST_RESULT_FAILED;
  } else if(pass > 0 && failed == 0) {
    result = TEEIO_TEST_RESULT_PASS;
  }

  return result;
}

bool teeio_record_assertion_result(
  int case_class,
  int case_id,
  int assertion_id,
  ide_common_test_case_assertion_type_t assertion_type,
  teeio_test_result_t result,
  const char *message_format,
  ...)
{
  va_list marker;

  if(g_current_case_result == NULL) {
    TEEIO_ASSERT(false);
    return false;
  }

  ide_run_test_case_result_t* case_result = g_current_case_result;
  if(case_result->class_id != case_class || case_result->case_id != case_id) {
    TEEIO_ASSERT(false);
    return false;
  }

  ide_run_test_case_assertion_result_t* ar = (ide_run_test_case_assertion_result_t *)malloc(sizeof(ide_run_test_case_assertion_result_t));
  memset(ar, 0, sizeof(ide_run_test_case_assertion_result_t));

  if (message_format != NULL) {
      va_start(marker, message_format);

      vsnprintf(ar->extra_data, sizeof(ar->extra_data), message_format, marker);

      va_end(marker);
  }

  ar->class_id = case_class;
  ar->case_id = case_id;
  ar->assertion_id = assertion_id;
  ar->type = assertion_type;
  ar->result = result;

  // Append it to case_result
  ide_run_test_case_assertion_result_t *itr = case_result->assertion_result;
  if(itr == NULL) {
    case_result->assertion_result = ar;
  } else {
    while(itr->next) {
      itr = itr->next;
    }
    itr->next = ar;
  }

  // increase the total passed/failed
  if(result == TEEIO_TEST_RESULT_PASS) {
    g_current_config_result->total_passed++;
    g_current_group_result->total_passed++;
    case_result->total_passed++;
  } else if(result == TEEIO_TEST_RESULT_FAILED) {
    g_current_config_result->total_failed++;
    g_current_group_result->total_failed++;
    case_result->total_failed++;
  }

  return true;
}

bool teeio_record_config_item_result(
  int config_item_id,
  teeio_test_config_func_t func,
  teeio_test_result_t result
  )
{
  if(g_current_case_result == NULL) {
    return false;
  }

  ide_run_test_config_item_result_t* config_item_result = g_current_case_result->config_item_result;
  while(config_item_result) {
    if(config_item_result->config_item_id == config_item_id) {
      break;
    }
    config_item_result = config_item_result->next;
  }

  if(config_item_result == NULL) {
    config_item_result = (ide_run_test_config_item_result_t *)malloc(sizeof(ide_run_test_config_item_result_t));
    memset(config_item_result, 0, sizeof(ide_run_test_config_item_result_t));
    config_item_result->config_item_id = config_item_id;

    // Append it to g_current_case_result->config_item_result
    ide_run_test_config_item_result_t *itr = g_current_case_result->config_item_result;
    if(itr == NULL) {
      g_current_case_result->config_item_result = config_item_result;
    } else {
      while(itr->next) {
        itr = itr->next;
      }
      itr->next = config_item_result;
    }
  }

  config_item_result->results[func] = result;

  return true;
}

bool teeio_record_group_result(
  teeio_test_group_func_t func,
  teeio_test_result_t result,
  const char *message_format,
  ...  )
{
  va_list marker;

  if(g_current_group_result == NULL || func >= TEEIO_TEST_GROUP_FUNC_MAX) {
    return false;
  }

  teeio_test_group_func_result_t* func_result = &g_current_group_result->func_results[func];
  func_result->result = result;

  if (message_format != NULL) {
    va_start(marker, message_format);
    vsnprintf(func_result->extra_data, sizeof(func_result->extra_data), message_format, marker);
    va_end(marker);
  }

  return true;
}
