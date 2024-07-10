/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>

#include "hal/base.h"
#include "hal/library/debuglib.h"

#include "ide_test.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "pcie_ide_lib.h"
#include "pcie_ide_test_lib.h"
#include "pcie_ide_test_common.h"

// test selective_ide and link_ide with default config
bool pcie_ide_test_config_default_enable_common(void *test_context)
{
  return pcie_ide_test_config_enable_common(test_context);
}

bool pcie_ide_test_config_default_disable_common(void *test_context)
{
  return true;
}

bool pcie_ide_test_config_default_support_common(void *test_context)
{
  return pcie_ide_test_config_support_common(test_context);
}

bool pcie_ide_test_config_default_check_common(void *test_context)
{
  return pcie_ide_test_config_check_common(test_context);
}
