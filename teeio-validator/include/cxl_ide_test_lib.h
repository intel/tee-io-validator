/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __CXL_IDE_TEST_LIB_H__
#define __CXL_IDE_TEST_LIB_H__

#include "ide_test.h"

// pcie_ide_test_lib header file
bool cxl_ide_test_lib_register_funcs(
  teeio_test_case_funcs_t* case_funcs,
  teeio_test_config_funcs_t* config_funcs,
  teeio_test_group_funcs_t* group_funcs);

ide_test_case_name_t* cxl_ide_test_lib_get_test_case_names(int* cnt);

#endif
