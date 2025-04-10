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

extern const char* m_cxl_ide_mode_names[];

static void do_test_cxl_ide_kset_stop (
  const void *doe_context, void *spdm_context,
  const uint32_t *session_id,  uint8_t port_index, uint8_t key_sub_stream,
  int case_class, int case_id)
{
  libspdm_return_t status;
  cxl_ide_km_k_set_stop_t request;
  size_t request_size;
  cxl_ide_km_k_gostop_ack_t response;
  size_t response_size;

  bool res = false;
  teeio_test_result_t assertion_result = TEEIO_TEST_RESULT_NOT_TESTED;

  libspdm_zero_mem (&request, sizeof(request));
  request.header.object_id = CXL_IDE_KM_OBJECT_ID_K_SET_STOP;
  request.stream_id = 0;
  request.key_sub_stream = key_sub_stream;
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
  res = (response_size == sizeof(cxl_ide_km_k_gostop_ack_t));
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 1, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "sizeof(CxlIdeKmMessage) = 0x%lx", response_size);
  if(!res) {
      return;
  }

  // Assertion.2
  res = (response.header.object_id == CXL_IDE_KM_OBJECT_ID_K_SET_GOSTOP_ACK);
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

static void test_cxl_ide_kset_stop(const void *doe_context, void *spdm_context,
  const uint32_t *session_id, uint8_t port_index, CXL_IDE_MODE ide_mode,
  int case_class, int case_id)
{
  bool result;
  uint8_t kp_ack_status;
  libspdm_return_t status;
  char error_info[MAX_LINE_LENGTH] = {0};
  char case_info[MAX_LINE_LENGTH] = {0};
  sprintf(case_info, "  ide_mode = %s, port_index = 0x%02x", m_cxl_ide_mode_names[ide_mode], port_index);

  cxl_ide_km_aes_256_gcm_key_buffer_t rx_key_buffer = {0};
  cxl_ide_km_aes_256_gcm_key_buffer_t tx_key_buffer = {0};

  uint8_t cxl_ide_km_iv_rx = 0;
  uint8_t cxl_ide_km_iv_tx = 0;

  uint8_t ide_km_mode = CXL_IDE_KM_KEY_MODE_SKID;
  if(ide_mode == CXL_IDE_MODE_CONTAINMENT) {
    ide_km_mode = CXL_IDE_KM_KEY_MODE_CONTAINMENT;
  }

  // generate cxl-ide key/iv for RX direction
  result = cxl_ide_prepare_dynamic_keys(&rx_key_buffer, &cxl_ide_km_iv_rx);
  if (!result) {
    sprintf(error_info, "%s", "cxl_ide_prepare_dynamic_keys for RX failed");
    goto PrepareDone;
  }

  // generate cxl-ide key/iv for TX direction
  result = cxl_ide_prepare_dynamic_keys(&tx_key_buffer, &cxl_ide_km_iv_tx);
  if (!result) {
    sprintf(error_info, "%s", "cxl_ide_prepare_dynamic_keys for TX failed");
    goto PrepareDone;
  }

  // ide_km_key_prog in RX
  status = cxl_ide_km_key_prog(
    doe_context, spdm_context,
    session_id, 0,
    CXL_IDE_KM_KEY_DIRECTION_RX | cxl_ide_km_iv_rx | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
    port_index, // port_index
    &rx_key_buffer, &kp_ack_status);

  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    sprintf(error_info, "cxl_ide_km_key_prog RX failed with status=0x%08x\n", status);
    result = false;
    goto PrepareDone;
  }
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "key_prog RX - %02x\n", kp_ack_status));
  dump_key_iv_in_key_prog(rx_key_buffer.key, sizeof(rx_key_buffer.key)/sizeof(uint32_t), rx_key_buffer.iv, sizeof(rx_key_buffer.iv)/sizeof(uint32_t));

  // ide_km_key_prog in TX
  status = cxl_ide_km_key_prog(
    doe_context, spdm_context,
    session_id, 0,
    CXL_IDE_KM_KEY_DIRECTION_TX | cxl_ide_km_iv_tx | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
    port_index,
    &tx_key_buffer, &kp_ack_status);

  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    sprintf(error_info, "cxl_ide_km_key_prog TX failed with status=0x%08x\n", status);
    result = false;
    goto PrepareDone;
  }
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "key_prog TX - %02x\n", kp_ack_status));
  dump_key_iv_in_key_prog(tx_key_buffer.key, sizeof(tx_key_buffer.key)/sizeof(uint32_t), tx_key_buffer.iv, sizeof(tx_key_buffer.iv)/sizeof(uint32_t));

  // ide_km_kset_go in RX
  // KSetGo in RX
  status = cxl_ide_km_key_set_go (doe_context, spdm_context,
                                  session_id, 0,
                                  CXL_IDE_KM_KEY_DIRECTION_RX | ide_km_mode | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
                                  port_index);
  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_key_set_go RX failed with status=0x%08x\n", status));
    result = false;
    goto PrepareDone;
  }
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "key_set_go RX\n"));

  // ide_km_kset_go in TX
  // KSetGo in TX
  status = cxl_ide_km_key_set_go (doe_context, spdm_context,
                                  session_id, 0,
                                  CXL_IDE_KM_KEY_DIRECTION_TX | ide_km_mode | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
                                  port_index);
  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_key_set_go TX failed with status=0x%08x\n", status));
    result = false;
    goto PrepareDone;
  }
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "key_set_go TX\n"));

  result = true;

PrepareDone:
  if(!result) {
    teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_SEPARATOR, TEEIO_TEST_RESULT_NOT_TESTED, "%s", case_info);
    teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED, "%s", error_info);
    return;
  }

  // Now do the KSetStop test

  // RX
  teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_SEPARATOR, TEEIO_TEST_RESULT_NOT_TESTED, "%s RX", case_info);
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Test CXL_IDE KSetStop %s RX\n", case_info));
  do_test_cxl_ide_kset_stop(doe_context, spdm_context, session_id, port_index,
                            CXL_IDE_KM_KEY_DIRECTION_RX | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
                            case_class, case_id);

  // TX
  teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_SEPARATOR, TEEIO_TEST_RESULT_NOT_TESTED, "%s TX", case_info);
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Test CXL_IDE KSetStop %s TX\n", case_info));
  do_test_cxl_ide_kset_stop(doe_context, spdm_context, session_id, port_index,
                            CXL_IDE_KM_KEY_DIRECTION_TX | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
                            case_class, case_id);
}


bool cxl_ide_test_kset_stop_1_setup(void *test_context)
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

  bool k_set_stop_capable = false;
  CXL_QUERY_RESP_CAPS *caps = NULL;
  for(int i = 0; i <= max_port_index; i++) {
    caps = &m_dev_caps[i];
    if(caps->k_set_stop_capable == 0) {
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "CXL IDE K_SET_STOP (for port_index=%d) is not supported.\n", i));
    } else {
      k_set_stop_capable = true;
    }
  }

  if(k_set_stop_capable == false) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "CXL IDE K_SET_STOP is not supported in all the ports of the device. Skip the case.\n"));
    case_context->action = IDE_COMMON_TEST_ACTION_SKIP;
  }

  return true;
}

bool cxl_ide_test_kset_stop_1_run(void *test_context)
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

  for(int port_index = 0; port_index <=lower_port->cxl_data.query_resp.max_port_index; port_index++) {
    if(m_dev_caps[port_index].k_set_stop_capable == 0) {
      continue;
    }

    for(CXL_IDE_MODE ide_mode = CXL_IDE_MODE_CONTAINMENT; ide_mode < CXL_IDE_MODE_MAX; ide_mode++) {
      if(!cxl_ide_check_and_enable_ide_mode(group_context, ide_mode, port_index)) {
        continue;
      }

      test_cxl_ide_kset_stop (doe_context, spdm_context, &session_id,
                              port_index, ide_mode,
                              case_class, case_id);
    }
  }

  return true;
}

bool cxl_ide_test_kset_stop_1_teardown(void *test_context)
{
  return true;
}
