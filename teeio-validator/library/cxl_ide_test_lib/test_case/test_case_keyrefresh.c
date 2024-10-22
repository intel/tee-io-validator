/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "assert.h"
#include "hal/base.h"
#include "hal/library/debuglib.h"

#include "hal/library/memlib.h"
#include "library/spdm_requester_lib.h"
#include "library/cxl_ide_km_requester_lib.h"
#include "ide_test.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "cxl_ide_lib.h"
#include "cxl_ide_test_common.h"

// setup cxl ide stream
bool cxl_setup_ide_stream(void *doe_context, void *spdm_context,
                          uint32_t *session_id, uint8_t *kcbar_addr,
                          uint8_t stream_id, uint8_t port_index,
                          ide_common_test_port_context_t *upper_port,
                          ide_common_test_port_context_t *lower_port,
                          bool skip_ksetgo, uint32_t config_bitmap,
                          bool set_link_enc_enable, bool program_iv);
// stop cxl ide stream
bool cxl_stop_ide_stream(void *doe_context, void *spdm_context,
                         uint32_t *session_id, uint8_t *kcbar_addr,
                         uint8_t stream_id,
                         uint8_t port_index,
                         ide_common_test_port_context_t *upper_port,
                         ide_common_test_port_context_t *lower_port);

bool cxl_ide_test_keyrefresh_setup(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  cxl_ide_test_group_context_t *group_context = (cxl_ide_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  teeio_common_test_group_context_t *common = &group_context->common;
  TEEIO_ASSERT(common->signature == GROUP_CONTEXT_SIGNATURE);
  spdm_doe_context_t* spdm_doe = &group_context->spdm_doe;
  TEEIO_ASSERT(spdm_doe);

  ide_common_test_port_context_t* upper_port = &common->upper_port;
  ide_common_test_port_context_t* lower_port = &common->lower_port;

  // An ide_stream is first setup so that key_refresh can be tested in run.
  IDE_TEST_CONFIGURATION *configuration = get_configuration_by_id(common->suite_context->test_config, common->config_id);
  TEEIO_ASSERT(configuration);

  return cxl_setup_ide_stream(spdm_doe->doe_context, spdm_doe->spdm_context,
                              &spdm_doe->session_id, upper_port->mapped_kcbar_addr,
                              group_context->stream_id, 0,
                              upper_port, lower_port, false, configuration->bit_map, true, true);
}

bool cxl_ide_test_keyrefresh_run(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  cxl_ide_test_group_context_t *group_context = (cxl_ide_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  teeio_common_test_group_context_t *common = &group_context->common;
  TEEIO_ASSERT(common->signature == GROUP_CONTEXT_SIGNATURE);
  spdm_doe_context_t* spdm_doe = &group_context->spdm_doe;
  TEEIO_ASSERT(spdm_doe);

  ide_common_test_port_context_t* upper_port = &common->upper_port;
  ide_common_test_port_context_t* lower_port = &common->lower_port;
  INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr = (INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)upper_port->mapped_kcbar_addr;

  IDE_TEST_CONFIGURATION *configuration = get_configuration_by_id(common->suite_context->test_config, common->config_id);
  TEEIO_ASSERT(configuration);

  int cmd = 0;
  bool res = true;

  while(true){
    TEEIO_PRINT(("\n"));
    TEEIO_PRINT(("Print host registers.\n"));
    cxl_dump_kcbar(kcbar_ptr);
    // dump CXL IDE Capability in memcache reg block
    cxl_dump_ide_status(upper_port->cxl_data.memcache.cap_headers, upper_port->cxl_data.memcache.cap_headers_cnt, upper_port->cxl_data.memcache.mapped_memcache_reg_block);

    TEEIO_PRINT(("\n"));
    TEEIO_PRINT(("Print device registers.\n"));
    // dump CXL IDE Capability in memcache reg block
    cxl_dump_ide_status(lower_port->cxl_data.memcache.cap_headers, lower_port->cxl_data.memcache.cap_headers_cnt, lower_port->cxl_data.memcache.mapped_memcache_reg_block);

    TEEIO_PRINT(("Press 'q' to quit test or any other keys to key_refresh.\n"));
    cmd = getchar();
    if(cmd == 'q' || cmd == 'Q') {
      break;
    } else {
      res = cxl_setup_ide_stream(spdm_doe->doe_context, spdm_doe->spdm_context,
                              &spdm_doe->session_id, upper_port->mapped_kcbar_addr,
                              group_context->stream_id, 0,
                              upper_port, lower_port, false, configuration->bit_map, false, false);

      if(!res) {
        break;
      }
    }
  }

  case_context->result = res ? IDE_COMMON_TEST_CASE_RESULT_SUCCESS : IDE_COMMON_TEST_CASE_RESULT_FAILED;

  return res;
}

bool cxl_ide_test_keyrefresh_teardown(void *test_context)
{
  bool ret = false;

  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  cxl_ide_test_group_context_t *group_context = (cxl_ide_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  teeio_common_test_group_context_t *common = &group_context->common;
  TEEIO_ASSERT(common->signature == GROUP_CONTEXT_SIGNATURE);
  spdm_doe_context_t* spdm_doe = &group_context->spdm_doe;
  TEEIO_ASSERT(spdm_doe);

  ide_common_test_port_context_t* upper_port = &common->upper_port;
  ide_common_test_port_context_t* lower_port = &common->lower_port;

  CXL_QUERY_RESP_CAPS dev_caps = {.raw = lower_port->cxl_data.query_resp.caps};
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "dev_caps.k_set_stop_capable = %d\n", dev_caps.k_set_stop_capable));

  // send KSetStop if supported.
  if(dev_caps.k_set_stop_capable == 1) {
    ret = cxl_stop_ide_stream(spdm_doe->doe_context, spdm_doe->spdm_context,
                              &spdm_doe->session_id, upper_port->mapped_kcbar_addr,
                              group_context->stream_id, 0,
                              upper_port, lower_port);
    if(!ret) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_stop_ide_stream failed.\n"));
      return false;
    }
  } else {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "KSetStop is not supported.\n"));
  }

  // clear LinkEncEnable on the RootPort side
  INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr = (INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)upper_port->mapped_kcbar_addr;
  cxl_cfg_rp_linkenc_enable(kcbar_ptr, false);

  return true;
}
