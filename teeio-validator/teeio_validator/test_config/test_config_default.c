/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>

#include "assert.h"
#include "hal/base.h"
#include "hal/library/debuglib.h"
#include "hal/library/platform_lib.h"
#include "ide_test.h"
#include "utils.h"
#include "teeio_debug.h"

bool test_config_enable_common(void *test_context);
bool test_config_check_common(void *test_context, const char* assertion_msg);
bool test_config_support_common(void *test_context);

// test selective_ide and link_ide with default config
bool test_config_default_enable_common(void *test_context)
{
  // reset ide registers
  return test_config_enable_common(test_context);
}

bool test_config_default_disable_common(void *test_context)
{
  return true;
}

bool test_config_default_support_common(void *test_context)
{
  return test_config_support_common(test_context);
}

bool test_config_default_check_common(void *test_context)
{
  return test_config_check_common(test_context, "Default Config Assertion");
}

// test selective_and_link_ide with default config
bool test_config_default_enable_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return false;
}

bool test_config_default_disable_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return false;
}

bool test_config_default_support_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return false;
}

bool test_config_default_check_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return false;
}
