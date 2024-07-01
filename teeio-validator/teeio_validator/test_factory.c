/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <string.h>
#include "ide_test.h"
#include "helperlib.h"
#include "test_factory.h"

#define PCIE_CLASS_CASE_NAMES "IdeStream,KeyRefresh"
#define CXL_MEM_CLASS_CASE_NAMES "IdeStream"

extern const char *IDE_HW_TYPE_NAMES[];

ide_test_case_name_t m_pcie_ide_test_case_names[IDE_COMMON_TEST_CASE_NUM] = {
  {"Query",       "1,2",                  IDE_COMMON_TEST_CASE_QUERY        , IDE_HW_TYPE_PCIE},
  {"KeyProg",     "1,2,3,4,5,6",          IDE_COMMON_TEST_CASE_KEYPROG      , IDE_HW_TYPE_PCIE},
  {"KSetGo",      "1,2,3,4",              IDE_COMMON_TEST_CASE_KSETGO       , IDE_HW_TYPE_PCIE},
  {"KSetStop",    "1,2,3,4",              IDE_COMMON_TEST_CASE_KSETSTOP     , IDE_HW_TYPE_PCIE},
  {"SpdmSession", "1,2",                  IDE_COMMON_TEST_CASE_SPDMSESSION  , IDE_HW_TYPE_PCIE},
  {"Test",        PCIE_CLASS_CASE_NAMES,  IDE_COMMON_TEST_CASE_TEST         , IDE_HW_TYPE_PCIE}
};

ide_test_case_name_t m_cxl_mem_ide_test_case_names[CXL_MEM_IDE_TEST_CASE_NUM] = {
  {"Query",       "1,2",                    CXL_MEM_IDE_TEST_CASE_QUERY     , IDE_HW_TYPE_CXL_MEM},
  {"KeyProg",     "1,2,3,4,5,6,7,8,9",      CXL_MEM_IDE_TEST_CASE_KEYPROG   , IDE_HW_TYPE_CXL_MEM},
  {"KSetGo",      "1",                      CXL_MEM_IDE_TEST_CASE_KSETGO    , IDE_HW_TYPE_CXL_MEM},
  {"KSetStop",    "1",                      CXL_MEM_IDE_TEST_CASE_KSETSTOP  , IDE_HW_TYPE_CXL_MEM},
  {"GetKey",      "1",                      CXL_MEM_IDE_TEST_CASE_GETKEY    , IDE_HW_TYPE_CXL_MEM},
  {"Test",        CXL_MEM_CLASS_CASE_NAMES, CXL_MEM_IDE_TEST_CASE_TEST      , IDE_HW_TYPE_CXL_MEM}
};

ide_test_case_name_t* get_test_case_from_string(const char* test_case_name, int* index, IDE_HW_TYPE ide_type)
{
  if(test_case_name == NULL) {
    return NULL;
  }

  TEEIO_ASSERT(ide_type < IDE_HW_TYPE_NUM);

  ide_test_case_name_t* test_case_names = NULL;
  int test_case_names_cnt = 0;

  if(ide_type == IDE_HW_TYPE_PCIE) {
    test_case_names = m_pcie_ide_test_case_names;
    test_case_names_cnt = IDE_COMMON_TEST_CASE_NUM;
  } else if(ide_type == IDE_HW_TYPE_CXL_MEM) {
    test_case_names = m_cxl_mem_ide_test_case_names;
    test_case_names_cnt = CXL_MEM_IDE_TEST_CASE_NUM;
  } else {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s is not supported.\n", IDE_HW_TYPE_NAMES[(int)ide_type]));
    return NULL;
  }

  char buf1[MAX_LINE_LENGTH] = {0};
  char buf2[MAX_LINE_LENGTH] = {0};
  strncpy(buf1, test_case_name, MAX_LINE_LENGTH);

  int pos = find_char_in_str(buf1, '.');
  if(pos == -1) {
    return NULL;
  }

  buf1[pos] = '\0';
  char* ptr1 = buf1 + pos + 1;

  int i = 0;
  for(; i < test_case_names_cnt; i++) {
    if(strcmp(buf1, test_case_names[i].class) == 0) {
      break;
    }
  }
  if(i == test_case_names_cnt) {
    return NULL;
  }

  bool hit = false;
  strncpy(buf2, test_case_names[i].names, MAX_LINE_LENGTH);
  char *ptr2 = buf2;
  int j = 0;

  pos = find_char_in_str(ptr2, ',');

  do {
    if(pos != -1) {
      ptr2[pos] = '\0';
    }
    if(strcmp(ptr1, ptr2) == 0) {
      hit = true;
      break;
    }

    if(pos == -1) {
      break;
    }

    ptr2 += (pos + 1);
    pos = find_char_in_str(ptr2, ',');
    j++;
  } while(true);

  if(index != NULL) {
    *index = j;
  }

  return hit ? test_case_names + i : NULL;
}

bool is_valid_test_case(const char* test_case_name, IDE_HW_TYPE ide_type)
{
  return get_test_case_from_string(test_case_name, NULL, ide_type) != NULL;
}

ide_test_config_funcs_t m_cxl_ide_config_funcs[CXL_IDE_CONFIGURATION_TYPE_NUM] = {
  { // Default Config
    cxl_ide_test_config_default_enable,
    cxl_ide_test_config_default_disable,
    cxl_ide_test_config_default_support,
    cxl_ide_test_config_default_check
  },
  {},   // pcrc
  {}    // ide.stop
};

ide_test_group_funcs_t m_cxl_ide_group_funcs = {
  cxl_ide_test_group_setup,
  cxl_ide_test_group_teardown
};

ide_test_case_funcs_t m_cxl_ide_test_case_funcs[CXL_MEM_IDE_TEST_CASE_NUM][MAX_CXL_CASE_ID] = {
  // Query
  {},
  // KeyProg
  {},
  // KSetGo
  {},
  // KSetStop
  {},
  // GetKey
  {},
  // Test
  {
    { cxl_ide_test_full_ide_stream_setup, cxl_ide_test_full_ide_stream_run, cxl_ide_test_full_ide_stream_teardown, false }, // IdeStream
    {}
  }
};

ide_test_config_funcs_t m_pcie_ide_config_funcs[IDE_TEST_TOPOLOGY_TYPE_NUM][IDE_TEST_CONFIGURATION_TYPE_NUM] = {
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
    {NULL, NULL, NULL, NULL}  // tee_limited_stream
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
    {NULL, NULL, NULL, NULL}
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
    {NULL, NULL, NULL, NULL}
  }
};

ide_test_group_funcs_t m_pcie_ide_group_funcs[] = {
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

ide_test_case_funcs_t m_pcie_ide_test_case_funcs[IDE_COMMON_TEST_CASE_NUM][MAX_PCIE_CASE_ID] = {
  // Query
  {
    { pcie_ide_test_query_1_setup, pcie_ide_test_query_1_run, pcie_ide_test_query_1_teardown, false },
    { pcie_ide_test_query_2_setup, pcie_ide_test_query_2_run, pcie_ide_test_query_2_teardown, false },
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false},
  },
  // KeyProg
  {
    {pcie_ide_test_keyprog_1_setup, pcie_ide_test_keyprog_1_run, pcie_ide_test_keyprog_1_teardown, false},
    {pcie_ide_test_keyprog_2_setup, pcie_ide_test_keyprog_2_run, pcie_ide_test_keyprog_2_teardown, false},
    {pcie_ide_test_keyprog_3_setup, pcie_ide_test_keyprog_3_run, pcie_ide_test_keyprog_3_teardown, false},
    {pcie_ide_test_keyprog_4_setup, pcie_ide_test_keyprog_4_run, pcie_ide_test_keyprog_4_teardown, false},
    {pcie_ide_test_keyprog_5_setup, pcie_ide_test_keyprog_5_run, pcie_ide_test_keyprog_5_teardown, false},
    {pcie_ide_test_keyprog_6_setup, pcie_ide_test_keyprog_6_run, pcie_ide_test_keyprog_6_teardown, false}
  },
  // KSetGo
  {
    { pcie_ide_test_ksetgo_1_setup, pcie_ide_test_ksetgo_1_run, pcie_ide_test_ksetgo_1_teardown, true },
    { pcie_ide_test_ksetgo_2_setup, pcie_ide_test_ksetgo_2_run, pcie_ide_test_ksetgo_2_teardown, true },
    { pcie_ide_test_ksetgo_3_setup, pcie_ide_test_ksetgo_3_run, pcie_ide_test_ksetgo_3_teardown, true },
    { pcie_ide_test_ksetgo_4_setup, pcie_ide_test_ksetgo_4_run, pcie_ide_test_ksetgo_4_teardown, true },
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false}
  },
  // KSetStop
  {
    { pcie_ide_test_ksetstop_1_setup, pcie_ide_test_ksetstop_1_run, pcie_ide_test_ksetstop_1_teardown, false },
    { pcie_ide_test_ksetstop_2_setup, pcie_ide_test_ksetstop_2_run, pcie_ide_test_ksetstop_2_teardown, false },
    { pcie_ide_test_ksetstop_3_setup, pcie_ide_test_ksetstop_3_run, pcie_ide_test_ksetstop_3_teardown, false },
    { pcie_ide_test_ksetstop_4_setup, pcie_ide_test_ksetstop_4_run, pcie_ide_test_ksetstop_4_teardown, false },
    {NULL, NULL, NULL, false},
    {NULL, NULL, NULL, false}
  },
  // SpdmSession
  {
    {NULL, NULL, NULL},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL}
  },
  // Test Full
  {
    { pcie_ide_test_full_1_setup, pcie_ide_test_full_1_run, pcie_ide_test_full_1_teardown, false },  // IdeStream
    { pcie_ide_test_full_keyrefresh_setup, pcie_ide_test_full_keyrefresh_run, pcie_ide_test_full_keyrefresh_teardown, false },  // KeyRefresh
    {NULL, NULL, NULL},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL}
  }
};

ide_test_config_funcs_t*
test_factory_get_test_config_funcs (
  IDE_HW_TYPE ide,
  IDE_TEST_TOPOLOGY_TYPE top_type,
  IDE_TEST_CONFIGURATION_TYPE config_type)
{
  if(ide == IDE_HW_TYPE_PCIE) {
    return &m_pcie_ide_config_funcs[top_type][config_type];
  } else if(ide == IDE_HW_TYPE_CXL_MEM) {
    return &m_cxl_ide_config_funcs[config_type];
  } else {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s is not supported yet.\n", IDE_HW_TYPE_NAMES[ide]));
    return NULL;
  }
}

ide_test_group_funcs_t*
test_factory_get_test_group_funcs (
  IDE_HW_TYPE ide,
  IDE_TEST_TOPOLOGY_TYPE top_type
)
{
  if(ide == IDE_HW_TYPE_PCIE) {
    return &m_pcie_ide_group_funcs[top_type];
  } else if(ide == IDE_HW_TYPE_CXL_MEM) {
    // CXL only supports selective_ide
    TEEIO_ASSERT(top_type == IDE_TEST_TOPOLOGY_TYPE_SEL_IDE);
    return &m_cxl_ide_group_funcs;
  } else {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s is not supported yet.\n", IDE_HW_TYPE_NAMES[ide]));
    return NULL;
  }
}

ide_test_case_funcs_t*
test_factory_get_test_case_funcs (
  IDE_HW_TYPE ide,
  IDE_COMMON_TEST_CASE test_case,
  int case_id
)
{
  if(ide == IDE_HW_TYPE_PCIE) {
    return &m_pcie_ide_test_case_funcs[test_case][case_id];
  } else if(ide == IDE_HW_TYPE_CXL_MEM) {
    return &m_cxl_ide_test_case_funcs[test_case][case_id];
  } else {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s is not supported yet.\n", IDE_HW_TYPE_NAMES[ide]));
    return NULL;
  }
}

bool not_supported_ide_test_config_common_func(void *test_context)
{
  return false;
}

ide_common_test_config_enable_func_t
test_factory_get_common_test_config_enable_func(
  IDE_HW_TYPE ide
)
{
  if(ide == IDE_HW_TYPE_PCIE) {
    return pcie_ide_test_config_enable_common;
  } else if(ide == IDE_HW_TYPE_CXL_MEM) {
    return cxl_ide_test_config_enable_common;
  } else {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s is not supported yet.\n", IDE_HW_TYPE_NAMES[ide]));
    return not_supported_ide_test_config_common_func;
  }
}

ide_common_test_config_check_func_t
test_factory_get_common_test_config_check_func(
  IDE_HW_TYPE ide
)
{
  if(ide == IDE_HW_TYPE_PCIE) {
    return pcie_ide_test_config_check_common;
  } else if(ide == IDE_HW_TYPE_CXL_MEM) {
    return cxl_ide_test_config_check_common;
  } else {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s is not supported yet.\n", IDE_HW_TYPE_NAMES[ide]));
    return not_supported_ide_test_config_common_func;
  }
}

ide_common_test_config_support_func_t
test_factory_get_common_test_config_support_func(
  IDE_HW_TYPE ide
)
{
  if(ide == IDE_HW_TYPE_PCIE) {
    return pcie_ide_test_config_support_common;
  } else if(ide == IDE_HW_TYPE_CXL_MEM) {
    return cxl_ide_test_config_support_common;
  } else {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s is not supported yet.\n", IDE_HW_TYPE_NAMES[ide]));
    return not_supported_ide_test_config_common_func;
  }
}
