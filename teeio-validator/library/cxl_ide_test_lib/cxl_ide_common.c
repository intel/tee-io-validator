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

#define CXL_MEM_CLASS_CASE_NAMES "IdeStream"
ide_test_case_name_t m_cxl_mem_ide_test_case_names[CXL_MEM_IDE_TEST_CASE_NUM] = {
  {"Query",       "1,2",                    CXL_MEM_IDE_TEST_CASE_QUERY     },
  {"KeyProg",     "1,2,3,4,5,6,7,8,9",      CXL_MEM_IDE_TEST_CASE_KEYPROG   },
  {"KSetGo",      "1",                      CXL_MEM_IDE_TEST_CASE_KSETGO    },
  {"KSetStop",    "1",                      CXL_MEM_IDE_TEST_CASE_KSETSTOP  },
  {"GetKey",      "1",                      CXL_MEM_IDE_TEST_CASE_GETKEY    },
  {"Test",        CXL_MEM_CLASS_CASE_NAMES, CXL_MEM_IDE_TEST_CASE_TEST      }
};

ide_test_case_name_t* cxl_ide_test_lib_get_test_case_names(int* cnt)
{
  *cnt = CXL_MEM_IDE_TEST_CASE_NUM;
  return m_cxl_mem_ide_test_case_names;
}

uint32_t m_cxl_ide_top_config_bitmasks[] = {
  (uint32_t)0,
  (uint32_t)CXL_LINK_IDE_CONFIGURATION_BITMASK
};

uint32_t cxl_ide_test_lib_get_config_bitmask(int* config_type_num, IDE_TEST_TOPOLOGY_TYPE top_type)
{
  TEEIO_ASSERT(config_type_num);
  TEEIO_ASSERT(top_type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE);

  *config_type_num = CXL_IDE_CONFIGURATION_TYPE_NUM;
  return m_cxl_ide_top_config_bitmasks[top_type];
}


static bool _register_test_case_func(
  teeio_test_case_funcs_t* case_funcs,
  int test_case, int case_id,
  ide_common_test_case_setup_func_t setup,
  ide_common_test_case_run_func_t run,
  ide_common_test_case_teardown_func_t teardown,
  bool complete_ide_stream
)
{
  teeio_test_case_funcs_t* p = (teeio_test_case_funcs_t *)malloc(sizeof(teeio_test_case_funcs_t));
  memset(p, 0, sizeof(teeio_test_case_funcs_t));

  p->test_category = TEEIO_TEST_CATEGORY_CXL_IDE;
  p->test_case = test_case;
  p->case_id = case_id;
  p->funcs.setup = setup;
  p->funcs.run = run;
  p->funcs.teardown = teardown;
  p->funcs.complete_ide_stream = complete_ide_stream;

  case_funcs->cnt += 1;
  if(case_funcs->next == NULL) {
    case_funcs->test_category = TEEIO_TEST_CATEGORY_CXL_IDE;
    case_funcs->head = true;
  }
  
  while(case_funcs->next) {
    case_funcs = case_funcs->next;
  }

  case_funcs->next = p;  

  return true;
}

static bool _register_test_case_funcs(teeio_test_case_funcs_t* case_funcs)
{
  TEEIO_ASSERT(case_funcs);

  int cnt = case_funcs->cnt;

  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_TEST, 1, cxl_ide_test_full_ide_stream_setup, cxl_ide_test_full_ide_stream_run, cxl_ide_test_full_ide_stream_teardown, false);  // IdeStream

  TEEIO_DEBUG((TEEIO_DEBUG_INFO,"Register %d test_case funcs for CXL.memcache IDE.\n", case_funcs->cnt - cnt));

  return true;
}

// Register test_config
static bool _register_test_config_func(
  teeio_test_config_funcs_t* config_funcs,
  IDE_TEST_TOPOLOGY_TYPE top_type, IDE_TEST_CONFIGURATION_TYPE config_type,
  ide_common_test_config_enable_func_t enable,
  ide_common_test_config_disable_func_t disable,
  ide_common_test_config_support_func_t support,
  ide_common_test_config_check_func_t check
)
{
  teeio_test_config_funcs_t* p = (teeio_test_config_funcs_t *)malloc(sizeof(teeio_test_config_funcs_t));
  memset(p, 0, sizeof(teeio_test_config_funcs_t));

  p->test_category = TEEIO_TEST_CATEGORY_CXL_IDE;
  p->top_type = top_type;
  p->config_type = config_type;
  p->funcs.enable = enable;
  p->funcs.disable = disable;
  p->funcs.support = support;
  p->funcs.check = check;

  config_funcs->cnt += 1;
  if(config_funcs->next == NULL) {
    config_funcs->test_category = TEEIO_TEST_CATEGORY_CXL_IDE;
    config_funcs->head = true;
  }

  while(config_funcs->next) {
    config_funcs = config_funcs->next;
  }

  config_funcs->next = p;  

  return true;
}

static bool _register_test_config_funcs(teeio_test_config_funcs_t* config_funcs)
{
  TEEIO_ASSERT(config_funcs);

  int cnt = config_funcs->cnt;

  // Link IDE
  _register_test_config_func(
    config_funcs, IDE_TEST_TOPOLOGY_TYPE_LINK_IDE, IDE_TEST_CONFIGURATION_TYPE_DEFAULT,
    cxl_ide_test_config_default_enable, cxl_ide_test_config_default_disable,
    cxl_ide_test_config_default_support, cxl_ide_test_config_default_check
    );

  TEEIO_DEBUG((TEEIO_DEBUG_INFO,"Register %d test_config funcs for CXL.memcache IDE.\n", config_funcs->cnt - cnt));
  return true;
}

static bool _register_test_group_func(
  teeio_test_group_funcs_t* group_funcs,
  IDE_TEST_TOPOLOGY_TYPE top_type,
  ide_common_test_group_setup_func_t setup,
  ide_common_test_group_teardown_func_t teardown
)
{
  teeio_test_group_funcs_t* p = (teeio_test_group_funcs_t *)malloc(sizeof(teeio_test_group_funcs_t));
  memset(p, 0, sizeof(teeio_test_group_funcs_t));

  p->test_category = TEEIO_TEST_CATEGORY_CXL_IDE;
  p->top_type = top_type;
  p->funcs.setup = setup;
  p->funcs.teardown = teardown;

  group_funcs->cnt += 1;
  if(group_funcs->next == NULL) {
    group_funcs->test_category = TEEIO_TEST_CATEGORY_CXL_IDE;
    group_funcs->head = true;
  }

  while(group_funcs->next) {
    group_funcs = group_funcs->next;
  }

  group_funcs->next = p;  

  return true;
}

static bool _register_test_group_funcs(teeio_test_group_funcs_t* group_funcs)
{
  TEEIO_ASSERT(group_funcs);

  int cnt = group_funcs->cnt;

  _register_test_group_func(group_funcs, IDE_TEST_TOPOLOGY_TYPE_LINK_IDE, cxl_ide_test_group_setup, cxl_ide_test_group_teardown);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO,"Register %d test_group funcs for CXL.memcache IDE.\n", group_funcs->cnt - cnt));
  return true;
}

bool cxl_ide_test_lib_register_funcs(
  teeio_test_case_funcs_t* case_funcs,
  teeio_test_config_funcs_t* config_funcs,
  teeio_test_group_funcs_t* group_funcs)
{
  return _register_test_case_funcs(case_funcs) 
      && _register_test_config_funcs(config_funcs) 
      && _register_test_group_funcs(group_funcs);
}