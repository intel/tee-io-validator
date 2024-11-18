/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "teeio_debug.h"
#include "hal/base.h"
#include "ide_test.h"
#include "cxl_tsp_test_common.h"

// CXL-TSP supported config items
const char* m_cxl_tsp_test_configuration_name[] = {
  "default",
  NULL
};

uint32_t m_cxl_tsp_config_bitmask = (uint32_t)CXL_TSP_CONFIGURATION_BITMASK;

ide_test_config_funcs_t m_cxl_tsp_config_funcs = {
  cxl_tsp_test_config_common_enable,
  cxl_tsp_test_config_common_disable,
  cxl_tsp_test_config_common_support,
  cxl_tsp_test_config_common_check
};

ide_test_group_funcs_t m_cxl_tsp_group_funcs = {
  cxl_tsp_test_group_setup,
  cxl_tsp_test_group_teardown
};

#define CXL_IDE_TEST_CLASS_CASE_NAMES "IdeStream"
ide_test_case_name_t m_cxl_tsp_test_case_names[] = {
  {"GetVersion",              "1",   CXL_TSP_TEST_CASE_GET_VERSION      },
  {"GetCapabilities",         "1",   CXL_TSP_TEST_CASE_GET_CAPS         },
  {"SetConfiguration",        "1",   CXL_TSP_TEST_CASE_SET_CFG          },
  {"GetConfiguration",        "1",   CXL_TSP_TEST_CASE_GET_CFG          },
  {"GetConfigurationReport",  "1",   CXL_TSP_TEST_CASE_GET_CFG_REPORT   },
  {"LockConfiguration",       "1,2", CXL_TSP_TEST_CASE_LOCK_CFG         },
  {NULL,                      NULL,  CXL_TSP_TEST_CASE_MAX              }
};

ide_test_case_funcs_t m_cxl_tsp_get_version_cases[MAX_CXL_TSP_GET_VERSION_CASE_ID] = {
  {cxl_tsp_test_get_version_setup, cxl_tsp_test_get_version_run, cxl_tsp_test_get_version_teardown, false}
};

ide_test_case_funcs_t m_cxl_tsp_get_caps_cases[MAX_CXL_TSP_GET_CAPS_CASE_ID] = {
  {cxl_tsp_test_get_caps_setup, cxl_tsp_test_get_caps_run, cxl_tsp_test_get_caps_teardown, false}
};

ide_test_case_funcs_t m_cxl_tsp_set_configuration_cases[MAX_CXL_TSP_SET_CFG_CASE_ID] = {
  {cxl_tsp_test_set_configuration_setup, cxl_tsp_test_set_configuration_run, cxl_tsp_test_set_configuration_teardown, false}
};

ide_test_case_funcs_t m_cxl_tsp_get_configuration_cases[MAX_CXL_TSP_GET_CFG_CASE_ID] = {
  {cxl_tsp_test_get_configuration_setup, cxl_tsp_test_get_configuration_run, cxl_tsp_test_get_configuration_teardown, false}
};

ide_test_case_funcs_t m_cxl_tsp_get_configuration_report_cases[MAX_CXL_TSP_GET_CFG_REPORT_CASE_ID] = {
  {cxl_tsp_test_get_configuration_report_setup, cxl_tsp_test_get_configuration_report_run, cxl_tsp_test_get_configuration_report_teardown, false}
};

ide_test_case_funcs_t m_cxl_tsp_lock_configuration_cases[MAX_CXL_TSP_LOCK_CFG_CASE_ID] = {
  {cxl_tsp_test_lock_configuration_1_setup, cxl_tsp_test_lock_configuration_1_run, cxl_tsp_test_lock_configuration_1_teardown, false},
  {cxl_tsp_test_lock_configuration_2_setup, cxl_tsp_test_lock_configuration_2_run, cxl_tsp_test_lock_configuration_2_teardown, false}
};

TEEIO_TEST_CASES m_cxl_tsp_test_case_funcs[CXL_TSP_TEST_CASE_MAX] = {
  {m_cxl_tsp_get_version_cases,               MAX_CXL_TSP_GET_VERSION_CASE_ID},
  {m_cxl_tsp_get_caps_cases,                  MAX_CXL_TSP_GET_CAPS_CASE_ID},
  {m_cxl_tsp_set_configuration_cases,         MAX_CXL_TSP_SET_CFG_CASE_ID},
  {m_cxl_tsp_get_configuration_cases,         MAX_CXL_TSP_GET_CFG_CASE_ID},
  {m_cxl_tsp_get_configuration_report_cases,  MAX_CXL_TSP_GET_CFG_REPORT_CASE_ID},
  {m_cxl_tsp_lock_configuration_cases,        MAX_CXL_TSP_LOCK_CFG_CASE_ID}
};

static const char* get_test_configuration_name (int configuration_type)
{
  if(configuration_type > sizeof(m_cxl_tsp_test_configuration_name)/sizeof(const char*)) {
    return NULL;
  }

  return m_cxl_tsp_test_configuration_name[configuration_type];
}

static uint32_t get_test_configuration_bitmask (int top_tpye)
{
  return m_cxl_tsp_config_bitmask;
}

static ide_test_config_funcs_t* get_test_configuration_funcs (int top_type, int configuration_type)
{
  return &m_cxl_tsp_config_funcs;
}

static ide_test_group_funcs_t* get_test_group_funcs (int top_type)
{
  return &m_cxl_tsp_group_funcs;
}

static ide_test_case_funcs_t* get_test_case_funcs (int case_class, int case_id)
{
  TEEIO_ASSERT(case_class < CXL_TSP_TEST_CASE_MAX);
  TEEIO_TEST_CASES* test_cases = &m_cxl_tsp_test_case_funcs[case_class];

  TEEIO_ASSERT(case_id < test_cases->cnt);
  return &test_cases->funcs[case_id];
}

static ide_test_case_name_t* get_test_case_name (int case_class)
{
  TEEIO_ASSERT(case_class < CXL_TSP_TEST_CASE_MAX + 1);
  return &m_cxl_tsp_test_case_names[case_class];
}

static void* alloc_cxl_tsp_test_group_context(void)
{
  cxl_tsp_test_group_context_t* context = (cxl_tsp_test_group_context_t*)malloc(sizeof(cxl_tsp_test_group_context_t));
  TEEIO_ASSERT(context);
  memset(context, 0, sizeof(cxl_tsp_test_group_context_t));
  context->common.signature = GROUP_CONTEXT_SIGNATURE;

  return context;
}

static bool cxl_tsp_check_configuration_bitmap(uint32_t* bitmap)
{
  // default config is always set
  *bitmap |= CXL_TSP_CONFIGURATION_BITMASK;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl-tsp configuration bitmap=0x%08x\n", *bitmap));

  return true;
}

bool cxl_tsp_test_lib_register_test_suite_funcs(teeio_test_funcs_t* funcs)
{
  TEEIO_ASSERT(funcs);

  funcs->get_case_funcs_func = get_test_case_funcs;
  funcs->get_case_name_func = get_test_case_name;
  funcs->get_configuration_bitmask_func = get_test_configuration_bitmask;
  funcs->get_configuration_funcs_func = get_test_configuration_funcs;
  funcs->get_configuration_name_func = get_test_configuration_name;
  funcs->get_group_funcs_func = get_test_group_funcs;
  funcs->alloc_test_group_context_func = alloc_cxl_tsp_test_group_context;
  funcs->check_configuration_bitmap_func = cxl_tsp_check_configuration_bitmap;

  return true;
}