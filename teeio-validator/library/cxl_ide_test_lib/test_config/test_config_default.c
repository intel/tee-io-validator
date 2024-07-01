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
#include "cxl_ide_test_lib.h"

// test selective_ide and link_ide with default config
bool cxl_ide_test_config_default_enable(void *test_context)
{
  return true;
}

bool cxl_ide_test_config_default_disable(void *test_context)
{
  return true;
}

bool cxl_ide_test_config_default_support(void *test_context)
{
  return true;
}

bool cxl_ide_test_config_default_check(void *test_context)
{
  return true;
}

// common function
bool cxl_ide_test_config_enable_common(void *test_context)
{
  return true;
}

bool cxl_ide_test_config_check_common(void *test_context)
{
  // ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  // TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  // ide_common_test_group_context_t *group_context = config_context->group_context;
  // TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  // IDE_TEST_TOPOLOGY* top = group_context->top;
  // TEEIO_ASSERT(top->connection == IDE_TEST_TOPOLOGY_TYPE_SEL_IDE);


  return true;
}

bool cxl_ide_test_config_support_common(void *test_context)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  ide_common_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  CXL_PRIV_DATA* rp_cxl_data = &group_context->upper_port.priv_data.cxl;
  CXL_PRIV_DATA* ep_cxl_data = &group_context->lower_port.priv_data.cxl;

  // TODO
  bool supported = rp_cxl_data->memcache.ide_cap.cxl_ide_capable == 1 && ep_cxl_data->memcache.ide_cap.cxl_ide_capable == 1
                  && ep_cxl_data->ecap.cap.mem_capable == 1;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "rootport cxl_ide_capable=%d\n", rp_cxl_data->memcache.ide_cap.cxl_ide_capable));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "endpoint cxl_ide_capable=%d mem_capable=%d\n", rp_cxl_data->memcache.ide_cap.cxl_ide_capable, ep_cxl_data->ecap.cap.mem_capable));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_ide_test_config_support_common supported=%d\n", supported));
  return supported;
}

