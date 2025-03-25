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

// test config of pcrc disable

// Set "PCRC Disable" bit in "CXL IDE Control register"
static bool set_cxl_ide_pcrc_disable(ide_common_test_port_context_t* port, bool set)
{
  CXL_PRIV_DATA_MEMCACHE_REG_DATA* memcache = &port->cxl_data.memcache;

  // CXL IDE Control is CXL IDE Capability Structure (CXL 3.1 8.2.4.22)
  // So first walk thru to find CXL IDE Capability
  int i = 0;
  for(; i < memcache->cap_headers_cnt; i++) {
    if(memcache->cap_headers[i].cap_id == CXL_CAPABILITY_ID_IDE_CAP) {
      break;
    }
  }

  if(i == memcache->cap_headers_cnt) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Cannot find CXL IDE Capability!\n"));
    return false;
  }

  uint8_t* ptr = memcache->mapped_memcache_reg_block + memcache->cap_headers[i].pointer + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, control);
  CXL_IDE_CONTROL ide_control = {.raw = mmio_read_reg32(ptr)};
  bool write_back = false;

  if (set) {
    // set "pcrc disable" bit if it is 0
    if(ide_control.pcrc_disable == 0) {
      ide_control.pcrc_disable = 1;
      write_back = true;
    }
  } else {
    // unset "pcrc disable" if it is 1
    if(ide_control.pcrc_disable == 1) {
      ide_control.pcrc_disable = 0;
      write_back = true;
    }
  }

  if(write_back) {
    mmio_write_reg32(ptr, ide_control.raw);
  }

  return true;
}

bool cxl_ide_test_config_set_pcrc_disable(void *test_context)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  cxl_ide_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  if(!set_cxl_ide_pcrc_disable(&group_context->common.upper_port, true)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Set PCRC Disable in root port failed\n"));
    teeio_record_config_item_result(
      CXL_IDE_CONFIGURATION_TYPE_PCRC_DISABLE,
      TEEIO_TEST_CONFIG_FUNC_ENABLE,
      TEEIO_TEST_RESULT_FAILED);
    return false;
  } else {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Set PCRC Disable in root port successfully\n"));
  }

  if(!set_cxl_ide_pcrc_disable(&group_context->common.lower_port, true)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Set PCRC Disable in device failed\n"));
    teeio_record_config_item_result(
      CXL_IDE_CONFIGURATION_TYPE_PCRC_DISABLE,
      TEEIO_TEST_CONFIG_FUNC_ENABLE,
      TEEIO_TEST_RESULT_FAILED);
    return false;
  } else {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Set PCRC Disable in device successfully\n"));
  }

  teeio_record_config_item_result(
    CXL_IDE_CONFIGURATION_TYPE_PCRC_DISABLE,
    TEEIO_TEST_CONFIG_FUNC_ENABLE,
    TEEIO_TEST_RESULT_PASS);

  return true;
}

bool cxl_ide_test_config_unset_pcrc_disable(void *test_context)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  cxl_ide_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  if(!set_cxl_ide_pcrc_disable(&group_context->common.upper_port, false)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Unset PCRC Disable in root port failed\n"));
    teeio_record_config_item_result(
      CXL_IDE_CONFIGURATION_TYPE_PCRC_DISABLE,
      TEEIO_TEST_CONFIG_FUNC_DISABLE,
      TEEIO_TEST_RESULT_FAILED);
    return false;
  } else {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Unset PCRC Disable in root port successfully\n"));
  }

  if(!set_cxl_ide_pcrc_disable(&group_context->common.lower_port, false)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Unset PCRC Disable in device failed\n"));
    teeio_record_config_item_result(
      CXL_IDE_CONFIGURATION_TYPE_PCRC_DISABLE,
      TEEIO_TEST_CONFIG_FUNC_DISABLE,
      TEEIO_TEST_RESULT_FAILED);
    return false;
  } else {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Unset PCRC Disable in device successfully\n"));
  }

  teeio_record_config_item_result(
    CXL_IDE_CONFIGURATION_TYPE_PCRC_DISABLE,
    TEEIO_TEST_CONFIG_FUNC_DISABLE,
    TEEIO_TEST_RESULT_PASS);

  return true;
}

bool cxl_ide_test_config_pcrc_disable_support(void *test_context)
{
  return true;
}

bool cxl_ide_test_config_pcrc_disable_check(void *test_context)
{
  return true;
}
