/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "hal/base.h"
#include "ide_test.h"
#include "teeio_debug.h"
#include "pcie_ide_test_common.h"

#define PCIE_CLASS_CASE_NAMES "IdeStream,KeyRefresh"
ide_test_case_name_t m_pcie_ide_test_case_names[IDE_COMMON_TEST_CASE_NUM] = {
  {"Query",       "1,2",                  IDE_COMMON_TEST_CASE_QUERY        },
  {"KeyProg",     "1,2,3,4,5,6",          IDE_COMMON_TEST_CASE_KEYPROG      },
  {"KSetGo",      "1,2,3,4",              IDE_COMMON_TEST_CASE_KSETGO       },
  {"KSetStop",    "1,2,3,4",              IDE_COMMON_TEST_CASE_KSETSTOP     },
  {"SpdmSession", "1,2",                  IDE_COMMON_TEST_CASE_SPDMSESSION  },
  {"Test",        PCIE_CLASS_CASE_NAMES,  IDE_COMMON_TEST_CASE_TEST         }
};

uint32_t m_pcie_ide_top_config_bitmasks[] = {
  (uint32_t)SELECTIVE_IDE_CONFIGURATION_BITMASK,
  (uint32_t)LINK_IDE_CONFIGURATION_BITMASK,
  (uint32_t)SELECTIVE_LINK_IDE_CONFIGURATION_BITMASK
};

ide_test_case_name_t* pcie_ide_test_lib_get_test_case_names(int* cnt)
{
  *cnt = IDE_COMMON_TEST_CASE_NUM;
  return m_pcie_ide_test_case_names;
}

uint32_t pcie_ide_test_lib_get_config_bitmask(int* config_type_num, IDE_TEST_TOPOLOGY_TYPE top_type)
{
  TEEIO_ASSERT(config_type_num);
  TEEIO_ASSERT(top_type < IDE_TEST_TOPOLOGY_TYPE_NUM);

  *config_type_num = IDE_TEST_CONFIGURATION_TYPE_NUM;
  return m_pcie_ide_top_config_bitmasks[top_type];
}

/**
 * register test_case
 */
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

  p->test_category = IDE_TEST_CATEGORY_PCIE;
  p->test_case = test_case;
  p->case_id = case_id;
  p->funcs.setup = setup;
  p->funcs.run = run;
  p->funcs.teardown = teardown;
  p->funcs.complete_ide_stream = complete_ide_stream;

  case_funcs->cnt += 1;
  if(case_funcs->next == NULL) {
    case_funcs->test_category = IDE_TEST_CATEGORY_PCIE;
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

  // Query
  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_QUERY, 1, pcie_ide_test_query_1_setup, pcie_ide_test_query_1_run, pcie_ide_test_query_1_teardown, false);
  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_QUERY, 2, pcie_ide_test_query_2_setup, pcie_ide_test_query_2_run, pcie_ide_test_query_2_teardown, false);

  // KeyProg
  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_KEYPROG, 1, pcie_ide_test_keyprog_1_setup, pcie_ide_test_keyprog_1_run, pcie_ide_test_keyprog_1_teardown, false);
  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_KEYPROG, 2, pcie_ide_test_keyprog_2_setup, pcie_ide_test_keyprog_2_run, pcie_ide_test_keyprog_2_teardown, false);
  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_KEYPROG, 3, pcie_ide_test_keyprog_3_setup, pcie_ide_test_keyprog_3_run, pcie_ide_test_keyprog_3_teardown, false);
  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_KEYPROG, 4, pcie_ide_test_keyprog_4_setup, pcie_ide_test_keyprog_4_run, pcie_ide_test_keyprog_4_teardown, false);
  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_KEYPROG, 5, pcie_ide_test_keyprog_5_setup, pcie_ide_test_keyprog_5_run, pcie_ide_test_keyprog_5_teardown, false);
  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_KEYPROG, 6, pcie_ide_test_keyprog_6_setup, pcie_ide_test_keyprog_6_run, pcie_ide_test_keyprog_6_teardown, false);

  // KSetGo
  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_KSETGO, 1, pcie_ide_test_ksetgo_1_setup, pcie_ide_test_ksetgo_1_run, pcie_ide_test_ksetgo_1_teardown, true);
  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_KSETGO, 2, pcie_ide_test_ksetgo_2_setup, pcie_ide_test_ksetgo_2_run, pcie_ide_test_ksetgo_2_teardown, true);
  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_KSETGO, 3, pcie_ide_test_ksetgo_3_setup, pcie_ide_test_ksetgo_3_run, pcie_ide_test_ksetgo_3_teardown, true);
  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_KSETGO, 4, pcie_ide_test_ksetgo_4_setup, pcie_ide_test_ksetgo_4_run, pcie_ide_test_ksetgo_4_teardown, true);

  // KSetStop
  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_KSETSTOP, 1, pcie_ide_test_ksetstop_1_setup, pcie_ide_test_ksetstop_1_run, pcie_ide_test_ksetstop_1_teardown, false);
  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_KSETSTOP, 2, pcie_ide_test_ksetstop_2_setup, pcie_ide_test_ksetstop_2_run, pcie_ide_test_ksetstop_2_teardown, false);
  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_KSETSTOP, 3, pcie_ide_test_ksetstop_3_setup, pcie_ide_test_ksetstop_3_run, pcie_ide_test_ksetstop_3_teardown, false);
  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_KSETSTOP, 4, pcie_ide_test_ksetstop_4_setup, pcie_ide_test_ksetstop_4_run, pcie_ide_test_ksetstop_4_teardown, false);

  // SpdmSession

  // Test Full
  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_TEST, 1, pcie_ide_test_full_1_setup, pcie_ide_test_full_1_run, pcie_ide_test_full_1_teardown, false);  // IdeStream
  _register_test_case_func(case_funcs, IDE_COMMON_TEST_CASE_TEST, 2, pcie_ide_test_full_keyrefresh_setup, pcie_ide_test_full_keyrefresh_run, pcie_ide_test_full_keyrefresh_teardown, false);  // KeyRefresh

  TEEIO_DEBUG((TEEIO_DEBUG_INFO,"Register %d test_case funcs for PCIE IDE.\n", case_funcs->cnt - cnt));
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

  p->test_category = IDE_TEST_CATEGORY_PCIE;
  p->top_type = top_type;
  p->config_type = config_type;
  p->funcs.enable = enable;
  p->funcs.disable = disable;
  p->funcs.support = support;
  p->funcs.check = check;

  config_funcs->cnt += 1;
  if(config_funcs->next == NULL) {
    config_funcs->test_category = IDE_TEST_CATEGORY_PCIE;
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

  // Selective IDE
  _register_test_config_func(
    config_funcs, IDE_TEST_TOPOLOGY_TYPE_SEL_IDE, IDE_TEST_CONFIGURATION_TYPE_DEFAULT,
    pcie_ide_test_config_default_enable_common, pcie_ide_test_config_default_disable_common,
    pcie_ide_test_config_default_support_common, pcie_ide_test_config_default_check_common
    );

  _register_test_config_func(
    config_funcs, IDE_TEST_TOPOLOGY_TYPE_SEL_IDE, IDE_TEST_CONFIGURATION_TYPE_PCRC,
    pcie_ide_test_config_pcrc_enable_sel, pcie_ide_test_config_pcrc_disable_sel,
    pcie_ide_test_config_pcrc_support_sel, pcie_ide_test_config_pcrc_check_sel
    );

  _register_test_config_func(
    config_funcs, IDE_TEST_TOPOLOGY_TYPE_SEL_IDE, IDE_TEST_CONFIGURATION_TYPE_SELECTIVE_IDE_FOR_CONFIG,
    pcie_ide_test_config_enable_sel_ide_for_cfg_req, pcie_ide_test_config_disable_sel_ide_for_cfg_req,
    pcie_ide_test_config_support_sel_ide_for_cfg_req, pcie_ide_test_config_check_sel_ide_for_cfg_req
    );

  // Link IDE
  _register_test_config_func(
    config_funcs, IDE_TEST_TOPOLOGY_TYPE_LINK_IDE, IDE_TEST_CONFIGURATION_TYPE_DEFAULT,
    pcie_ide_test_config_default_enable_common, pcie_ide_test_config_default_disable_common,
    pcie_ide_test_config_default_support_common, pcie_ide_test_config_default_check_common
    );

  _register_test_config_func(
    config_funcs, IDE_TEST_TOPOLOGY_TYPE_LINK_IDE, IDE_TEST_CONFIGURATION_TYPE_DEFAULT,
      pcie_ide_test_config_pcrc_enable_link, pcie_ide_test_config_pcrc_disable_link,
      pcie_ide_test_config_pcrc_support_link, pcie_ide_test_config_pcrc_check_link
    );

  TEEIO_DEBUG((TEEIO_DEBUG_INFO,"Register %d test_config funcs for PCIE IDE.\n", config_funcs->cnt - cnt));
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

  p->test_category = IDE_TEST_CATEGORY_PCIE;
  p->top_type = top_type;
  p->funcs.setup = setup;
  p->funcs.teardown = teardown;

  group_funcs->cnt += 1;
  if(group_funcs->next == NULL) {
    group_funcs->test_category = IDE_TEST_CATEGORY_PCIE;
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

  _register_test_group_func(group_funcs, IDE_TEST_TOPOLOGY_TYPE_SEL_IDE, pcie_ide_test_group_setup_sel, pcie_ide_test_group_teardown_sel);
  _register_test_group_func(group_funcs, IDE_TEST_TOPOLOGY_TYPE_LINK_IDE, pcie_ide_test_group_setup_link, pcie_ide_test_group_teardown_link);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO,"Register %d test_group funcs for PCIE IDE.\n", group_funcs->cnt - cnt));
  return true;
}

bool pcie_ide_test_lib_register_funcs(
  teeio_test_case_funcs_t* case_funcs,
  teeio_test_config_funcs_t* config_funcs,
  teeio_test_group_funcs_t* group_funcs)
{
  return _register_test_case_funcs(case_funcs) 
      && _register_test_config_funcs(config_funcs) 
      && _register_test_group_funcs(group_funcs);
}
