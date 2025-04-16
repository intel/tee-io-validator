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

extern bool g_teeio_fixed_key;
static uint8_t mPortIndex[0x100] = {0};

static int construct_test_port_index(uint8_t max_port_index)
{
  int port_index_cnt = 0;

  if(max_port_index == 0xff) {
    return 0;
  }

  for(uint16_t port_index = max_port_index + 1; port_index <= 0xff; port_index++) {
    if(is_power_of_two(port_index) || port_index == max_port_index + 1 || port_index == 0xff) {
      mPortIndex[port_index_cnt++] = port_index;
    }
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "construct test port_index list. Total %d port_index.\n", port_index_cnt));
  for(int i = 0; i < port_index_cnt; i++) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  mPortIndex[%d] = 0x%02x\n", i, mPortIndex[i]));
  }

  return port_index_cnt;
}

// Test cxl.key_prog with invalid parameters
// Invalid params includes:
//    stream_id, port_index, key_sub_stream
void test_cxl_ide_key_prog_invalid_params (
  const void *doe_context,
  void *spdm_context, const uint32_t *session_id, int stream_id,
  uint8_t key_sub_stream, uint8_t port_index,
  const cxl_ide_km_aes_256_gcm_key_buffer_t *key_buffer, uint8_t* kp_ack_status,
  int case_class, int case_id)
{
    libspdm_return_t status;
    cxl_ide_km_key_prog_teeio_t request;
    cxl_ide_km_kp_ack_t response;
    size_t response_size;
    size_t request_size;

    bool res = false;
    teeio_test_result_t assertion_result = TEEIO_TEST_RESULT_NOT_TESTED;

    libspdm_zero_mem (&request, sizeof(request));
    request.header.object_id = CXL_IDE_KM_OBJECT_ID_KEY_PROG;
    request.stream_id = stream_id;
    request.key_sub_stream = key_sub_stream;
    request.port_index = port_index;
    libspdm_copy_mem (&request.key_buffer, sizeof(request.key_buffer),
                      key_buffer, sizeof(cxl_ide_km_aes_256_gcm_key_buffer_t));

    response_size = sizeof(response);
    request_size = sizeof(request);
    status = cxl_ide_km_send_receive_data(spdm_context, session_id,
                                          &request, request_size,
                                          &response, &response_size);
    libspdm_zero_mem (&request.key_buffer, sizeof(cxl_ide_km_aes_256_gcm_key_buffer_t));

    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_send_receive_data failed with status 0x%x\n", status));
        teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED, "cxl_ide_km_send_receive_data failed with 0x%x", status);
        return;
    }

    // Assertion.1
    res = response_size == sizeof(cxl_ide_km_kp_ack_t);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 1, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "sizeof(CxlIdeKmMessage) = 0x%lx", response_size);
    if(!res) {
        return;
    }

    // Assertion.2
    res = (response.header.object_id == CXL_IDE_KM_OBJECT_ID_KP_ACK);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 2, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "CxlIdeKmMessage.ObjectID = 0x%x", response.header.object_id);

    // Assertion.3
    res = response.status == CXL_IDE_KM_KP_ACK_STATUS_INVALID;
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 3, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "CxlIdeKmMessage.Status = 0x%x", response.status);

    // Assertion.4
    res = response.port_index == request.port_index;
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 4, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "CxlIdeKmMessage.PortIndex = 0x%x", response.port_index);

    // Assertion.5
    res = response.stream_id == request.stream_id;
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 5, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "CxlIdeKmMessage.StreamID = 0x%x", response.stream_id);

    // Assertion.6
    res = response.key_sub_stream == request.key_sub_stream;
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 6, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "CxlIdeKmMessage.key_sub_stream = 0x%x", response.key_sub_stream);

    *kp_ack_status = response.status;
}

bool cxl_ide_test_key_prog_3_setup(void *test_context)
{
  // Cxl.Query has been called in test_group.setup()

  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  cxl_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  if(group_context->common.lower_port.cxl_data.query_resp.max_port_index == 0xff) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Skip KeyProg.3 because max_port_index == 0xff.\n"));
    case_context->action = IDE_COMMON_TEST_ACTION_SKIP;
  }

  return true;
}

bool cxl_ide_test_key_prog_3_run(void *test_context)
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

  int port_index_cnt = construct_test_port_index(lower_port->cxl_data.query_resp.max_port_index);
  TEEIO_ASSERT (port_index_cnt > 0);

  char case_info[MAX_LINE_LENGTH] = {0};
  for(int i = 0; i < port_index_cnt; i++) {
    uint8_t port_index = mPortIndex[i];
    sprintf(case_info, "  port_index = 0x%02x", port_index);
    test_cxl_ide_key_prog(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context,
                          &group_context->spdm_doe.session_id, 0, CXL_IDE_KM_KEY_SUB_STREAM_CXL,
                          port_index, false, case_info,
                          case_class, case_id, test_cxl_ide_key_prog_invalid_params);
  }

  return true;
}

bool cxl_ide_test_key_prog_3_teardown(void *test_context)
{
  return true;
}
