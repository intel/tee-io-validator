/**
 *  Copyright Notice:
 *  Copyright 2023-2025 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "ide_test.h"
#include "teeio_debug.h"
#include "pcie_ide_common.h"

// PCIE-IDE supported config items
const char* m_ide_test_configuration_name[] = {
  "default",
  "switch",
  "partial_header_encryption",
  "pcrc",
  "aggregation",
  "selective_ide_for_configuration",
  "tee_limited_stream",
  "flit_mode_disable",
  NULL
};

uint32_t m_top_config_bitmasks[IDE_TEST_TOPOLOGY_TYPE_NUM] = {
  (uint32_t)SELECTIVE_IDE_CONFIGURATION_BITMASK,
  (uint32_t)LINK_IDE_CONFIGURATION_BITMASK,
  (uint32_t)SELECTIVE_LINK_IDE_CONFIGURATION_BITMASK
};

ide_test_config_funcs_t m_config_funcs[IDE_TEST_TOPOLOGY_TYPE_NUM][IDE_TEST_CONFIGURATION_TYPE_NUM] = {
  { // selective_ide
    { // Default Config
      pcie_ide_test_config_default_enable_common,
      pcie_ide_test_config_default_disable_common,
      pcie_ide_test_config_default_support_common,
      pcie_ide_test_config_default_check_common
    },
    {NULL, NULL, NULL, NULL}, // switch
    {NULL, NULL, NULL, NULL}, // partial header encryption
    { // pcrc
      pcie_ide_test_config_pcrc_enable_sel,
      pcie_ide_test_config_pcrc_disable_sel,
      pcie_ide_test_config_pcrc_support_sel,
      pcie_ide_test_config_pcrc_check_sel
    },
    {NULL, NULL, NULL, NULL}, // aggregation
    { // selective_ide for configuration request
      pcie_ide_test_config_enable_sel_ide_for_cfg_req,
      pcie_ide_test_config_disable_sel_ide_for_cfg_req,
      pcie_ide_test_config_support_sel_ide_for_cfg_req,
      pcie_ide_test_config_check_sel_ide_for_cfg_req
    },
    {NULL, NULL, NULL, NULL},  // tee_limited_stream
    { // flit mode disable
      pcie_ide_test_config_flit_mode_disable_enable_sel,
      pcie_ide_test_config_flit_mode_disable_disable_sel,
      pcie_ide_test_config_flit_mode_disable_support_sel,
      pcie_ide_test_config_flit_mode_disable_check_sel
    },
  },
  { // link_ide
    {
      // Default Config
      pcie_ide_test_config_default_enable_common,
      pcie_ide_test_config_default_disable_common,
      pcie_ide_test_config_default_support_common,
      pcie_ide_test_config_default_check_common
    },

    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},

    {
      // pcrc
      pcie_ide_test_config_pcrc_enable_link,
      pcie_ide_test_config_pcrc_disable_link,
      pcie_ide_test_config_pcrc_support_link,
      pcie_ide_test_config_pcrc_check_link
    },

    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    { // flit mode disable
      pcie_ide_test_config_flit_mode_disable_enable_link,
      pcie_ide_test_config_flit_mode_disable_disable_link,
      pcie_ide_test_config_flit_mode_disable_support_link,
      pcie_ide_test_config_flit_mode_disable_check_link
    },
  },
  { // selective_and_link_ide
    {
      // Default Config
      pcie_ide_test_config_default_enable_sel_link,
      pcie_ide_test_config_default_disable_sel_link,
      pcie_ide_test_config_default_support_sel_link,
      pcie_ide_test_config_default_check_sel_link
    },
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {
      // pcrc
      pcie_ide_test_config_pcrc_enable_sel_link,
      pcie_ide_test_config_pcrc_disable_sel_link,
      pcie_ide_test_config_pcrc_support_sel_link,
      pcie_ide_test_config_pcrc_check_sel_link
    },
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    { // flit mode disable
      pcie_ide_test_config_flit_mode_disable_enable_sel_link,
      pcie_ide_test_config_flit_mode_disable_disable_sel_link,
      pcie_ide_test_config_flit_mode_disable_support_sel_link,
      pcie_ide_test_config_flit_mode_disable_check_sel_link
    },
  }
};

ide_test_group_funcs_t m_group_funcs[IDE_TEST_TOPOLOGY_TYPE_NUM] = {
  { // selective_ide
      pcie_ide_test_group_setup_sel,
      pcie_ide_test_group_teardown_sel
  },
  { // link_ide
      pcie_ide_test_group_setup_link,
      pcie_ide_test_group_teardown_link
  },
  { // selective_link_ide
      pcie_ide_test_group_setup_sel_link,
      pcie_ide_test_group_teardown_sel_link
  }
};

#define TEST_CLASS_CASE_NAMES "IdeStream,KeyRefresh"

ide_test_case_name_t m_test_case_names[] = {
  {"Query",       "1,2",                  IDE_COMMON_TEST_CASE_QUERY},
  {"KeyProg",     "1,2,3,4,5,6",          IDE_COMMON_TEST_CASE_KEYPROG},
  {"KSetGo",      "1,2,3,4",              IDE_COMMON_TEST_CASE_KSETGO},
  {"KSetStop",    "1,2,3,4",              IDE_COMMON_TEST_CASE_KSETSTOP},
  {"SpdmSession", "1,2",                  IDE_COMMON_TEST_CASE_SPDMSESSION},
  {"Test",        TEST_CLASS_CASE_NAMES,  IDE_COMMON_TEST_CASE_TEST},
  {NULL,          NULL,                   IDE_COMMON_TEST_CASE_NUM}
};

ide_test_case_funcs_t m_pcie_ide_query_cases[MAX_QUERY_CASE_ID] = {
  { pcie_ide_test_query_1_setup, pcie_ide_test_query_1_run, pcie_ide_test_query_1_teardown, false },
  { pcie_ide_test_query_2_setup, pcie_ide_test_query_2_run, pcie_ide_test_query_2_teardown, false }
};

ide_test_case_funcs_t m_pcie_ide_key_prog_cases[MAX_KEYPROG_CASE_ID] = {
  {pcie_ide_test_keyprog_1_setup, pcie_ide_test_keyprog_1_run, pcie_ide_test_keyprog_1_teardown, false},
  {pcie_ide_test_keyprog_2_setup, pcie_ide_test_keyprog_2_run, pcie_ide_test_keyprog_2_teardown, false},
  {pcie_ide_test_keyprog_3_setup, pcie_ide_test_keyprog_3_run, pcie_ide_test_keyprog_3_teardown, false},
  {pcie_ide_test_keyprog_4_setup, pcie_ide_test_keyprog_4_run, pcie_ide_test_keyprog_4_teardown, false},
  {pcie_ide_test_keyprog_5_setup, pcie_ide_test_keyprog_5_run, pcie_ide_test_keyprog_5_teardown, false},
  {pcie_ide_test_keyprog_6_setup, pcie_ide_test_keyprog_6_run, pcie_ide_test_keyprog_6_teardown, false}
};

ide_test_case_funcs_t m_pcie_ide_kset_go_cases[MAX_KSETGO_CASE_ID] = {
  { pcie_ide_test_ksetgo_1_setup, pcie_ide_test_ksetgo_1_run, pcie_ide_test_ksetgo_1_teardown, true },
  { pcie_ide_test_ksetgo_2_setup, pcie_ide_test_ksetgo_2_run, pcie_ide_test_ksetgo_2_teardown, true },
  { pcie_ide_test_ksetgo_3_setup, pcie_ide_test_ksetgo_3_run, pcie_ide_test_ksetgo_3_teardown, true },
  { pcie_ide_test_ksetgo_4_setup, pcie_ide_test_ksetgo_4_run, pcie_ide_test_ksetgo_4_teardown, true }
};

ide_test_case_funcs_t m_pcie_ide_kset_stop_cases[MAX_KSETSTOP_CASE_ID] = {
  { pcie_ide_test_ksetstop_1_setup, pcie_ide_test_ksetstop_1_run, pcie_ide_test_ksetstop_1_teardown, false },
  { pcie_ide_test_ksetstop_2_setup, pcie_ide_test_ksetstop_2_run, pcie_ide_test_ksetstop_2_teardown, false },
  { pcie_ide_test_ksetstop_3_setup, pcie_ide_test_ksetstop_3_run, pcie_ide_test_ksetstop_3_teardown, false },
  { pcie_ide_test_ksetstop_4_setup, pcie_ide_test_ksetstop_4_run, pcie_ide_test_ksetstop_4_teardown, false }
};

ide_test_case_funcs_t m_pcie_ide_spdm_session_cases[MAX_SPDMSESSION_CASE_ID] = {
  {pcie_ide_test_spdm_session_1_setup, pcie_ide_test_spdm_session_1_run, pcie_ide_test_spdm_session_1_teardown, false},
  {pcie_ide_test_spdm_session_2_setup, pcie_ide_test_spdm_session_2_run, pcie_ide_test_spdm_session_2_teardown, false},
};

ide_test_case_funcs_t m_pcie_ide_test_full_cases[MAX_FULL_CASE_ID] = {
  { pcie_ide_test_full_1_setup, pcie_ide_test_full_1_run, pcie_ide_test_full_1_teardown, false },  // IdeStream
  { pcie_ide_test_full_keyrefresh_setup, pcie_ide_test_full_keyrefresh_run, pcie_ide_test_full_keyrefresh_teardown, false }  // KeyRefresh
};

TEEIO_TEST_CASES m_pcie_ide_test_case_funcs[IDE_COMMON_TEST_CASE_NUM] = {
  {m_pcie_ide_query_cases,          MAX_QUERY_CASE_ID},
  {m_pcie_ide_key_prog_cases,       MAX_KEYPROG_CASE_ID},
  {m_pcie_ide_kset_go_cases,        MAX_KSETGO_CASE_ID},
  {m_pcie_ide_kset_stop_cases,      MAX_KSETSTOP_CASE_ID},
  {m_pcie_ide_spdm_session_cases,   MAX_SPDMSESSION_CASE_ID},
  {m_pcie_ide_test_full_cases,      MAX_FULL_CASE_ID}
};

static const char* get_test_configuration_name (int configuration_type)
{
  if(configuration_type > sizeof(m_ide_test_configuration_name)/sizeof(const char*)) {
    return NULL;
  }

  return m_ide_test_configuration_name[configuration_type];
}

static uint32_t get_test_configuration_bitmask (int top_tpye)
{
  TEEIO_ASSERT(top_tpye < IDE_TEST_TOPOLOGY_TYPE_NUM);
  return m_top_config_bitmasks[top_tpye];
}

static ide_test_config_funcs_t* get_test_configuration_funcs (int top_type, int configuration_type)
{
  TEEIO_ASSERT(top_type < IDE_TEST_TOPOLOGY_TYPE_NUM && configuration_type < IDE_TEST_CONFIGURATION_TYPE_NUM);
  return &m_config_funcs[top_type][configuration_type];
}

static ide_test_group_funcs_t* get_test_group_funcs (int top_type)
{
  TEEIO_ASSERT(top_type < IDE_TEST_TOPOLOGY_TYPE_NUM);
  return &m_group_funcs[top_type];
}

static ide_test_case_funcs_t* get_test_case_funcs (int case_class, int case_id)
{
  TEEIO_ASSERT(case_class < IDE_COMMON_TEST_CASE_NUM);
  TEEIO_TEST_CASES* test_cases = &m_pcie_ide_test_case_funcs[case_class];

  TEEIO_ASSERT(case_id < test_cases->cnt);
  return &test_cases->funcs[case_id];
}

static ide_test_case_name_t* get_test_case_name (int case_class)
{
  TEEIO_ASSERT(case_class < IDE_COMMON_TEST_CASE_NUM + 1);
  return &m_test_case_names[case_class];
}

static void* alloc_pcie_ide_test_group_context(void)
{
  pcie_ide_test_group_context_t* context = (pcie_ide_test_group_context_t*)malloc(sizeof(pcie_ide_test_group_context_t));
  TEEIO_ASSERT(context);
  memset(context, 0, sizeof(pcie_ide_test_group_context_t));
  context->common.signature = GROUP_CONTEXT_SIGNATURE;

  return context;
}

static bool pcie_ide_check_configuration_bitmap(uint32_t* bitmap)
{
  // default config is always set
  *bitmap |= BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_DEFAULT);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "pcie-ide configuration bitmap=0x%08x\n", *bitmap));

  return true;
}

bool pcie_ide_test_lib_register_test_suite_funcs(teeio_test_funcs_t* funcs)
{
  TEEIO_ASSERT(funcs);

  funcs->get_case_funcs_func = get_test_case_funcs;
  funcs->get_case_name_func = get_test_case_name;
  funcs->get_configuration_bitmask_func = get_test_configuration_bitmask;
  funcs->get_configuration_funcs_func = get_test_configuration_funcs;
  funcs->get_configuration_name_func = get_test_configuration_name;
  funcs->get_group_funcs_func = get_test_group_funcs;
  funcs->alloc_test_group_context_func = alloc_pcie_ide_test_group_context;
  funcs->check_configuration_bitmap_func = pcie_ide_check_configuration_bitmap;

  return true;
}