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

void test_cxl_ide_query(const void *pci_doe_context,
                        void *spdm_context, const uint32_t *session_id,
                        uint8_t port_index, uint8_t dev_func_num, uint8_t bus_num, uint8_t segment,
                        int case_class, int case_id)
{
    libspdm_return_t status;
    cxl_ide_km_query_t request;
    size_t request_size;
    cxl_ide_km_query_resp_teeio_t response;
    size_t response_size;

    bool res = false;
    teeio_test_result_t assertion_result = TEEIO_TEST_RESULT_NOT_TESTED;

    libspdm_zero_mem (&request, sizeof(request));
    request.header.object_id = CXL_IDE_KM_OBJECT_ID_QUERY;
    request.port_index = port_index;

    request_size = sizeof(request);
    response_size = sizeof(response);
    status = cxl_ide_km_send_receive_data(spdm_context, session_id,
                                          &request, request_size,
                                          &response, &response_size);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_send_receive_data send receive_data failed with status 0x%x\n", status));
        teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED, "cxl_ide_km_send_receive_data failed with 0x%x", status);
        return;
    }

    // Assertion.1
    res = response_size == sizeof(cxl_ide_km_query_resp_t) ? false : true;
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 1, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "sizeof(CxlIdeKmMessage) = 0x%lx", response_size);
    if(!res) {
        return;
    }

    // Assertion.2
    res = (response.header.object_id == CXL_IDE_KM_OBJECT_ID_QUERY_RESP);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 2, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "CxlIdeKmMessage.ObjectID = 0x%x", response.header.object_id);

    // Assertion.3
    res = (response.port_index == request.port_index);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 3, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "CxlIdeKmMessage.PortIndex = 0x%x", response.port_index);

    // Assertion.4
    res = (response.max_port_index >= request.port_index);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 4, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "CxlIdeKmMessage.MaxPortIndex = 0x%x", response.max_port_index);

  // assertion.5
  res = dev_func_num == response.dev_func_num && bus_num == response.bus_num && segment == response.segment;
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 5, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                "CxlIdeKmMessage.DevFunc = 0x%x && CxlIdeKmMessage.Bus = 0x%x && CxlIdeKmMessage.Segment = 0x%x",
                                response.dev_func_num,
                                response.bus_num,
                                response.segment);

  // assertion.6
  CXL_QUERY_RESP_CAPS caps = {.raw = response.caps};
  res = caps.cxl_ide_cap_version == 1;
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 6, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                "CxlIdeKmMessage.CXL_IDE_Capability_Version = 0x%x",
                                caps.cxl_ide_cap_version);
}

bool cxl_ide_test_query_1_setup(void *test_context)
{
  return true;
}

void cxl_ide_test_query_1_run(void *test_context)
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

  test_cxl_ide_query(group_context->spdm_doe.doe_context,
                      group_context->spdm_doe.spdm_context,
                      &group_context->spdm_doe.session_id,
                      0, dev_func_num, bus_num, segment,
                      case_class, case_id);
}

void cxl_ide_test_query_1_teardown(void *test_context)
{
}
