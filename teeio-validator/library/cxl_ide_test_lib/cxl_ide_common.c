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
#include "cxl_ide_test_common.h"

// CXL-IDE supported config items
const char* m_cxl_ide_test_configuration_name[] = {
  "default",
  "pcrc_disable",
  "ide_stop",
  "cxl_get_key",
  NULL
};

const char* m_cxl_ide_test_configuration_priv_name[] = {
  "cxl_ide_mode",
  NULL
};

const char* m_cxl_ide_mode_names[] = {
  "containment",
  "skid",
  NULL
};

uint32_t m_cxl_ide_config_bitmask = (uint32_t)CXL_LINK_IDE_CONFIGURATION_BITMASK;

ide_test_config_funcs_t m_cxl_ide_config_funcs[CXL_IDE_CONFIGURATION_TYPE_NUM] = {
  {
    // Default Config
    cxl_ide_test_config_default_enable,
    cxl_ide_test_config_default_disable,
    cxl_ide_test_config_default_support,
    cxl_ide_test_config_default_check
  },
  {
    // pcrc disable Config
    cxl_ide_test_config_set_pcrc_disable,
    cxl_ide_test_config_unset_pcrc_disable,
    cxl_ide_test_config_pcrc_disable_support,
    cxl_ide_test_config_pcrc_disable_check
  },
  {
    // ide stop Config
    cxl_ide_test_config_ide_stop_enable,
    cxl_ide_test_config_ide_stop_disable,
    cxl_ide_test_config_ide_stop_support,
    cxl_ide_test_config_ide_stop_check
  },
  {
    // get_key
    cxl_ide_test_config_get_key_enable,
    cxl_ide_test_config_get_key_disable,
    cxl_ide_test_config_get_key_support,
    cxl_ide_test_config_get_key_check
  }
};

ide_test_group_funcs_t m_cxl_ide_group_funcs = {
  cxl_ide_test_group_setup,
  cxl_ide_test_group_teardown
};

#define CXL_IDE_TEST_CLASS_CASE_NAMES "IdeStream,KeyRefresh"
ide_test_case_name_t m_cxl_ide_test_case_names[] = {
  {"Query",       "1,2",                        CXL_MEM_IDE_TEST_CASE_QUERY     },
  {"KeyProg",     "1,2,3,4,5,6,7,8,9",          CXL_MEM_IDE_TEST_CASE_KEYPROG   },
  {"KSetGo",      "1",                          CXL_MEM_IDE_TEST_CASE_KSETGO    },
  {"KSetStop",    "1",                          CXL_MEM_IDE_TEST_CASE_KSETSTOP  },
  {"GetKey",      "1",                          CXL_MEM_IDE_TEST_CASE_GETKEY    },
  {"Test",        CXL_IDE_TEST_CLASS_CASE_NAMES,CXL_MEM_IDE_TEST_CASE_TEST      },
  {NULL,          NULL,                         CXL_MEM_IDE_TEST_CASE_NUM       }
};

ide_test_case_funcs_t m_cxl_ide_query_cases[MAX_CXL_QUERY_CASE_ID] = {
  {cxl_ide_test_query_1_setup, cxl_ide_test_query_1_run, cxl_ide_test_query_1_teardown, false},
  {cxl_ide_test_query_2_setup, cxl_ide_test_query_2_run, cxl_ide_test_query_2_teardown, false},
};

ide_test_case_funcs_t m_cxl_ide_key_prog_cases[MAX_CXL_KEYPROG_CASE_ID] = {
  {cxl_ide_test_key_prog_1_setup, cxl_ide_test_key_prog_1_run, cxl_ide_test_key_prog_1_teardown, false},
  {cxl_ide_test_key_prog_2_setup, cxl_ide_test_key_prog_2_run, cxl_ide_test_key_prog_2_teardown, false},
  {cxl_ide_test_key_prog_3_setup, cxl_ide_test_key_prog_3_run, cxl_ide_test_key_prog_3_teardown, false},
  {cxl_ide_test_key_prog_4_setup, cxl_ide_test_key_prog_4_run, cxl_ide_test_key_prog_4_teardown, false},
  {cxl_ide_test_key_prog_5_setup, cxl_ide_test_key_prog_5_run, cxl_ide_test_key_prog_5_teardown, false},
  {cxl_ide_test_key_prog_6_setup, cxl_ide_test_key_prog_6_run, cxl_ide_test_key_prog_6_teardown, false},
  {cxl_ide_test_key_prog_7_setup, cxl_ide_test_key_prog_7_run, cxl_ide_test_key_prog_7_teardown, false},
  {cxl_ide_test_key_prog_8_setup, cxl_ide_test_key_prog_8_run, cxl_ide_test_key_prog_8_teardown, false},
  {cxl_ide_test_key_prog_9_setup, cxl_ide_test_key_prog_9_run, cxl_ide_test_key_prog_9_teardown, false},
};

ide_test_case_funcs_t m_cxl_ide_kset_go_cases[MAX_CXL_KSETGO_CASE_ID] = {
  {cxl_ide_test_kset_go_1_setup, cxl_ide_test_kset_go_1_run, cxl_ide_test_kset_go_1_teardown, true},
};

ide_test_case_funcs_t m_cxl_ide_kset_stop_cases[MAX_CXL_KSETSTOP_CASE_ID] = {
  {cxl_ide_test_kset_stop_1_setup, cxl_ide_test_kset_stop_1_run, cxl_ide_test_kset_stop_1_teardown, false},
};

ide_test_case_funcs_t m_cxl_ide_get_key_cases[MAX_CXL_GETKEY_CASE_ID] = {
  {cxl_ide_test_get_key_1_setup, cxl_ide_test_get_key_1_run, cxl_ide_test_get_key_1_teardown, false},
};

ide_test_case_funcs_t m_cxl_ide_test_full_cases[MAX_CXL_FULL_CASE_ID] = {
  { cxl_ide_test_full_ide_stream_setup, cxl_ide_test_full_ide_stream_run, cxl_ide_test_full_ide_stream_teardown, false },  // IdeStream
  { cxl_ide_test_keyrefresh_setup,     cxl_ide_test_keyrefresh_run,     cxl_ide_test_keyrefresh_teardown,     false },  // KeyRefresh
};

TEEIO_TEST_CASES m_cxl_ide_test_case_funcs[CXL_MEM_IDE_TEST_CASE_NUM] = {
  {m_cxl_ide_query_cases,       MAX_CXL_QUERY_CASE_ID},
  {m_cxl_ide_key_prog_cases,    MAX_CXL_KEYPROG_CASE_ID},
  {m_cxl_ide_kset_go_cases,     MAX_CXL_KSETGO_CASE_ID},
  {m_cxl_ide_kset_stop_cases,   MAX_CXL_KSETSTOP_CASE_ID},
  {m_cxl_ide_get_key_cases,     MAX_CXL_GETKEY_CASE_ID},
  {m_cxl_ide_test_full_cases,   MAX_CXL_FULL_CASE_ID}
};

static const char* get_test_configuration_name (int configuration_type)
{
  if(configuration_type > sizeof(m_cxl_ide_test_configuration_name)/sizeof(const char*)) {
    return NULL;
  }

  return m_cxl_ide_test_configuration_name[configuration_type];
}

static const char** get_test_configuration_priv_names ()
{
  return m_cxl_ide_test_configuration_priv_name;
}

static bool parse_cxl_ide_mode_from_string(const char* string, int* value)
{
  if(string == NULL || value == NULL) {
    return false;
  }

  for(int i = 0; m_cxl_ide_mode_names[i] != NULL; i++) {
    if(strcmp(string, m_cxl_ide_mode_names[i]) == 0) {
      *value = i;
      return true;
    }
  }

  return false;
}

static bool parse_test_configuration_priv_name (const char* key, const char* value, IDE_TEST_CONFIGURATION* config)
{
  if(key == NULL || value == NULL || config == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid input\n"));
    return false;
  }

  if(strcmp(key, "cxl_ide_mode") == 0) {
    int ide_mode = 0;
    if(!parse_cxl_ide_mode_from_string(value, &ide_mode)) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid cxl_ide_mode value: %s\n", value));
      return false;
    }
    config->priv_data.cxl_ide.ide_mode = ide_mode;
  } else {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid key: %s\n", key));
    return false;
  }

  return true;
}

static uint32_t get_test_configuration_bitmask (int top_tpye)
{
  // CXL.memcache only supports LinkIde
  TEEIO_ASSERT(top_tpye == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE);
  return m_cxl_ide_config_bitmask;
}

static ide_test_config_funcs_t* get_test_configuration_funcs (int top_type, int configuration_type)
{
  TEEIO_ASSERT(top_type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE && configuration_type < CXL_IDE_CONFIGURATION_TYPE_NUM);
  return &m_cxl_ide_config_funcs[configuration_type];
}

static ide_test_group_funcs_t* get_test_group_funcs (int top_type)
{
  TEEIO_ASSERT(top_type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE);
  return &m_cxl_ide_group_funcs;
}

static ide_test_case_funcs_t* get_test_case_funcs (int case_class, int case_id)
{
  TEEIO_ASSERT(case_class < CXL_MEM_IDE_TEST_CASE_NUM);
  TEEIO_TEST_CASES* test_cases = &m_cxl_ide_test_case_funcs[case_class];

  TEEIO_ASSERT(case_id < test_cases->cnt);
  return &test_cases->funcs[case_id];
}

static ide_test_case_name_t* get_test_case_name (int case_class)
{
  TEEIO_ASSERT(case_class < CXL_MEM_IDE_TEST_CASE_NUM + 1);
  return &m_cxl_ide_test_case_names[case_class];
}

static void* alloc_cxl_ide_test_group_context(void)
{
  cxl_ide_test_group_context_t* context = (cxl_ide_test_group_context_t*)malloc(sizeof(cxl_ide_test_group_context_t));
  TEEIO_ASSERT(context);
  memset(context, 0, sizeof(cxl_ide_test_group_context_t));
  context->common.signature = GROUP_CONTEXT_SIGNATURE;

  return context;
}

static bool cxl_ide_check_configuration_bitmap(uint32_t* bitmap)
{
  // default config is always set
  *bitmap |= CXL_BIT_MASK(CXL_IDE_CONFIGURATION_TYPE_DEFAULT);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl-ide configuration bitmap=0x%08x\n", *bitmap));

  return true;
}

bool cxl_ide_test_lib_register_test_suite_funcs(teeio_test_funcs_t* funcs)
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
  funcs->alloc_test_group_context_func = alloc_cxl_ide_test_group_context;
  funcs->check_configuration_bitmap_func = cxl_ide_check_configuration_bitmap;

  return true;
}