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

  // Query the port index
  if (!cxl_ide_query_port_index(group_context)) {
    return false;
  }

  return cxl_setup_ide_stream(spdm_doe->doe_context, spdm_doe->spdm_context,
                              &spdm_doe->session_id, upper_port->mapped_kcbar_addr,
                              group_context->stream_id, group_context->common.lower_port.port->port_index,
                              upper_port, lower_port, false,
                              configuration->bit_map, configuration->priv_data.cxl_ide.ide_mode,
                              false);
}

void cxl_ide_test_keyrefresh_run(void *test_context)
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
      // Query the port index
      if (!cxl_ide_query_port_index(group_context)) {
        break;
      }

      res = cxl_setup_ide_stream(spdm_doe->doe_context, spdm_doe->spdm_context,
                              &spdm_doe->session_id, upper_port->mapped_kcbar_addr,
                              group_context->stream_id, group_context->common.lower_port.port->port_index,
                              upper_port, lower_port, false,
                              configuration->bit_map, configuration->priv_data.cxl_ide.ide_mode,
                              true);

      if(!res) {
        break;
      }
    }
  }

  teeio_record_assertion_result(case_class, case_id, 1, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                res ? "CXL-IDE KeyRefresh succeeded." : "CXL-IDE KeyRefresh failed.");
}

void cxl_ide_test_keyrefresh_teardown(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  cxl_ide_test_group_context_t *group_context = (cxl_ide_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  teeio_common_test_group_context_t *common = &group_context->common;
  TEEIO_ASSERT(common->signature == GROUP_CONTEXT_SIGNATURE);

  ide_common_test_port_context_t* upper_port = &common->upper_port;

  // clear LinkEncEnable on the RootPort side
  INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr = (INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)upper_port->mapped_kcbar_addr;
  cxl_cfg_rp_linkenc_enable(kcbar_ptr, false);
}
