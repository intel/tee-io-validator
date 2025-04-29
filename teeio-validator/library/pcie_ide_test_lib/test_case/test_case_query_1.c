/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "hal/base.h"
#include "hal/library/debuglib.h"

#include "library/spdm_transport_pcidoe_lib.h"
#include "library/pci_ide_km_requester_lib.h"
#include "hal/library/memlib.h"
#include "ide_test.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "pcie_ide_test_internal.h"

/**
 * Send and receive an IDE_KM query message
 *
 * @param  spdm_context                 A pointer to the SPDM context.
 * @param  session_id                   Indicates if it is a secured message protected via SPDM session.
 *                                     If session_id is NULL, it is a normal message.
 *                                     If session_id is NOT NULL, it is a secured message.
 *
 * @retval LIBSPDM_STATUS_SUCCESS               The IDM_KM request is sent and response is received.
 * @return ERROR                        The IDM_KM response is not received correctly.
 **/
bool
pcie_ide_test_query(const void *pci_doe_context,
                      void *spdm_context, const uint32_t *session_id,
                      uint8_t port_index, uint8_t *dev_func_num,
                      uint8_t *bus_num, uint8_t *segment, uint8_t *max_port_index,
                      uint32_t *ide_reg_buffer, uint32_t *ide_reg_buffer_count,
                      int case_class, int case_id)
{
  libspdm_return_t status;
  pci_ide_km_query_t request;
  size_t request_size;
  test_pci_ide_km_query_resp_t response;
  size_t response_size;
  uint32_t ide_reg_count;
  bool res;

  teeio_test_result_t assertion_result = TEEIO_TEST_RESULT_NOT_TESTED;

  libspdm_zero_mem(&request, sizeof(request));
  request.header.object_id = PCI_IDE_KM_OBJECT_ID_QUERY;
  request.port_index = port_index;

  request_size = sizeof(request);
  response_size = sizeof(response);
  status = pci_ide_km_send_receive_data(spdm_context, session_id,
                                        &request, request_size,
                                        &response, &response_size);

  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "ide_km_kset_go send receive_data failed with status 0x%x\n", status));
    teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED, "pci_ide_km_send_receive_data failed with 0x%x", status);
    return true;
  }

  // assertion.1
  ide_reg_count = (uint32_t)(response_size - sizeof(pci_ide_km_query_resp_t)) / sizeof(uint32_t);
  res = (response_size == sizeof(pci_ide_km_query_resp_t) + ide_reg_count * sizeof(uint32_t));
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 1, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "sizeof(IdeKmMessage) = 0x%lx", response_size);
  if(!res) {
      return true;
  }

  // assertion.2
  res = response.header.object_id == PCI_IDE_KM_OBJECT_ID_QUERY_RESP;
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 2, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "IdeKmMessage.ObjectID = 0x%x", response.header.object_id);

  // assertion.3
  res = response.port_index == request.port_index;
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 3, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "IdeKmMessage.PortIndex = 0x%x", response.port_index);

  // assertion.4
  res = response.max_port_index >= request.port_index;
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 4, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "IdeKmMessage.MaxPortIndex = 0x%x", response.max_port_index);

  // assertion.5
  res = *dev_func_num == response.dev_func_num && *bus_num == response.bus_num && *segment == response.segment;
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 5, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                "IdeKmMessage.DevFunc = 0x%x && IdeKmMessage.Bus = 0x%x && IdeKmMessage.Segment = 0x%x",
                                response.dev_func_num,
                                response.bus_num,
                                response.segment);

  return true;
}

// case 1.1
bool pcie_ide_test_query_1_setup(void *test_context)
{
  return true;
}

void pcie_ide_test_query_1_run(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_run_test_case_t* test_case = case_context->test_case;
  TEEIO_ASSERT(test_case);
  int case_class = test_case->class_id;
  int case_id = test_case->case_id;

  pcie_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);
  TEEIO_ASSERT(group_context->spdm_doe.spdm_context);
  TEEIO_ASSERT(group_context->spdm_doe.session_id);

  uint8_t port_index = 0;
  uint8_t dev_func = ((group_context->common.lower_port.port->device & 0x1f) << 3) | (group_context->common.lower_port.port->function & 0x7);
  uint8_t bus = group_context->common.lower_port.port->bus;
  uint8_t segment = 0;
  uint8_t max_port_index = 0;
  uint32_t ide_reg_block[PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT] = {0};
  uint32_t ide_reg_block_count = PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT;

  pcie_ide_test_query(group_context->spdm_doe.doe_context,
                      group_context->spdm_doe.spdm_context,
                      &group_context->spdm_doe.session_id,
                      port_index, &dev_func, &bus, &segment,
                      &max_port_index, ide_reg_block, &ide_reg_block_count,
                      case_class, case_id);
}

void pcie_ide_test_query_1_teardown(void *test_context)
{
}
