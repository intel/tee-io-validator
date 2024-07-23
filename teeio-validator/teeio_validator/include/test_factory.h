/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __IDE_FACTORY_H__
#define __IDE_FACTORY_H__

#include "ide_test.h"
#include "pcie_ide_test_lib.h"

typedef uint32_t(*get_config_bitmask_func) (int* config_type_num, IDE_TEST_TOPOLOGY_TYPE top_type);
typedef ide_test_case_name_t*(*get_test_case_names_func)(int *cnt);

bool test_factory_init();
bool test_factory_close();

ide_test_config_funcs_t*
test_factory_get_test_config_funcs (
  TEEIO_TEST_CATEGORY test_category,
  IDE_TEST_TOPOLOGY_TYPE top_type,
  IDE_TEST_CONFIGURATION_TYPE config_type);

ide_test_group_funcs_t*
test_factory_get_test_group_funcs (
  TEEIO_TEST_CATEGORY test_category,
  IDE_TEST_TOPOLOGY_TYPE top_type);

ide_test_case_funcs_t*
test_factory_get_test_case_funcs (
  TEEIO_TEST_CATEGORY test_category,
  int test_case,
  int case_id
);

uint32_t test_factory_get_config_bitmask(
  int* config_type_num,
  IDE_TEST_TOPOLOGY_TYPE top_type,
  TEEIO_TEST_CATEGORY test_category);

int get_test_case_names(
  ide_test_case_name_t** test_cases,
  TEEIO_TEST_CATEGORY test_category);

int get_test_configuration_names(
  char*** config_names,
  TEEIO_TEST_CATEGORY test_category);

#endif
