/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "helperlib.h"
#include "ide_test.h"
#include "library/common_test_utility_lib.h"
#include "library/spdm_responder_conformance_test_lib.h"
#include "spdm_test_common.h"

// SPDM supported config items
const char* m_spdm_test_configuration_name[] = {
  "default"
};

uint32_t m_spdm_config_bitmask = SPDM_TEST_CONFIGURATION_BITMASK;

ide_test_config_funcs_t m_spdm_test_config_funcs[SPDM_TEST_CONFIGURATION_TYPE_NUM] = {
  {
    // Default Config
    spdm_test_config_default_enable,
    spdm_test_config_default_disable,
    spdm_test_config_default_support,
    spdm_test_config_default_check
  }
};

ide_test_group_funcs_t m_spdm_test_group_funcs = {
  spdm_test_group_setup,
  spdm_test_group_teardown
};

static const char* get_test_configuration_name (int configuration_type)
{
  if(configuration_type > sizeof(m_spdm_test_configuration_name)/sizeof(const char*)) {
    return NULL;
  }

  return m_spdm_test_configuration_name[configuration_type];
}

static uint32_t get_test_configuration_bitmask (int top_tpye)
{
  return m_spdm_config_bitmask;
}

static ide_test_config_funcs_t* get_test_configuration_funcs (int top_type, int configuration_type)
{
  return &m_spdm_test_config_funcs[configuration_type];
}

static ide_test_group_funcs_t* get_test_group_funcs (int top_type)
{
  return &m_spdm_test_group_funcs;
}

static ide_test_case_funcs_t* get_test_case_funcs (int case_class, int case_id)
{
  TEEIO_ASSERT(case_class < SPDM_TEST_CASE_NUM);
  TEEIO_TEST_CASES* test_cases = &m_spdm_test_case_funcs[case_class];

  TEEIO_ASSERT(case_id < test_cases->cnt);
  return &test_cases->funcs[case_id];
}

static ide_test_case_name_t* get_test_case_name (int case_class)
{
  TEEIO_ASSERT(case_class < SPDM_TEST_CASE_NUM + 1);
  return &m_spdm_test_case_names[case_class];
}

static void* alloc_spdm_test_group_context(void)
{
  spdm_test_group_context_t* context = (spdm_test_group_context_t*)malloc(sizeof(spdm_test_group_context_t));
  TEEIO_ASSERT(context);
  memset(context, 0, sizeof(spdm_test_group_context_t));
  context->common.signature = GROUP_CONTEXT_SIGNATURE;

  return context;
}

static bool spdm_test_check_configuration_bitmap(uint32_t* bitmap)
{
  // default config is always set
  *bitmap |= BIT_MASK(SPDM_TEST_CONFIGURATION_TYPE_DEFAULT);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "spdm_test configuration bitmap=0x%08x\n", *bitmap));

  return true;
}

bool spdm_test_lib_register_test_suite_funcs(teeio_test_funcs_t* funcs)
{
  TEEIO_ASSERT(funcs);

  spdm_test_lib_init_test_cases();

  funcs->get_case_funcs_func = get_test_case_funcs;
  funcs->get_case_name_func = get_test_case_name;
  funcs->get_configuration_bitmask_func = get_test_configuration_bitmask;
  funcs->get_configuration_funcs_func = get_test_configuration_funcs;
  funcs->get_configuration_name_func = get_test_configuration_name;
  funcs->get_group_funcs_func = get_test_group_funcs;
  funcs->alloc_test_group_context_func = alloc_spdm_test_group_context;
  funcs->check_configuration_bitmap_func = spdm_test_check_configuration_bitmap;

  return true;
}

int spdm_test_lib_get_case_class(int responder_group_id);

void common_test_record_test_assertion (
  common_test_group_id group_id,
  common_test_case_id case_id,
  common_test_assertion_id assertion_id,
  common_test_result_t test_result,
  const char *message_format,
  ...)
{
  char buffer[MAX_LINE_LENGTH] = {0};
  va_list marker;

  int case_class = spdm_test_lib_get_case_class(group_id);
  TEEIO_ASSERT(case_class < SPDM_TEST_CASE_NUM);

  if (message_format != NULL) {
      va_start(marker, message_format);

      vsnprintf(buffer, sizeof(buffer), message_format, marker);

      va_end(marker);
  }

  teeio_record_assertion_result(
    case_class, case_id, assertion_id,
    IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
    (teeio_test_result_t)test_result, 
    "%s", buffer);
}

void common_test_run_test_suite (
  void *test_context,
  const common_test_suite_t *test_suite,
  const common_test_suite_config_t *test_suite_config)
{
  TEEIO_ASSERT(false);
}

void common_test_record_test_message(const char *message_format, ...)
{
  char buffer[MAX_LINE_LENGTH] = {0};
  va_list marker;

  if (message_format != NULL) {
      va_start(marker, message_format);

      vsnprintf(buffer, sizeof(buffer), message_format, marker);

      va_end(marker);
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%s\n", buffer));
}

void* spdm_test_get_spdm_context_from_test_context(void *test_context)
{
  TEEIO_ASSERT(test_context);

  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  spdm_test_group_context_t *group_context = (spdm_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);
  TEEIO_ASSERT(group_context->spdm_doe.spdm_context);

  return group_context->spdm_doe.spdm_context;
}
