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
#include "cxl_ide_test_internal.h"

bool cxl_reset_ecap_registers(ide_common_test_port_context_t *port_context);
bool cxl_reset_kcbar_registers(ide_common_test_port_context_t *port_context);

bool cxl_ide_test_full_ide_stream_setup(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  cxl_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  ide_common_test_port_context_t *upper_port = &group_context->common.upper_port;
  ide_common_test_port_context_t *lower_port = &group_context->common.lower_port;

  IDE_TEST_CONFIGURATION *configuration = get_configuration_by_id(group_context->common.suite_context->test_config, group_context->common.config_id);
  TEEIO_ASSERT(configuration);

  return cxl_setup_ide_stream(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context,
                              &group_context->spdm_doe.session_id, upper_port->mapped_kcbar_addr,
                              group_context->stream_id, 0,
                              upper_port, lower_port, false, configuration->bit_map);
}

bool cxl_ide_test_full_ide_stream_run(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_run_test_case_t* test_case = case_context->test_case;
  TEEIO_ASSERT(test_case);
  int case_class = test_case->class_id;
  int case_id = test_case->case_id;

  cxl_ide_test_group_context_t *group_context = (cxl_ide_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr = (INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)group_context->common.upper_port.mapped_kcbar_addr;
  ide_common_test_port_context_t* upper_port = &group_context->common.upper_port;
  ide_common_test_port_context_t* lower_port = &group_context->common.lower_port;

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
  // dump CXL IDE Capability in memcache reg block
  cxl_dump_ide_status(lower_port->cxl_data.memcache.cap_headers, lower_port->cxl_data.memcache.cap_headers_cnt, lower_port->cxl_data.memcache.mapped_memcache_reg_block);

  TEEIO_PRINT(("ide_stream is setup. Press any key to continue.\n"));
  getchar();

  teeio_record_assertion_result(case_class, case_id, 1, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_PASS, "CXL-IDE Stream is setup.");
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
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  ide_common_test_port_context_t *upper_port = &group_context->common.upper_port;
  ide_common_test_port_context_t *lower_port = &group_context->common.lower_port;

  CXL_QUERY_RESP_CAPS dev_caps = {.raw = lower_port->cxl_data.query_resp.caps};
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "dev_caps.k_set_stop_capable = %d\n", dev_caps.k_set_stop_capable));

  // send KSetStop if supported.
  if(dev_caps.k_set_stop_capable == 1) {
    ret = cxl_stop_ide_stream(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context,
                              &group_context->spdm_doe.session_id, upper_port->mapped_kcbar_addr,
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
