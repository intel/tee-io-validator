/**
 *  Copyright Notice:
 *  Copyright 2026 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>

#include "hal/base.h"
#include "hal/library/debuglib.h"

#include "ide_test.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "multi_ide_test_internal.h"

extern const char* m_cxl_ide_mode_names[];

bool multi_ide_test_config_default_enable(void *test_context)
{
  return true;
}

bool multi_ide_test_config_default_disable(void *test_context)
{
  return true;
}

// check that both ports support the type of every stream in the streams list
bool multi_ide_test_config_default_support(void *test_context)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  multi_ide_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  PCIE_IDE_CAP *host_cap = &group_context->common.upper_port.ide_cap;
  PCIE_IDE_CAP *dev_cap = &group_context->common.lower_port.ide_cap;
  CXL_IDE_CAPABILITY rp_cxl_ide_cap = {.raw = group_context->common.upper_port.cxl_data.memcache.ide_cap.raw};
  CXL_IDE_CAPABILITY ep_cxl_ide_cap = {.raw = group_context->common.lower_port.cxl_data.memcache.ide_cap.raw};
  CXL_IDE_MODE ide_mode = multi_ide_get_cxl_ide_mode(group_context);
  uint32_t ide_mode_mask = ide_mode == CXL_IDE_MODE_SKID ? CXL_IDE_MODE_SKID_MASK : CXL_IDE_MODE_CONTAINMENT_MASK;
  bool supported = true;
  bool stream_supported = false;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "host lnk_ide_supported=%d, sel_ide_supported=%d\n",
               host_cap->lnk_ide_supported, host_cap->sel_ide_supported));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "dev lnk_ide_supported=%d, sel_ide_supported=%d, ide_km_protocol_supported=%d\n",
               dev_cap->lnk_ide_supported, dev_cap->sel_ide_supported, dev_cap->ide_km_protocol_supported));

  for(int i = 0; i < group_context->streams_cnt; i++) {
    multi_ide_stream_t *stream = &group_context->streams[i];

    if(stream->type == IDE_TEST_STREAM_TYPE_LINK_IDE) {
      stream_supported = host_cap->lnk_ide_supported == 1 && dev_cap->lnk_ide_supported == 1 &&
                         dev_cap->ide_km_protocol_supported == 1;
    } else if(stream->type == IDE_TEST_STREAM_TYPE_SEL_IDE) {
      stream_supported = host_cap->sel_ide_supported == 1 && dev_cap->sel_ide_supported == 1 &&
                         dev_cap->ide_km_protocol_supported == 1;
    } else {
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "rootport cxl_ide_cap=0x%08x, endpoint cxl_ide_cap=0x%08x, ide_mode=%s\n",
                   rp_cxl_ide_cap.raw, ep_cxl_ide_cap.raw, m_cxl_ide_mode_names[ide_mode]));
      stream_supported = rp_cxl_ide_cap.cxl_ide_capable == 1 && ep_cxl_ide_cap.cxl_ide_capable == 1 &&
                         (rp_cxl_ide_cap.raw & ide_mode_mask) != 0 && (ep_cxl_ide_cap.raw & ide_mode_mask) != 0;
    }

    if(!stream_supported) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s:%d stream is not supported.\n", IDE_TEST_STREAM_TYPE_NAMES[stream->type], stream->stream_id));
      supported = false;
    }
  }

  teeio_record_config_item_result(
    MULTI_IDE_CONFIGURATION_TYPE_DEFAULT,
    TEEIO_TEST_CONFIG_FUNC_SUPPORT,
    supported ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED);

  return supported;
}

bool multi_ide_test_config_default_check(void *test_context)
{
  return true;
}
