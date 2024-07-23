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

int pcie_ide_get_test_configuration_names(char*** config_names);

get_config_bitmask_func m_get_config_bitmask_funcs[TEEIO_TEST_CATEGORY_MAX] = {
  pcie_ide_test_lib_get_config_bitmask,
  NULL  // cxl-ide not supported
};

get_test_case_names_func m_get_test_case_names_funcs[TEEIO_TEST_CATEGORY_MAX] = {
  pcie_ide_test_lib_get_test_case_names,
  NULL  // cxl-ide not supported
};

teeio_test_case_funcs_t m_teeio_test_case_funcs = {0};
teeio_test_config_funcs_t m_teeio_test_config_funcs = {0};
teeio_test_group_funcs_t m_teeio_test_group_funcs = {0};

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

  return NULL;
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

ide_test_case_name_t* get_test_case_from_string(const char* test_case_name, int* index, TEEIO_TEST_CATEGORY test_category)
{
  if(test_case_name == NULL) {
    return NULL;
  }

  TEEIO_ASSERT(test_category < TEEIO_TEST_CATEGORY_MAX);

  ide_test_case_name_t* test_case_names = NULL;
  int test_case_names_cnt = 0;

  get_test_case_names_func func = m_get_test_case_names_funcs[test_category];
  TEEIO_ASSERT(func);
  test_case_names = func(&test_case_names_cnt);

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

bool is_valid_test_case(const char* test_case_name, TEEIO_TEST_CATEGORY test_category)
{
  return get_test_case_from_string(test_case_name, NULL, test_category) != NULL;
}

int get_test_case_names(ide_test_case_name_t** test_cases, TEEIO_TEST_CATEGORY test_category)
{
  int cnt = 0;
  if(test_category == TEEIO_TEST_CATEGORY_PCIE_IDE) {
    *test_cases = pcie_ide_test_lib_get_test_case_names(&cnt);
  } else {
    TEEIO_ASSERT(false);
  }

  return cnt;
}

int get_test_configuration_names(char*** config_names, TEEIO_TEST_CATEGORY test_category)
{
  int cnt = 0;
  if(test_category == TEEIO_TEST_CATEGORY_PCIE_IDE) {
    cnt = pcie_ide_get_test_configuration_names(config_names);
  } else {
    TEEIO_ASSERT(false);
  }
  
  return cnt;
}