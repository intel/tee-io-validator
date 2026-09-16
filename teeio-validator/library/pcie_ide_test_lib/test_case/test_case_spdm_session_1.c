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

// Session-y created in setup and used/closed within this case; Session-x (the
// group's primary session) is left untouched for the rest of the test group.
static uint32_t m_spdm_session_1_session_y = 0;

/**
 * Case 5.1
 * IDE_KM responder shall discard QUERY/KEY_PROG received on a second SPDM session
 * while the first (port-owning) session is still active.
 */
bool pcie_ide_test_spdm_session_1_setup(void *test_context)
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

  // TestSetup: bind port_index=0 to Session-x with QUERY then KEY_PROG.
  uint8_t dev_func_num, bus_num, segment, max_port_index;
  uint32_t ide_reg_block[PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT] = {0};
  uint32_t ide_reg_block_count = PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT;
  libspdm_return_t status = pci_ide_km_query(doe_context, spdm_context, &session_id_x,
                                             0, &dev_func_num, &bus_num, &segment,
                                             &max_port_index, ide_reg_block, &ide_reg_block_count);
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "SpdmSession.1 setup: query on Session-x failed with 0x%x\n", status));
    return false;
  }

  pci_ide_km_aes_256_gcm_key_buffer_t key_buffer = {0};
  uint8_t kp_ack_status = 0;
  status = pci_ide_km_key_prog(doe_context, spdm_context, &session_id_x,
                               group_context->stream_id,
                               PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_PR,
                               0, &key_buffer, &kp_ack_status);
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "SpdmSession.1 setup: key_prog on Session-x failed with 0x%x\n", status));
    return false;
  }

  // Create Session-y on the same spdm_context.
  uint32_t session_id_y = 0;
  if (!spdm_start_session(spdm_context, &session_id_y)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "SpdmSession.1 setup: failed to create Session-y\n"));
    return false;
  }

  m_spdm_session_1_session_y = session_id_y;

  return true;
}

void pcie_ide_test_spdm_session_1_run(void *test_context)
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

  void *doe_context = group_context->spdm_doe.doe_context;
  void *spdm_context = group_context->spdm_doe.spdm_context;
  uint32_t session_id_y = m_spdm_session_1_session_y;

  bool res;
  teeio_test_result_t assertion_result;

  // Assertion 5.1.1: QUERY on Session-y shall be discarded (no valid response).
  uint8_t dev_func_num, bus_num, segment, max_port_index;
  uint32_t ide_reg_block[PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT] = {0};
  uint32_t ide_reg_block_count = PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT;
  libspdm_return_t status = pci_ide_km_query(doe_context, spdm_context, &session_id_y,
                                             0, &dev_func_num, &bus_num, &segment,
                                             &max_port_index, ide_reg_block, &ide_reg_block_count);
  res = LIBSPDM_STATUS_IS_ERROR(status);
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 1, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                "IdeKmMessage == NULL (QUERY on Session-y), status = 0x%x", status);

  // Assertion 5.1.2: KEY_PROG on Session-y shall be discarded (no valid response).
  pci_ide_km_aes_256_gcm_key_buffer_t key_buffer = {0};
  uint8_t kp_ack_status = 0;
  status = pci_ide_km_key_prog(doe_context, spdm_context, &session_id_y,
                               group_context->stream_id,
                               PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_PR,
                               0, &key_buffer, &kp_ack_status);
  res = LIBSPDM_STATUS_IS_ERROR(status);
  assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
  teeio_record_assertion_result(case_class, case_id, 2, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                "IdeKmMessage == NULL (KEY_PROG on Session-y), status = 0x%x", status);
}

void pcie_ide_test_spdm_session_1_teardown(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);

  // Only close Session-y; Session-x is owned by the test group and closed at group teardown.
  if (m_spdm_session_1_session_y != 0) {
    spdm_end_session(group_context->spdm_doe.spdm_context, m_spdm_session_1_session_y);
    m_spdm_session_1_session_y = 0;
  }
}
