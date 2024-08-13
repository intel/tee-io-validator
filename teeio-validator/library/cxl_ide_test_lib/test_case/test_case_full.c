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
                          bool skip_ksetgo, uint32_t config_bitmap);
bool cxl_stop_ide_stream(void *doe_context, void *spdm_context,
                         uint32_t *session_id, uint8_t *kcbar_addr,
                         uint8_t stream_id,
                         uint8_t port_index,
                         ide_common_test_port_context_t *upper_port,
                         ide_common_test_port_context_t *lower_port);

bool cxl_reset_ecap_registers(ide_common_test_port_context_t *port_context);
bool cxl_reset_kcbar_registers(ide_common_test_port_context_t *port_context);

bool cxl_ide_test_full_ide_stream_setup(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  cxl_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  ide_common_test_port_context_t *upper_port = &group_context->upper_port;
  ide_common_test_port_context_t *lower_port = &group_context->lower_port;

  IDE_TEST_CONFIGURATION *configuration = get_configuration_by_id(group_context->suite_context->test_config, group_context->config_id);
  TEEIO_ASSERT(configuration);

  return cxl_setup_ide_stream(group_context->doe_context, group_context->spdm_context,
                              &group_context->session_id, upper_port->mapped_kcbar_addr,
                              group_context->stream_id, 0,
                              upper_port, lower_port, false, configuration->bit_map);
}

bool cxl_ide_test_full_ide_stream_run(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  cxl_ide_test_group_context_t *group_context = (cxl_ide_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  // int upper_port_cfg_space_fd = group_context->upper_port.cfg_space_fd;
  // uint32_t upper_port_ecap_offset = group_context->upper_port.ecap_offset;
  INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr = (INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)group_context->upper_port.mapped_kcbar_addr;
  ide_common_test_port_context_t* upper_port = &group_context->upper_port;
  ide_common_test_port_context_t* lower_port = &group_context->lower_port;

  // int lower_port_cfg_space_fd = group_context->lower_port.cfg_space_fd;
  // uint32_t lower_port_ecap_offset = group_context->lower_port.ecap_offset;

  // Now ide stream shall be in secure state
  // Dump registers to check
  TEEIO_PRINT(("\n"));
  TEEIO_PRINT(("Print host registers.\n"));

  // cxl_dump_ecap(upper_port_cfg_space_fd, upper_port_ecap_offset);
  cxl_dump_kcbar(kcbar_ptr);
  // dump CXL IDE Capability in memcache reg block
  cxl_dump_ide_status(upper_port->cxl_data.memcache.cap_headers, upper_port->cxl_data.memcache.cap_headers_cnt, upper_port->cxl_data.memcache.mapped_memcache_reg_block);

  TEEIO_PRINT(("\n"));
  TEEIO_PRINT(("Print device registers.\n"));
  // cxl_dump_ecap(lower_port_cfg_space_fd, lower_port_ecap_offset);
  // dump CXL IDE Capability in memcache reg block
  cxl_dump_ide_status(lower_port->cxl_data.memcache.cap_headers, lower_port->cxl_data.memcache.cap_headers_cnt, lower_port->cxl_data.memcache.mapped_memcache_reg_block);

  TEEIO_PRINT(("ide_stream is setup. Press any key to continue.\n"));
  getchar();

  case_context->result = IDE_COMMON_TEST_CASE_RESULT_SUCCESS;
  return true;
}

bool cxl_ide_test_full_ide_stream_teardown(void *test_context)
{
  bool ret = false;

  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  cxl_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  ide_common_test_port_context_t *upper_port = &group_context->upper_port;
  ide_common_test_port_context_t *lower_port = &group_context->lower_port;

  // send KSetStop if supported.
  if(lower_port->cxl_data.memcache.ide_cap.ide_stop_capable) {
    ret = cxl_stop_ide_stream(group_context->doe_context, group_context->spdm_context,
                              &group_context->session_id, upper_port->mapped_kcbar_addr,
                              group_context->stream_id, 0,
                              upper_port, lower_port);
    if(!ret) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_stop_ide_stream failed.\n"));
      return false;
    }
  }

  // clear LinkEncEnable on the RootPort side
  INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr = (INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)upper_port->mapped_kcbar_addr;
  cxl_cfg_rp_linkenc_enable(kcbar_ptr, false);

  return true;
}
