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
#include "cxl_tsp_internal.h"

// doc/tsp_test/TspTestCase/1.GetTargetTspVersionResponse.md
static const char* mCxlTspGetVersionAssersion[] = {
    "cxl_tsp_get_version send_receive data",                      // .0
    "sizeof(TspMessage) == sizeof(GET_TARGET_TSP_VERSION_RSP)",   // .1
    "TspMessage.Opcode == GET_TARGET_TSP_VERSION_RSP",            // .2
    "TspMessage.TSPVersion == 0x10",                              // .3
    "TspMessage.VersionNumberEntryCount == 1",                    // .4
    "TspMessage.VersionNumberEntry[0] == 0x10"                    // .5
};

/**
 * Send and receive an TSP message
 *
 * @param  spdm_context                 A pointer to the SPDM context.
 * @param  session_id                   Indicates if it is a secured message protected via SPDM session.
 *                                     If session_id is NULL, it is a normal message.
 *                                     If session_id is NOT NULL, it is a secured message.
 *
 * @retval LIBSPDM_STATUS_SUCCESS               The TSP request is sent and response is received.
 * @return ERROR                        The TSP response is not received correctly.
 **/
libspdm_return_t test_cxl_tsp_get_version(
    const void *pci_doe_context,
    void *spdm_context,
    const uint32_t *session_id)
{
    libspdm_return_t status;
    cxl_tsp_get_target_tsp_version_req_t request;
    size_t request_size;
    uint8_t res_buf[LIBCXLTSP_ERROR_MESSAGE_MAX_SIZE];
    teeio_cxl_tsp_get_target_tsp_version_rsp_mine_t *response;
    size_t response_size;
    bool res;

    libspdm_zero_mem (&request, sizeof(request));
    request.header.tsp_version = CXL_TSP_MESSAGE_VERSION_10;
    request.header.op_code = CXL_TSP_OPCODE_GET_TARGET_TSP_VERSION;

    const char* case_msg = "  Assertion 1.1";

    request_size = sizeof(request);
    response = (void *)res_buf;
    response_size = sizeof(res_buf);
    status = cxl_tsp_send_receive_data(spdm_context, session_id,
                                       &request, request_size,
                                       response, &response_size);

    // Assersion.0
    TEEIO_PRINT(("         %s.0: %s(0x%x) %s.\n", case_msg, mCxlTspGetVersionAssersion[0], status, !LIBSPDM_STATUS_IS_ERROR(status) ? "Pass" : "failed"));
    if (LIBSPDM_STATUS_IS_ERROR(status))
    {
        return status;
    }

    // Assertion.1
    res = response_size == sizeof(teeio_cxl_tsp_get_target_tsp_version_rsp_mine_t);
    TEEIO_PRINT(("         %s.1: %s %s\n", case_msg, mCxlTspGetVersionAssersion[1], res ? "Pass" : "Fail"));
    if (!res) {
        return LIBSPDM_STATUS_INVALID_MSG_SIZE;
    }

    // Assertion.2
    res = response->header.op_code == CXL_TSP_OPCODE_GET_TARGET_TSP_VERSION_RSP;
    TEEIO_PRINT(("         %s.2: %s %s\n", case_msg, mCxlTspGetVersionAssersion[2], res ? "Pass" : "Fail"));
    if (!res) {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    // Assertion.3
    res = response->header.tsp_version == request.header.tsp_version;
    TEEIO_PRINT(("         %s.3: %s %s\n", case_msg, mCxlTspGetVersionAssersion[3], res ? "Pass" : "Fail"));
    if (!res) {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    // Assertion.4
    res = response->version_number_entry_count == 1;
    TEEIO_PRINT(("         %s.4: %s %s\n", case_msg, mCxlTspGetVersionAssersion[4], res ? "Pass" : "Fail"));
    if (!res) {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    // Assertion.5
    res = response->version_number_entry[0] == CXL_TSP_MESSAGE_VERSION_10;
    TEEIO_PRINT(("         %s.5: %s %s\n", case_msg, mCxlTspGetVersionAssersion[5], res ? "Pass" : "Fail"));
    if (!res) {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    return LIBSPDM_STATUS_SUCCESS;
}

bool cxl_tsp_test_get_version_setup(void *test_context)
{
  return true;
}

bool cxl_tsp_test_get_version_run(void *test_context)
{
  libspdm_return_t status;
  bool pass = true;

  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  cxl_tsp_test_group_context_t *group_context = (cxl_tsp_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  teeio_common_test_group_context_t *common = &group_context->common;
  TEEIO_ASSERT(common->signature == GROUP_CONTEXT_SIGNATURE);
  spdm_doe_context_t *spdm_doe = &group_context->spdm_doe;
  TEEIO_ASSERT(spdm_doe);

  status = test_cxl_tsp_get_version(spdm_doe->doe_context, spdm_doe->spdm_context, &spdm_doe->session_id);

  pass = !LIBSPDM_STATUS_IS_ERROR(status);
  case_context->result = pass ? IDE_COMMON_TEST_CASE_RESULT_SUCCESS : IDE_COMMON_TEST_CASE_RESULT_FAILED;

  return true;
}

bool cxl_tsp_test_get_version_teardown(void *test_context)
{
  return true;
}
