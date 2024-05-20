/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>

#include "hal/base.h"
#include "hal/library/debuglib.h"

#include "library/spdm_transport_pcidoe_lib.h"
#include "library/pci_ide_km_requester_lib.h"
#include "hal/library/memlib.h"
#include "ide_test.h"
#include "utils.h"
#include "teeio_debug.h"

#pragma pack(1)
typedef struct
{
  pci_ide_km_header_t header;
  uint8_t reserved;
  uint8_t port_index;
  uint8_t dev_func_num;
  uint8_t bus_num;
  uint8_t segment;
  uint8_t max_port_index;
  uint32_t ide_reg_block[PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT];
} test_pci_ide_km_query_resp_t;
#pragma pack()

static const char *mAssertion[] = {
    "ide_km_query send_receive_data",
    "sizeof(IdeKmMessage) == sizeof(QUERY_RESP)",
    "IdeKmMessage.ObjectID == QUERY_RESP",
    "IdeKmMessage.PortIndex == QUERY.PortIndex",
    "IdeKmMessage.MaxPortIndex >= QUERY.PortIndex",
    "IdeKmMessage.DevFunc == PCI.DevFunc && IdeKmMessage.Bus == PCI.Bus && IdeKmMessage.Segment == PCI.Segment"};

static uint8_t mMaxPortIndex = 0;

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
static libspdm_return_t
test_pci_ide_km_query(const void *pci_doe_context,
                      void *spdm_context, const uint32_t *session_id,
                      uint8_t port_index, uint8_t *dev_func_num,
                      uint8_t *bus_num, uint8_t *segment, uint8_t *max_port_index,
                      uint32_t *ide_reg_buffer, uint32_t *ide_reg_buffer_count,
                      bool assertion, const char *assertion_msg)
{
  libspdm_return_t status;
  pci_ide_km_query_t request;
  size_t request_size;
  test_pci_ide_km_query_resp_t response;
  size_t response_size;
  uint32_t ide_reg_count;
  bool res;

  libspdm_zero_mem(&request, sizeof(request));
  request.header.object_id = PCI_IDE_KM_OBJECT_ID_QUERY;
  request.port_index = port_index;

  request_size = sizeof(request);
  response_size = sizeof(response);
  status = pci_ide_km_send_receive_data(spdm_context, session_id,
                                        &request, request_size,
                                        &response, &response_size);

  // assertion.0
  if (assertion)
  {
    TEEIO_PRINT(("         %s.0: %s %s.\n", assertion_msg, mAssertion[0], !LIBSPDM_STATUS_IS_ERROR(status) ? "Pass" : "failed"));
  }
  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    return status;
  }

  // assertion.1
  ide_reg_count = (uint32_t)(response_size - sizeof(pci_ide_km_query_resp_t)) / sizeof(uint32_t);
  res = (response_size == sizeof(pci_ide_km_query_resp_t) + ide_reg_count * sizeof(uint32_t));
  if (assertion)
  {
    TEEIO_PRINT(("         %s.1: %s %s\n", assertion_msg, mAssertion[1], res ? "Pass" : "Fail"));
  }
  if (!res)
  {
    return LIBSPDM_STATUS_INVALID_MSG_SIZE;
  }

  if (*ide_reg_buffer_count < ide_reg_count)
  {
    *ide_reg_buffer_count = ide_reg_count;
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Buffer too small in pci_ide_km_query.\n"));
    return LIBSPDM_STATUS_BUFFER_TOO_SMALL;
  }

  // assertion.2
  res = response.header.object_id == PCI_IDE_KM_OBJECT_ID_QUERY_RESP;
  if (assertion)
  {
    TEEIO_PRINT(("         %s.2: %s %s\n", assertion_msg, mAssertion[2], res ? "Pass" : "Fail"));
  }
  if (!res)
  {
    return LIBSPDM_STATUS_INVALID_MSG_FIELD;
  }

  // assertion.3
  res = response.port_index == request.port_index;
  if (assertion)
  {
    TEEIO_PRINT(("         %s.3: %s %s\n", assertion_msg, mAssertion[3], res ? "Pass" : "Fail"));
  }
  if (!res)
  {
    return LIBSPDM_STATUS_INVALID_MSG_FIELD;
  }

  // assertion.4
  res = response.max_port_index >= request.port_index;
  if (assertion)
  {
    TEEIO_PRINT(("         %s.4: %s %s\n", assertion_msg, mAssertion[4], res ? "Pass" : "Fail"));
  }
  if (!res)
  {
    return LIBSPDM_STATUS_INVALID_MSG_FIELD;
  }

  // assertion.5
  res = *dev_func_num == response.dev_func_num && *bus_num == response.bus_num && *segment == response.segment;
  if (assertion)
  {
    TEEIO_PRINT(("         %s.5: %s %s\n", assertion_msg, mAssertion[5], res ? "Pass" : "Fail"));
  }
  if (!res)
  {
    return LIBSPDM_STATUS_INVALID_MSG_FIELD;
  }

  *dev_func_num = response.dev_func_num;
  *bus_num = response.bus_num;
  *segment = response.segment;
  *max_port_index = response.max_port_index;
  libspdm_copy_mem(ide_reg_buffer,
                   *ide_reg_buffer_count * sizeof(uint32_t),
                   response.ide_reg_block,
                   ide_reg_count * sizeof(uint32_t));
  *ide_reg_buffer_count = ide_reg_count;

  return LIBSPDM_STATUS_SUCCESS;
}

// case 1.1
bool test_query_1_setup(void *test_context)
{
  return true;
}

bool test_query_1_run(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_common_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);
  TEEIO_ASSERT(group_context->spdm_context);
  TEEIO_ASSERT(group_context->session_id);

  uint8_t port_index = 0;
  uint8_t dev_func = ((group_context->lower_port.port->device & 0x1f) << 3) | (group_context->lower_port.port->function & 0x7);
  uint8_t bus = group_context->lower_port.port->bus;
  uint8_t segment = 0;
  uint8_t max_port_index = 0;
  uint32_t ide_reg_block[PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT] = {0};
  uint32_t ide_reg_block_count = PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT;

  libspdm_return_t status = test_pci_ide_km_query(group_context->doe_context,
                                                  group_context->spdm_context,
                                                  &group_context->session_id,
                                                  port_index, &dev_func, &bus, &segment,
                                                  &max_port_index, ide_reg_block, &ide_reg_block_count,
                                                  true, "Assertion 1.1");

  case_context->result = LIBSPDM_STATUS_IS_SUCCESS(status) ? IDE_COMMON_TEST_CASE_RESULT_SUCCESS : IDE_COMMON_TEST_CASE_RESULT_FAILED;

  return true;
}

bool test_query_1_teardown(void *test_context)
{
  return true;
}

// case 1.2
bool test_query_2_setup(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_common_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);
  TEEIO_ASSERT(group_context->spdm_context);
  TEEIO_ASSERT(group_context->session_id);

  uint8_t port_index = 0;
  uint8_t dev_func = ((group_context->lower_port.port->device & 0x1f) << 3) | (group_context->lower_port.port->function & 0x7);
  uint8_t bus = group_context->lower_port.port->bus;
  uint8_t segment = 0;
  uint8_t max_port_index = 0;
  uint32_t ide_reg_block[PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT] = {0};
  uint32_t ide_reg_block_count = PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT;

  libspdm_return_t status = test_pci_ide_km_query(group_context->doe_context,
                                                  group_context->spdm_context, &group_context->session_id,
                                                  port_index, &dev_func, &bus, &segment,
                                                  &max_port_index, ide_reg_block, &ide_reg_block_count,
                                                  false, "");

  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    return false;
  }

  if (max_port_index == 0)
  {
    // skip the case
    case_context->action = IDE_COMMON_TEST_ACTION_SKIP;
  }

  mMaxPortIndex = max_port_index;

  return true;
}

bool test_query_2_run(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_common_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);
  TEEIO_ASSERT(group_context->spdm_context);
  TEEIO_ASSERT(group_context->session_id);

  uint8_t port_index = mMaxPortIndex;
  uint8_t dev_func = ((group_context->lower_port.port->device & 0x1f) << 3) | (group_context->lower_port.port->function & 0x7);
  uint8_t bus = group_context->lower_port.port->bus;
  uint8_t segment = 0;
  uint8_t max_port_index = 0;
  uint32_t ide_reg_block[PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT] = {0};
  uint32_t ide_reg_block_count = PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT;

  libspdm_return_t status = test_pci_ide_km_query(group_context->doe_context,
                                                  group_context->spdm_context,
                                                  &group_context->session_id,
                                                  port_index, &dev_func, &bus, &segment,
                                                  &max_port_index, ide_reg_block, &ide_reg_block_count,
                                                  true, "Assertion 2.1");

  case_context->result = LIBSPDM_STATUS_IS_SUCCESS(status) ? IDE_COMMON_TEST_CASE_RESULT_SUCCESS : IDE_COMMON_TEST_CASE_RESULT_FAILED;

  return true;
}

bool test_query_2_teardown(void *test_context)
{
  mMaxPortIndex = 0;
  return true;
}
