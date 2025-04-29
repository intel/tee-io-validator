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

// store the dev_caps for all the ports of the device
static CXL_QUERY_RESP_CAPS m_dev_caps[0x100] = {0};

static void test_cxl_ide_get_key(
  const void *doe_context, void *spdm_context,
  const uint32_t *session_id, uint8_t port_index,
  const char* case_info, int case_class, int case_id)
{
  libspdm_return_t status;
  cxl_ide_km_get_key_t request;
  size_t request_size;
  cxl_ide_km_get_key_ack_teeio_t response;
  size_t response_size;

  bool res = false;
  teeio_test_result_t assertion_result = TEEIO_TEST_RESULT_NOT_TESTED;

  teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_SEPARATOR, TEEIO_TEST_RESULT_NOT_TESTED, "%s", case_info);

  libspdm_zero_mem (&request, sizeof(request));
  request.header.object_id = CXL_IDE_KM_OBJECT_ID_GET_KEY;
  request.stream_id = 0;
  request.key_sub_stream = CXL_IDE_KM_KEY_SUB_STREAM_CXL;
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
  res = (response_size == sizeof(cxl_ide_km_get_key_ack_teeio_t));
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 1, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "sizeof(CxlIdeKmMessage) = 0x%lx", response_size);
  if(!res) {
      return;
  }

  // Assertion.2
  res = (response.header.object_id == CXL_IDE_KM_OBJECT_ID_GET_KEY_ACK);
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 2, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "CxlIdeKmMessage.ObjectID = 0x%x", response.header.object_id);

  // Assertion.3
  res = response.port_index == request.port_index;
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 3, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "CxlIdeKmMessage.PortIndex = 0x%x", response.port_index);

  // Assertion.4
  res = response.stream_id == request.stream_id;
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 4, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "CxlIdeKmMessage.StreamID = 0x%x", response.stream_id);

  // Assertion.5
  res = response.key_sub_stream == request.key_sub_stream;
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 5, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "CxlIdeKmMessage.key_sub_stream = 0x%x", response.key_sub_stream);
}

bool cxl_ide_test_get_key_1_setup(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  cxl_ide_test_group_context_t *group_context = (cxl_ide_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  int max_port_index = group_context->common.lower_port.cxl_data.query_resp.max_port_index;

  // Before running the test, we need to check if the device supports IDE Key/IV Generation.
  if(!cxl_ide_get_dev_caps(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context,
                          &group_context->spdm_doe.session_id, max_port_index, &m_dev_caps[0])) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_get_dev_caps failed.\n"));
    return false;
  }

  bool ide_key_iv_generation_capable = false;
  CXL_QUERY_RESP_CAPS *caps = NULL;
  for(int i = 0; i <= max_port_index; i++) {
    caps = &m_dev_caps[i];
    if(caps->ide_key_generation_capable == 0 || caps->iv_generation_capable == 0) {
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "CXL IDE Key/IV Generation (for port_index=%d) is not supported.(key=%d, iv=%d)\n",
        i, caps->ide_key_generation_capable, caps->iv_generation_capable));
    } else {
      ide_key_iv_generation_capable = true;
    }
  }

  if(ide_key_iv_generation_capable == false) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "CXL IDE Key/IV Generation is not supported in all the ports of the device. Skip the case.\n"));
    case_context->action = IDE_COMMON_TEST_ACTION_SKIP;
  }

  return true;
}

void cxl_ide_test_get_key_1_run(void *test_context)
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
  void* doe_context = group_context->spdm_doe.doe_context;
  void* spdm_context = group_context->spdm_doe.spdm_context;
  uint32_t session_id = group_context->spdm_doe.session_id;

  char case_info[MAX_LINE_LENGTH] = {0};
  for(int port_index = 0; port_index <=lower_port->cxl_data.query_resp.max_port_index; port_index++) {
    if(m_dev_caps[port_index].ide_key_generation_capable == 1 && m_dev_caps[port_index].iv_generation_capable == 1) {
      sprintf(case_info, "  port_index = 0x%02x", port_index);
      test_cxl_ide_get_key(doe_context, spdm_context,
                            &session_id, port_index,
                            case_info, case_class, case_id);
    }
  }
}

void cxl_ide_test_get_key_1_teardown(void *test_context)
{
}
