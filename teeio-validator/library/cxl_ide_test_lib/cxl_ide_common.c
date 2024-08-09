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
  "pcrc",
  "ide_stop",
  "skid",
  "containment",
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
  {NULL, NULL, NULL, NULL},
  {NULL, NULL, NULL, NULL},
  {
    // skid mode
    cxl_ide_test_config_skid_enable,
    cxl_ide_test_config_skid_disable,
    cxl_ide_test_config_skid_support,
    cxl_ide_test_config_skid_check
  },
  {
    // containment mode
    cxl_ide_test_config_containment_enable,
    cxl_ide_test_config_containment_disable,
    cxl_ide_test_config_containment_support,
    cxl_ide_test_config_containment_check
  }
};

ide_test_group_funcs_t m_cxl_ide_group_funcs = {
  cxl_ide_test_group_setup,
  cxl_ide_test_group_teardown
};

#define CXL_IDE_TEST_CLASS_CASE_NAMES "IdeStream"
ide_test_case_name_t m_cxl_ide_test_case_names[] = {
  {"Query",       "1,2",                        CXL_MEM_IDE_TEST_CASE_QUERY     },
  {"KeyProg",     "1,2,3,4,5,6,7,8,9",          CXL_MEM_IDE_TEST_CASE_KEYPROG   },
  {"KSetGo",      "1",                          CXL_MEM_IDE_TEST_CASE_KSETGO    },
  {"KSetStop",    "1",                          CXL_MEM_IDE_TEST_CASE_KSETSTOP  },
  {"GetKey",      "1",                          CXL_MEM_IDE_TEST_CASE_GETKEY    },
  {"Test",        CXL_IDE_TEST_CLASS_CASE_NAMES,CXL_MEM_IDE_TEST_CASE_TEST      },
  {NULL,          NULL,                         CXL_MEM_IDE_TEST_CASE_NUM       }
};


ide_test_case_funcs_t m_cxl_ide_test_case_funcs[CXL_MEM_IDE_TEST_CASE_NUM][MAX_CXL_CASE_ID] = {
  // Query
  {
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false}
  },
  // KeyProg
  {
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false}
  },
  // KSetGo
  {
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false}
  },
  // KSetStop
  {
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false}
  },
  // GetKey
  {
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false}
  },
  // Test Full
  {
    { cxl_ide_test_full_ide_stream_setup, cxl_ide_test_full_ide_stream_run, cxl_ide_test_full_ide_stream_teardown, true },  // IdeStream
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false}
  }
};

static const char* get_test_configuration_name (int configuration_type)
{
  if(configuration_type > sizeof(m_cxl_ide_test_configuration_name)/sizeof(const char*)) {
    return NULL;
  }

  return m_cxl_ide_test_configuration_name[configuration_type];
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
  TEEIO_ASSERT(case_class < IDE_COMMON_TEST_CASE_NUM && case_id < MAX_PCIE_CASE_ID);
  return &m_cxl_ide_test_case_funcs[case_class][case_id];
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
  context->signature = GROUP_CONTEXT_SIGNATURE;

  return context;
}

static bool cxl_ide_check_configuration_bitmap(uint32_t* bitmap)
{
  // default config is always set
  *bitmap |= CXL_BIT_MASK(CXL_IDE_CONFIGURATION_TYPE_DEFAULT);

  // either skid mode or containment mode shall be set
  // by default skid mode is set
  if(*bitmap & CXL_BIT_MASK(CXL_IDE_CONFIGURATION_TYPE_SKID_MODE)) {
    *bitmap &= ~CXL_BIT_MASK(CXL_IDE_CONFIGURATION_TYPE_CONTAINMENT_MODE);
  } else if(*bitmap & CXL_BIT_MASK(CXL_IDE_CONFIGURATION_TYPE_CONTAINMENT_MODE)) {
    *bitmap &= ~CXL_BIT_MASK(CXL_IDE_CONFIGURATION_TYPE_SKID_MODE);
  } else {
    *bitmap |= CXL_BIT_MASK(CXL_IDE_CONFIGURATION_TYPE_SKID_MODE);
  }

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
  funcs->get_group_funcs_func = get_test_group_funcs;
  funcs->alloc_test_group_context_func = alloc_cxl_ide_test_group_context;
  funcs->check_configuration_bitmap_func = cxl_ide_check_configuration_bitmap;

  return true;
}