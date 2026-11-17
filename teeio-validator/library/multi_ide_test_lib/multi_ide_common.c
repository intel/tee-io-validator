/**
 *  Copyright Notice:
 *  Copyright 2026 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <string.h>

#include "ide_test.h"
#include "teeio_debug.h"
#include "multi_ide_test_lib.h"
#include "multi_ide_test_internal.h"

extern const char* m_cxl_ide_mode_names[];

// MULTI-IDE supported config items
static const char* m_multi_ide_test_configuration_name[] = {
  "default",
  NULL
};

static const char* m_multi_ide_test_configuration_priv_name[] = {
  "cxl_ide_mode",
  NULL
};

static ide_test_config_funcs_t m_multi_ide_config_funcs[MULTI_IDE_CONFIGURATION_TYPE_NUM] = {
  {
    // Default Config
    multi_ide_test_config_default_enable,
    multi_ide_test_config_default_disable,
    multi_ide_test_config_default_support,
    multi_ide_test_config_default_check
  }
};

static ide_test_group_funcs_t m_multi_ide_group_funcs = {
  multi_ide_test_group_setup,
  multi_ide_test_group_teardown
};

static ide_test_case_name_t m_multi_ide_test_case_names[] = {
  {"MultiStream", "1,2", MULTI_IDE_TEST_CASE_MULTI_STREAM},
  {NULL,          NULL,  MULTI_IDE_TEST_CASE_NUM}
};

static ide_test_case_funcs_t m_multi_ide_multi_stream_cases[MAX_MULTI_IDE_STREAM_CASE_ID] = {
  { multi_ide_test_multi_stream_1_setup, multi_ide_test_multi_stream_1_run, multi_ide_test_multi_stream_1_teardown, false },
  { multi_ide_test_multi_stream_2_setup, multi_ide_test_multi_stream_2_run, multi_ide_test_multi_stream_2_teardown, false },
};

static TEEIO_TEST_CASES m_multi_ide_test_case_funcs[MULTI_IDE_TEST_CASE_NUM] = {
  {m_multi_ide_multi_stream_cases, MAX_MULTI_IDE_STREAM_CASE_ID}
};

static const char* get_test_configuration_name (int configuration_type)
{
  if(configuration_type < 0 || configuration_type >= MULTI_IDE_CONFIGURATION_TYPE_NUM) {
    return NULL;
  }

  return m_multi_ide_test_configuration_name[configuration_type];
}

static const char** get_test_configuration_priv_names ()
{
  return m_multi_ide_test_configuration_priv_name;
}

static bool parse_test_configuration_priv_name (const char* key, const char* value, IDE_TEST_CONFIGURATION* config)
{
  if(key == NULL || value == NULL || config == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid input\n"));
    return false;
  }

  if(strcmp(key, "cxl_ide_mode") != 0) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid key: %s\n", key));
    return false;
  }

  for(int i = 0; m_cxl_ide_mode_names[i] != NULL; i++) {
    if(strcmp(value, m_cxl_ide_mode_names[i]) == 0) {
      config->priv_data.cxl_ide.ide_mode = i;
      return true;
    }
  }

  TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid cxl_ide_mode value: %s\n", value));
  return false;
}

static uint32_t get_test_configuration_bitmask (int top_tpye)
{
  TEEIO_ASSERT(top_tpye == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE);
  return BIT_MASK(MULTI_IDE_CONFIGURATION_TYPE_DEFAULT);
}

static ide_test_config_funcs_t* get_test_configuration_funcs (int top_type, int configuration_type)
{
  TEEIO_ASSERT(top_type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE && configuration_type < MULTI_IDE_CONFIGURATION_TYPE_NUM);
  return &m_multi_ide_config_funcs[configuration_type];
}

static ide_test_group_funcs_t* get_test_group_funcs (int top_type)
{
  TEEIO_ASSERT(top_type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE);
  return &m_multi_ide_group_funcs;
}

static ide_test_case_funcs_t* get_test_case_funcs (int case_class, int case_id)
{
  TEEIO_ASSERT(case_class < MULTI_IDE_TEST_CASE_NUM);
  TEEIO_TEST_CASES* test_cases = &m_multi_ide_test_case_funcs[case_class];

  TEEIO_ASSERT(case_id < test_cases->cnt);
  return &test_cases->funcs[case_id];
}

static ide_test_case_name_t* get_test_case_name (int case_class)
{
  TEEIO_ASSERT(case_class < MULTI_IDE_TEST_CASE_NUM + 1);
  return &m_multi_ide_test_case_names[case_class];
}

static void* alloc_multi_ide_test_group_context(void)
{
  multi_ide_test_group_context_t* context = (multi_ide_test_group_context_t*)malloc(sizeof(multi_ide_test_group_context_t));
  TEEIO_ASSERT(context);
  memset(context, 0, sizeof(multi_ide_test_group_context_t));
  context->common.signature = GROUP_CONTEXT_SIGNATURE;

  return context;
}

static bool multi_ide_check_configuration_bitmap(uint32_t* bitmap)
{
  // default config is always set
  *bitmap |= BIT_MASK(MULTI_IDE_CONFIGURATION_TYPE_DEFAULT);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "multi-ide configuration bitmap=0x%08x\n", *bitmap));

  return true;
}

bool multi_ide_test_lib_register_test_suite_funcs(teeio_test_funcs_t* funcs)
{
  TEEIO_ASSERT(funcs);

  funcs->get_case_funcs_func = get_test_case_funcs;
  funcs->get_case_name_func = get_test_case_name;
  funcs->get_configuration_bitmask_func = get_test_configuration_bitmask;
  funcs->get_configuration_funcs_func = get_test_configuration_funcs;
  funcs->get_configuration_name_func = get_test_configuration_name;
  funcs->get_configuration_priv_names_func = get_test_configuration_priv_names;
  funcs->parse_configuration_priv_name_func = parse_test_configuration_priv_name;
  funcs->get_group_funcs_func = get_test_group_funcs;
  funcs->alloc_test_group_context_func = alloc_multi_ide_test_group_context;
  funcs->check_configuration_bitmap_func = multi_ide_check_configuration_bitmap;

  return true;
}
