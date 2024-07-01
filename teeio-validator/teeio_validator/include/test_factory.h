/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __IDE_FACTORY_H__
#define __IDE_FACTORY_H__

#include "ide_test.h"
#include "pcie_ide_test_lib.h"
#include "cxl_ide_test_lib.h"

ide_test_config_funcs_t*
test_factory_get_test_config_funcs (
  IDE_HW_TYPE ide,
  IDE_TEST_TOPOLOGY_TYPE top_type,
  IDE_TEST_CONFIGURATION_TYPE config_type);

ide_test_group_funcs_t*
test_factory_get_test_group_funcs (
  IDE_HW_TYPE ide,
  IDE_TEST_TOPOLOGY_TYPE top_type
);

ide_test_case_funcs_t*
test_factory_get_test_case_funcs (
  IDE_HW_TYPE ide,
  IDE_COMMON_TEST_CASE test_case,
  int case_id
);

ide_common_test_config_enable_func_t
test_factory_get_common_test_config_enable_func(
  IDE_HW_TYPE ide
);

ide_common_test_config_check_func_t
test_factory_get_common_test_config_check_func(
  IDE_HW_TYPE ide
);

ide_common_test_config_support_func_t
test_factory_get_common_test_config_support_func(
  IDE_HW_TYPE ide
);

#endif
