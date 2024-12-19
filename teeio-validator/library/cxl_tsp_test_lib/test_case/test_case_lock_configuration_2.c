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
#include "helperlib.h"
#include "cxl_tsp_internal.h"

static libcxltsp_device_capabilities_t m_device_capabilities = {0};

static void test_cxl_tsp_lock_configuration_already_locked (
  const void *pci_doe_context,
  void *spdm_context, const uint32_t *session_id,
  int case_class, int case_id)
{ 
    libspdm_return_t status;
    cxl_tsp_lock_target_configuration_req_t request;
    size_t request_size;
    uint8_t res_buf[LIBCXLTSP_ERROR_MESSAGE_MAX_SIZE];
    cxl_tsp_error_rsp_t *response;
    size_t response_size;
    bool res;
    teeio_test_result_t assertion_result = TEEIO_TEST_RESULT_NOT_TESTED;

    libspdm_zero_mem (&request, sizeof(request));
    request.header.tsp_version = CXL_TSP_MESSAGE_VERSION_10;
    request.header.op_code = CXL_TSP_OPCODE_LOCK_TARGET_CONFIGURATION;

    request_size = sizeof(request);
    response = (void *)res_buf;
    response_size = sizeof(res_buf);
    status = cxl_tsp_send_receive_data(spdm_context, session_id,
                                       &request, request_size,
                                       response, &response_size);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_tsp_send_receive_data failed with status 0x%x\n", status));
        teeio_record_assertion_result(case_class, case_id, 0,
                                      IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED,
                                      "cxl_tsp_send_receive_data failed with 0x%x", status);
        return;
    }

    // Assertion.1
    res = response_size == sizeof(cxl_tsp_error_rsp_t);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 1,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                  "sizeof(TspMessage) = 0x%lx", response_size);
    if(!res) {
        return;
    }

    // Assertion.2
    res = response->header.op_code == CXL_TSP_OPCODE_ERROR_RSP;
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 2,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                  "TspMessage.Opcode = 0x%x", response->header.op_code);

    // Assertion.3
    res = response->header.tsp_version == request.header.tsp_version;
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 3,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                  "TspMessage.TSPVersion = 0x%x", response->header.tsp_version);

    // Assertion.4
    res = response->error_code == CXL_TSP_ERROR_CODE_ALREADY_LOCKED;
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 4,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                  "TspMessage.ErrorCode = 0x%x", response->error_code);

    // Assertion.5
    res = response->error_data == 0;
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 5,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                  "TspMessage.ErrorData = 0x%x", response->error_data);

}

bool cxl_tsp_test_lock_configuration_2_setup(void *test_context)
{
  libspdm_return_t status;

  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  cxl_tsp_test_group_context_t *group_context = (cxl_tsp_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  teeio_common_test_group_context_t *common = &group_context->common;
  TEEIO_ASSERT(common->signature == GROUP_CONTEXT_SIGNATURE);
  spdm_doe_context_t *spdm_doe = &group_context->spdm_doe;
  TEEIO_ASSERT(spdm_doe);

  status = cxl_tsp_get_version(spdm_doe->doe_context, spdm_doe->spdm_context, &spdm_doe->session_id);
  if(LIBSPDM_STATUS_IS_ERROR(status)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s: cxl_tsp_get_version failed with status=0x%x\n", __func__, status));
    return false;
  }

  libspdm_zero_mem(&m_device_capabilities, sizeof(m_device_capabilities));
  status = cxl_tsp_get_capabilities(spdm_doe->doe_context, spdm_doe->spdm_context, &spdm_doe->session_id, &m_device_capabilities);
  if(LIBSPDM_STATUS_IS_ERROR(status)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s: cxl_tsp_get_capabilities failed with status=0x%x\n", __func__, status));
    return false;
  }

  status = cxl_tsp_lock_configuration(spdm_doe->doe_context, spdm_doe->spdm_context, &spdm_doe->session_id);
  if(LIBSPDM_STATUS_IS_ERROR(status)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s: cxl_tsp_lock_configuration failed with status=0x%x\n", __func__, status));
    return false;
  }

  return true;
}

bool cxl_tsp_test_lock_configuration_2_run(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_run_test_case_t* test_case = case_context->test_case;
  TEEIO_ASSERT(test_case);
  int case_class = test_case->class_id;
  int case_id = test_case->case_id;

  cxl_tsp_test_group_context_t *group_context = (cxl_tsp_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  teeio_common_test_group_context_t *common = &group_context->common;
  TEEIO_ASSERT(common->signature == GROUP_CONTEXT_SIGNATURE);
  spdm_doe_context_t *spdm_doe = &group_context->spdm_doe;
  TEEIO_ASSERT(spdm_doe);

  test_cxl_tsp_lock_configuration_already_locked (
    spdm_doe->doe_context,
    spdm_doe->spdm_context,
    &spdm_doe->session_id,
    case_class, case_id);

  return true;
}

bool cxl_tsp_test_lock_configuration_2_teardown(void *test_context)
{
  TEEIO_PRINT(("CXL TSP test LOCK_CONFIGURATION Case 6.2 is done.\n"));
  TEEIO_PRINT(("If there are some other CXL TSP test cases, the device shall be first reset.\n"));
  TEEIO_PRINT(("Press any key to continue.\n"));
  getchar();
  
  return true;
}
