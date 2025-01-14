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

// test selective_ide and link_ide with skid mode
bool cxl_ide_test_config_skid_enable(void *test_context)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  cxl_ide_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  if(group_context->common.case_class == CXL_MEM_IDE_TEST_CASE_QUERY) {
    return true;
  }

  INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR* kcbar_ptr = (INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)group_context->common.upper_port.mapped_kcbar_addr;
  cxl_cfg_rp_mode(kcbar_ptr, INTEL_CXL_IDE_MODE_SKID);

  teeio_record_config_item_result(
    CXL_IDE_CONFIGURATION_TYPE_SKID_MODE,
    TEEIO_TEST_CONFIG_FUNC_ENABLE,
    TEEIO_TEST_RESULT_PASS);

  return true;
}

bool cxl_ide_test_config_skid_disable(void *test_context)
{
  return true;
}

bool cxl_ide_test_config_skid_support(void *test_context)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  cxl_ide_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  if(group_context->common.case_class == CXL_MEM_IDE_TEST_CASE_QUERY) {
    return true;
  }

  CXL_PRIV_DATA* rp_cxl_data = &group_context->common.upper_port.cxl_data;
  CXL_PRIV_DATA* ep_cxl_data = &group_context->common.lower_port.cxl_data;

  bool supported = rp_cxl_data->memcache.ide_cap.cxl_ide_capable == 1
                  && (rp_cxl_data->memcache.ide_cap.raw & (uint32_t)CXL_IDE_MODE_SKID_MASK) != 0
                  && ep_cxl_data->memcache.ide_cap.cxl_ide_capable == 1
                  && (ep_cxl_data->memcache.ide_cap.raw & (uint32_t)CXL_IDE_MODE_SKID_MASK) != 0;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "rootport cxl_ide_cap=0x%08x\n", rp_cxl_data->memcache.ide_cap.raw));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "endpoint cxl_ide_cap=0x%08x\n", ep_cxl_data->memcache.ide_cap.raw));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_ide_test_config_skid_support supported=%d\n", supported));
  return supported;
}

bool cxl_ide_test_config_skid_check(void *test_context)
{
  return true;
}
