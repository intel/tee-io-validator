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
#include "cxl_ide_lib.h"
#include "cxl_ide_test_common.h"

extern bool g_teeio_fixed_key;

bool cxl_ide_test_config_get_key_enable(void *test_context)
{
  return true;
}

bool cxl_ide_test_config_get_key_disable(void *test_context)
{
  return true;
}

bool cxl_ide_test_config_get_key_support(void *test_context)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  cxl_ide_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  CXL_PRIV_DATA* ep_cxl_data = &group_context->common.lower_port.cxl_data;

  CXL_QUERY_RESP_CAPS dev_caps = {.raw = ep_cxl_data->query_resp.caps};

  bool supported = dev_caps.ide_key_generation_capable && dev_caps.iv_generation_capable;
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "dev_caps: ide_key_generation_capable=%d, iv_generation_capable=%d\n",
                                dev_caps.ide_key_generation_capable, dev_caps.iv_generation_capable));

  if(g_teeio_fixed_key) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Tester intends to test with fixed IDE Key.\n"));
    supported = true;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_ide_test_config_get_key_support supported=%d\n", supported));
  return supported;
}

bool cxl_ide_test_config_get_key_check(void *test_context)
{
  return true;
}
