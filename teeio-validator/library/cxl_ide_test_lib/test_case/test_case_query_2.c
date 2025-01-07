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

static uint8_t mMaxPortIndex = 0;

bool cxl_ide_test_query_2_setup(void *test_context)
{
  libspdm_return_t status;

  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  cxl_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  CXL_QUERY_RESP_CAPS caps;
  uint8_t bus_num;
  uint8_t segment;
  uint8_t dev_func_num;
  uint8_t max_port_index;
  uint32_t ide_reg_block[CXL_IDE_KM_IDE_CAP_REG_BLOCK_MAX_COUNT] = {0};
  uint32_t ide_reg_block_count;

  // query
  caps.raw = 0;
  ide_reg_block_count = CXL_IDE_KM_IDE_CAP_REG_BLOCK_MAX_COUNT;
  status = cxl_ide_km_query(group_context->spdm_doe.doe_context,
                            group_context->spdm_doe.spdm_context,
                            &group_context->spdm_doe.session_id,
                            0, // port_index
                            &dev_func_num,
                            &bus_num,
                            &segment,
                            &max_port_index,
                            &caps.raw,
                            ide_reg_block,
                            &ide_reg_block_count);
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_query failed with status=0x%x\n", status));
    return false;
  }

  if (max_port_index == 0)
  {
    // skip the case
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Skip Query.2 because max_port_index == 0.\n"));
    case_context->action = IDE_COMMON_TEST_ACTION_SKIP;
  }

  mMaxPortIndex = max_port_index;

  return true;
}

bool cxl_ide_test_query_2_run(void *test_context)
{
  uint8_t bus_num;
  uint8_t segment;
  uint8_t dev_func_num;

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

  dev_func_num = ((group_context->common.lower_port.port->device & 0x1f) << 3) | (group_context->common.lower_port.port->function & 0x7);
  bus_num = group_context->common.lower_port.port->bus;
  segment = 0;

  for(int i = 1; i <= mMaxPortIndex; i++) {
    teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_SEPARATOR, TEEIO_TEST_RESULT_NOT_TESTED, "port_index = %d", i);
    test_cxl_ide_query(group_context->spdm_doe.doe_context,
                        group_context->spdm_doe.spdm_context,
                        &group_context->spdm_doe.session_id,
                        i, dev_func_num, bus_num, segment,
                        case_class, case_id);
  }

  return true;
}

bool cxl_ide_test_query_2_teardown(void *test_context)
{
  return true;
}
