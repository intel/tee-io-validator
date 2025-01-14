/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "ide_test.h"
#include "teeio_debug.h"

#include "library/spdm_requester_lib.h"
#include "hal/library/memlib.h"
#include "helperlib.h"
#include "cxl_ide_lib.h"
#include "cxl_ide_test_common.h"
#include "cxl_ide_test_internal.h"

bool cxl_ide_test_key_prog_4_setup(void *test_context)
{
  // Cxl.Query has been called in test_group.setup()
  return true;
}

bool cxl_ide_test_key_prog_4_run(void *test_context)
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

  IDE_TEST_CONFIGURATION *configuration = get_configuration_by_id(group_context->common.suite_context->test_config, group_context->common.config_id);
  TEEIO_ASSERT(configuration);

  ide_common_test_port_context_t *lower_port = &group_context->common.lower_port;
  char case_info[MAX_LINE_LENGTH] = {0};
  int max_port_index = lower_port->cxl_data.query_resp.max_port_index;
  for(int port_index = 0; port_index <= max_port_index; port_index++) {
    for(int stream_id = 1; stream_id <= 0xff; stream_id++) {
      sprintf(case_info, "  port_index = 0x%02x stream_id=0x%02x", port_index, stream_id);
      test_cxl_ide_key_prog(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context,
                            &group_context->spdm_doe.session_id, stream_id, CXL_IDE_KM_KEY_SUB_STREAM_CXL,
                            port_index, false, case_info,
                            case_class, case_id, test_cxl_ide_key_prog_invalid_params);
      }
  }

  return true;
}

bool cxl_ide_test_key_prog_4_teardown(void *test_context)
{
  return true;
}
