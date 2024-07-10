/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <string.h>
#include <stdlib.h>
#include "ide_test.h"
#include "helperlib.h"
#include "test_factory.h"
#include "pcie_ide_test_lib.h"

extern const char *TEEIO_TEST_CATEGORY_NAMES[];
extern const char *IDE_TEST_TOPOLOGY_TYPE_NAMES[];

typedef uint32_t(*get_config_bitmask_func) (int* config_type_num, IDE_TEST_TOPOLOGY_TYPE top_type);
get_config_bitmask_func m_get_config_bitmask_funcs[TEEIO_TEST_CATEGORY_MAX] = {
  pcie_ide_test_lib_get_config_bitmask,
  NULL  // cxl.memcache not supported
};

teeio_test_case_funcs_t m_teeio_test_case_funcs = {0};
teeio_test_config_funcs_t m_teeio_test_config_funcs = {0};
teeio_test_group_funcs_t m_teeio_test_group_funcs = {0};

const char* PCIE_IDE_TEST_CASE_NAMES[IDE_COMMON_TEST_CASE_NUM] = {
  "Query",
  "KeyProg",
  "KSetGo",
  "KSetStop",
  "SpdmSession",
  "TestFull"
};

bool test_factory_init()
{
  pcie_ide_test_lib_register_funcs(&m_teeio_test_case_funcs, &m_teeio_test_config_funcs, &m_teeio_test_group_funcs);

  return true;
}

bool test_factory_close()
{
  teeio_test_case_funcs_t* itr1 = m_teeio_test_case_funcs.next;
  teeio_test_case_funcs_t* next1 = NULL;
  while(itr1) {
    next1 = itr1->next;
    free(itr1);
    itr1 = next1;
  }
  memset(&m_teeio_test_case_funcs, 0, sizeof(teeio_test_case_funcs_t));

  teeio_test_group_funcs_t* itr2 = m_teeio_test_group_funcs.next;
  teeio_test_group_funcs_t* next2 = NULL;
  while(itr2) {
    next2 = itr2->next;
    free(itr2);
    itr2 = next2;
  }
  memset(&m_teeio_test_group_funcs, 0, sizeof(teeio_test_group_funcs_t));

  teeio_test_config_funcs_t* itr3 = m_teeio_test_config_funcs.next;
  teeio_test_config_funcs_t* next3 = NULL;
  while(itr3) {
    next3 = itr3->next;
    free(itr3);
    itr3 = next3;
  }
  memset(&m_teeio_test_config_funcs, 0, sizeof(teeio_test_config_funcs_t));

  return true;
}

ide_test_config_funcs_t*
test_factory_get_test_config_funcs (
  TEEIO_TEST_CATEGORY test_category,
  IDE_TEST_TOPOLOGY_TYPE top_type,
  IDE_TEST_CONFIGURATION_TYPE config_type)
{
  teeio_test_config_funcs_t* itr = m_teeio_test_config_funcs.next;
  while(itr) {
    if(itr->test_category == test_category && itr->top_type == top_type && itr->config_type == config_type) {
      return &itr->funcs;
    }
    itr = itr->next;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Cannot find test_config funcs for [%s][%s][config:%d]\n",
    TEEIO_TEST_CATEGORY_NAMES[test_category],
    IDE_TEST_TOPOLOGY_TYPE_NAMES[top_type],
    config_type));
  return NULL;
}

ide_test_group_funcs_t*
test_factory_get_test_group_funcs (
  TEEIO_TEST_CATEGORY test_category,
  IDE_TEST_TOPOLOGY_TYPE top_type
)
{
  teeio_test_group_funcs_t* itr = m_teeio_test_group_funcs.next;
  while(itr) {
    if(itr->test_category == test_category && itr->top_type == top_type) {
      return &itr->funcs;
    }
    itr = itr->next;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Cannot find test_group funcs for [%s][%s]\n",
    TEEIO_TEST_CATEGORY_NAMES[test_category],
    IDE_TEST_TOPOLOGY_TYPE_NAMES[top_type]));

  return NULL;
}

ide_test_case_funcs_t*
test_factory_get_test_case_funcs (
  TEEIO_TEST_CATEGORY test_category,
  int test_case,
  int case_id
)
{
  teeio_test_case_funcs_t* itr = m_teeio_test_case_funcs.next;
  while(itr) {
    if(itr->test_category == test_category && itr->test_case == test_case && itr->case_id == case_id) {
      return &itr->funcs;
    }
    itr = itr->next;
  }

  const char* case_name = NULL;
  if(test_category == TEEIO_TEST_CATEGORY_PCIE_IDE) {
    TEEIO_ASSERT(test_case < IDE_COMMON_TEST_CASE_NUM);
    case_name = PCIE_IDE_TEST_CASE_NAMES[test_case];
  } else {
    TEEIO_ASSERT(false);
  }

  TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Cannot find test_case funcs for [%s][%s][id:%d]\n",
    TEEIO_TEST_CATEGORY_NAMES[test_category], case_name, case_id));

  return NULL;
}

bool not_supported_ide_test_config_common_func(void *test_context)
{
  return false;
}

uint32_t test_factory_get_config_bitmask(
  int* config_type_num,
  IDE_TEST_TOPOLOGY_TYPE top_type,
  TEEIO_TEST_CATEGORY test_category)
{
  uint32_t bitmask = 0;

  TEEIO_ASSERT(config_type_num);
  *config_type_num = 0;

  TEEIO_ASSERT(test_category < TEEIO_TEST_CATEGORY_MAX);

  get_config_bitmask_func func = m_get_config_bitmask_funcs[test_category];
  TEEIO_ASSERT(func);

  bitmask = func(config_type_num, top_type);

  return bitmask;
}
