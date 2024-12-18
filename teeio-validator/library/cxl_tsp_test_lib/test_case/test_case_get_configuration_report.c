/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ide_test.h"
#include "teeio_debug.h"
#include "helperlib.h"
#include "cxl_tsp_internal.h"

static libcxltsp_device_capabilities_t m_device_capabilities = {0};

static void test_cxl_tsp_get_configuration_report(
  const void *pci_doe_context,
  void *spdm_context, const uint32_t *session_id,
  uint8_t* configuration_report, uint32_t* configuration_report_size,
  int case_class, int case_id)
{
    libspdm_return_t status;
    cxl_tsp_get_target_configuration_report_req_t request;
    size_t request_size;
    teeio_cxl_tsp_get_target_configuration_report_rsp_mine_t response;
    size_t response_size;
    uint16_t offset;
    uint16_t remainder_length;
    uint32_t total_report_length;
    bool res;
    teeio_test_result_t assertion_result = TEEIO_TEST_RESULT_NOT_TESTED;
    int round = 0;

    offset = 0;
    remainder_length = 0;
    total_report_length = 0;
    do {
        libspdm_zero_mem (&request, sizeof(request));
        request.header.tsp_version = CXL_TSP_MESSAGE_VERSION_10;
        request.header.op_code = CXL_TSP_OPCODE_GET_TARGET_CONFIGURATION_REPORT;
        request.offset = offset;
        request.length = LIBCXLTSP_CONFIGURATION_REPORT_PORTION_LEN;
        if (request.offset != 0) {
            request.length = LIBSPDM_MIN (remainder_length, LIBCXLTSP_CONFIGURATION_REPORT_PORTION_LEN);
        }

        teeio_record_assertion_result(case_class, case_id, 0,
                                      IDE_COMMON_TEST_CASE_ASSERTION_TYPE_SEPARATOR, TEEIO_TEST_RESULT_NOT_TESTED,
                                      "TSP_GET_CONFIGURATION_REPORT round-%d\n", round);

        request_size = sizeof(request);
        response_size = sizeof(response);
        status = cxl_tsp_send_receive_data(spdm_context, session_id,
                                        &request, request_size,
                                        &response, &response_size);
        if (LIBSPDM_STATUS_IS_ERROR(status)) {
            TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_tsp_send_receive_data failed with status 0x%x\n", status));
            teeio_record_assertion_result(case_class, case_id, 0,
                                          IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED,
                                          "cxl_tsp_send_receive_data failed with 0x%x", status);
            return;
        }

        // Assertion.1
        res = response_size == sizeof(cxl_tsp_get_target_configuration_report_rsp_t) + response.portion_length;
        assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
        teeio_record_assertion_result(case_class, case_id, 1,
                                      IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                      "sizeof(TspMessage) = 0x%lx", response_size);
        if(!res) {
            return;
        }

        // Assertion.2
        res = response.header.op_code == CXL_TSP_OPCODE_GET_TARGET_CONFIGURATION_REPORT;
        assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
        teeio_record_assertion_result(case_class, case_id, 2,
                                      IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                      "TspMessage.Opcode = 0x%x", response.header.op_code);


        // Assertion.3
        res = response.header.tsp_version == request.header.tsp_version;
        assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
        teeio_record_assertion_result(case_class, case_id, 3,
                                      IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                      "TspMessage.TSPVersion = 0x%x", response.header.tsp_version);


        // Assertion.4
        res = response.portion_length > 0 && response.portion_length <= request.length;
        assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
        teeio_record_assertion_result(case_class, case_id, 4,
                                      IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                      "TspMessage.PortionLength = 0x%x", response.portion_length);

        if (offset == 0) {
            total_report_length = response.portion_length + response.remainder_length;
            if (total_report_length > *configuration_report_size) {
                teeio_record_assertion_result(case_class, case_id, 0,
                                              IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED,
                                              "Configuration report size too small - 0x%x", *configuration_report_size);
                *configuration_report_size = total_report_length;
                return;
            }
        } else {
            if (total_report_length !=
                (uint32_t)(offset + response.portion_length + response.remainder_length)) {
                teeio_record_assertion_result(case_class, case_id, 0,
                                              IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED,
                                              "Total report size mismatch - 0x%x", total_report_length);
                return;
            }
        }
        libspdm_copy_mem (configuration_report + offset,
                          *configuration_report_size - offset,
                          response.report,
                          response.portion_length);
        offset = offset + response.portion_length;
        remainder_length = response.remainder_length;
        round++;
    } while (remainder_length != 0);

    *configuration_report_size = total_report_length;
}

bool cxl_tsp_test_get_configuration_report_setup(void *test_context)
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
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s: cxl_tsp_get_version failed with  status=0x%x\n", __func__, status));
    return false;
  }

  libspdm_zero_mem(&m_device_capabilities, sizeof(m_device_capabilities));
  status = cxl_tsp_get_capabilities(spdm_doe->doe_context, spdm_doe->spdm_context, &spdm_doe->session_id, &m_device_capabilities);
  if(LIBSPDM_STATUS_IS_ERROR(status)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s: cxl_tsp_get_capabilities failed with  status=0x%x\n", __func__, status));
    return false;
  }

  return true;
}

bool cxl_tsp_test_get_configuration_report_run(void *test_context)
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

  uint8_t configuration_report_buffer[LIBCXLTSP_CONFIGURATION_REPORT_MAX_SIZE] = {0};
  uint32_t configuration_report_size;

  test_cxl_tsp_get_configuration_report(spdm_doe->doe_context,
                                        spdm_doe->spdm_context,
                                        &spdm_doe->session_id,
                                        configuration_report_buffer,
                                        &configuration_report_size,
                                        case_class, case_id);

  return true;
}

bool cxl_tsp_test_get_configuration_report_teardown(void *test_context)
{
  return true;
}
