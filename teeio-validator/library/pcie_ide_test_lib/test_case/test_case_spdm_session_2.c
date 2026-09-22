/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "library/spdm_requester_lib.h"
#include "library/pci_ide_km_requester_lib.h"
#include "ide_test.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "teeio_spdmlib.h"
#include "pcie_ide_test_internal.h"

// After setup, Session-x has been terminated and Session-y is the group's
// only active session; run()/teardown() operate on group_context->spdm_doe.session_id.

/**
 * Case 5.2
 * IDE_KM responder shall return valid QUERY/KEY_PROG responses on a second SPDM
 * session, once the first (port-owning) session has been terminated.
 */
bool pcie_ide_test_spdm_session_2_setup(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);
  TEEIO_ASSERT(group_context->spdm_doe.spdm_context);
  TEEIO_ASSERT(group_context->spdm_doe.session_id);

  void *doe_context = group_context->spdm_doe.doe_context;
  void *spdm_context = group_context->spdm_doe.spdm_context;
  uint32_t session_id_x = group_context->spdm_doe.session_id;
  uint8_t port_index = group_context->common.lower_port.port->port_index;

  uint8_t dev_func_num, bus_num, segment, max_port_index;
  uint32_t ide_reg_block[PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT] = {0};
  uint32_t ide_reg_block_count = PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT;
  libspdm_return_t status = pci_ide_km_query(doe_context, spdm_context, &session_id_x,
                                             0, &dev_func_num, &bus_num, &segment,
                                             &max_port_index, ide_reg_block, &ide_reg_block_count);
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "SpdmSession.2 setup: query on Session-x failed with 0x%x\n", status));
    return false;
  }

  pci_ide_km_aes_256_gcm_key_buffer_t key_buffer = {0};
  key_buffer.iv[1] = PCIE_IDE_IV_INIT_VALUE;
  uint8_t kp_ack_status = 0;
  status = pci_ide_km_key_prog(doe_context, spdm_context, &session_id_x,
                               group_context->stream_id,
                               PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_PR,
                               port_index, &key_buffer, &kp_ack_status);
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "SpdmSession.2 setup: key_prog on Session-x failed with 0x%x\n", status));
    return false;
  }
  if (kp_ack_status != PCI_IDE_KM_KP_ACK_STATUS_SUCCESS) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "SpdmSession.2 setup: key_prog on Session-x rejected with kp_ack_status=0x%x\n", kp_ack_status));
    return false;
  }

  // Terminate Session-x.
  if (!spdm_end_session(spdm_context, session_id_x)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "SpdmSession.2 setup: failed to terminate Session-x\n"));
    return false;
  }

  // Create Session-y and make it the group's active session so that later
  // run()/teardown() (including group teardown) operate on it.
  uint32_t session_id_y = 0;
  if (!spdm_start_session(spdm_context, &session_id_y)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "SpdmSession.2 setup: failed to create Session-y\n"));
    return false;
  }
  group_context->spdm_doe.session_id = session_id_y;

  return true;
}

void pcie_ide_test_spdm_session_2_run(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_run_test_case_t *test_case = case_context->test_case;
  TEEIO_ASSERT(test_case);
  int case_class = test_case->class_id;
  int case_id = test_case->case_id;

  pcie_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  void *spdm_context = group_context->spdm_doe.spdm_context;
  uint32_t session_id_y = group_context->spdm_doe.session_id;

  bool res;
  teeio_test_result_t assertion_result;

  uint8_t port_index = group_context->common.lower_port.port->port_index;
  uint8_t expected_dev_func = ((group_context->common.lower_port.port->device & 0x1f) << 3) |
                              (group_context->common.lower_port.port->function & 0x7);
  uint8_t expected_bus = group_context->common.lower_port.port->bus;
  uint8_t expected_segment = group_context->common.lower_port.port->segment;

  // QUERY on Session-y.
  pci_ide_km_query_t query_request = {0};
  query_request.header.object_id = PCI_IDE_KM_OBJECT_ID_QUERY;
  query_request.port_index = port_index;
  test_pci_ide_km_query_resp_t query_response = {0};
  size_t response_size = sizeof(query_response);
  libspdm_return_t status = pci_ide_km_send_receive_data(spdm_context, &session_id_y,
                                                        &query_request, sizeof(query_request),
                                                        &query_response, &response_size);
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  TEEIO_TEST_RESULT_FAILED, "QUERY on Session-y failed with 0x%x", status);
    return;
  }

  res = response_size >= sizeof(pci_ide_km_query_resp_t) &&
        response_size <= sizeof(query_response) &&
        (response_size - sizeof(pci_ide_km_query_resp_t)) % sizeof(uint32_t) == 0;
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 1, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                assertion_result, "sizeof(IdeKmMessage) = 0x%lx", (unsigned long)response_size);
  if (!res) {
    return;
  }

  res = query_response.header.object_id == PCI_IDE_KM_OBJECT_ID_QUERY_RESP;
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 2, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                assertion_result, "IdeKmMessage.ObjectID = 0x%x", query_response.header.object_id);

  res = query_response.port_index == query_request.port_index;
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 3, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                assertion_result, "IdeKmMessage.PortIndex = 0x%x", query_response.port_index);

  // Assertion 5.2.4
  res = (query_response.max_port_index >= port_index);
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 4, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                "IdeKmMessage.MaxPortIndex = 0x%x", query_response.max_port_index);

  // Assertion 5.2.5
  res = (query_response.dev_func_num == expected_dev_func) &&
        (query_response.bus_num == expected_bus) && (query_response.segment == expected_segment);
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 5, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                "IdeKmMessage.DevFunc = 0x%x, Bus = 0x%x, Segment = 0x%x",
                                query_response.dev_func_num, query_response.bus_num, query_response.segment);

  // KEY_PROG on Session-y.
  test_pci_ide_km_key_prog_t key_prog_request = {0};
  key_prog_request.header.object_id = PCI_IDE_KM_OBJECT_ID_KEY_PROG;
  key_prog_request.stream_id = group_context->stream_id;
  key_prog_request.key_sub_stream = PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_PR;
  key_prog_request.port_index = port_index;
  key_prog_request.key_buffer.iv[1] = PCIE_IDE_IV_INIT_VALUE;
  pci_ide_km_kp_ack_t key_prog_response = {0};
  response_size = sizeof(key_prog_response);
  status = pci_ide_km_send_receive_data(spdm_context, &session_id_y,
                                        &key_prog_request, sizeof(key_prog_request),
                                        &key_prog_response, &response_size);
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  TEEIO_TEST_RESULT_FAILED, "KEY_PROG on Session-y failed with 0x%x", status);
    return;
  }

  res = response_size == sizeof(key_prog_response);
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 6, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                assertion_result, "sizeof(IdeKmMessage) = 0x%lx", (unsigned long)response_size);
  if (!res) {
    return;
  }

  res = key_prog_response.header.object_id == PCI_IDE_KM_OBJECT_ID_KP_ACK;
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 7, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                assertion_result, "IdeKmMessage.ObjectID = 0x%x", key_prog_response.header.object_id);

  // Assertion 5.2.8
  res = (key_prog_response.status == PCI_IDE_KM_KP_ACK_STATUS_SUCCESS);
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 8, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                "IdeKmMessage.Status = 0x%02x", key_prog_response.status);

  res = key_prog_response.port_index == key_prog_request.port_index;
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 9, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                assertion_result, "IdeKmMessage.PortIndex = 0x%x", key_prog_response.port_index);

  res = key_prog_response.stream_id == key_prog_request.stream_id;
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 10, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                assertion_result, "IdeKmMessage.StreamID = 0x%x", key_prog_response.stream_id);

  res = key_prog_response.key_sub_stream == key_prog_request.key_sub_stream;
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 11, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                assertion_result, "IdeKmMessage.KeySubStream = 0x%x", key_prog_response.key_sub_stream);
}

void pcie_ide_test_spdm_session_2_teardown(void *test_context)
{
  // Session-y is now the group's active session; it is closed by the group's
  // own teardown (common_test_group_teardown), nothing case-specific to do here.
}
