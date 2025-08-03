/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>

#include "assert.h"
#include "hal/base.h"
#include "hal/library/debuglib.h"

#include "hal/library/memlib.h"
#include "library/spdm_requester_lib.h"
#include "library/pci_ide_km_requester_lib.h"
#include "ide_test.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "pcie_ide_lib.h"
#include "pcie_ide_test_lib.h"
#include "pcie_ide_test_internal.h"

bool test_ide_km_key_set_go(const void *pci_doe_context,
                                   void *spdm_context, const uint32_t *session_id,
                                   uint8_t stream_id, uint8_t key_sub_stream,
                                   uint8_t port_index,
                                   bool phase1, const char *case_info,
                                   int case_class, int case_id)
{
    libspdm_return_t status;
    pci_ide_km_k_set_go_t request;
    size_t request_size;
    pci_ide_km_k_gostop_ack_t response;
    size_t response_size;
    bool res;
    int base = phase1 ? 0 : 5;

    teeio_test_result_t assertion_result = TEEIO_TEST_RESULT_NOT_TESTED;

    teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_SEPARATOR, TEEIO_TEST_RESULT_NOT_TESTED, case_info);

    libspdm_zero_mem(&request, sizeof(request));
    request.header.object_id = PCI_IDE_KM_OBJECT_ID_K_SET_GO;
    request.stream_id = stream_id;
    request.key_sub_stream = key_sub_stream;
    request.port_index = port_index;

    request_size = sizeof(request);
    response_size = sizeof(response);
    status = pci_ide_km_send_receive_data(spdm_context, session_id,
                                          &request, request_size,
                                          &response, &response_size);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "ide_km_key_prog send receive_data failed with status 0x%x\n", status));
        teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED, "pci_ide_km_send_receive_data failed with 0x%x", status);
        return true;
    }

    // Assertion.1
    res = response_size == sizeof(pci_ide_km_kp_ack_t);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, base + 1, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "sizeof(IdeKmMessage) = 0x%lx", response_size);
    if(!res) {
        return true;
    }

    // Assertion.2
    res = response.header.object_id == PCI_IDE_KM_OBJECT_ID_K_SET_GOSTOP_ACK;
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, base + 2, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "IdeKmMessage.ObjectID = 0x%x", response.header.object_id);

    // Assertion.3
    res = (response.port_index == request.port_index);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, base + 3, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "IdeKmMessage.PortIndex = 0x%x", response.port_index);

    // Assertion.4
    res = (response.stream_id == request.stream_id);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, base + 4, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "IdeKmMessage.StreamID = 0x%x", response.stream_id);

    // Assertion.5
    res = (response.key_sub_stream == request.key_sub_stream);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, base + 5, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                  "IdeKmMessage.KeySet = 0x%02x && IdeKmMessage.RxTx = 0x%02x && IdeKmMessage.SubStream = 0x%02x",
                                  response.key_sub_stream & PCI_IDE_KM_KEY_SET_MASK,
                                  response.key_sub_stream & PCI_IDE_KM_KEY_DIRECTION_MASK,
                                  response.key_sub_stream & PCI_IDE_KM_KEY_SUB_STREAM_MASK);
    return true;
}

bool test_ksetgo_setup_common(
  void *doe_context, void* spdm_context,
  uint32_t* session_id, uint8_t* kcbar_addr,
  uint8_t stream_id, uint8_t rp_stream_index, uint8_t ide_id,
  ide_key_set_t *k_set, uint8_t port_index, uint8_t ks)
{
  uint8_t dev_func_num;
  uint8_t bus_num;
  uint8_t segment;
  uint8_t max_port_index;
  uint32_t ide_reg_block[PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT] = {0};
  uint32_t ide_reg_block_count;
  libspdm_return_t status;
  bool result;

  // first query
  ide_reg_block_count = PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT;
  status = pci_ide_km_query(doe_context,
                            spdm_context,
                            session_id,
                            port_index,
                            &dev_func_num,
                            &bus_num,
                            &segment,
                            &max_port_index,
                            ide_reg_block,
                            &ide_reg_block_count);
  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "pci_ide_km_query failed with status=0x%x\n", status));
    TEEIO_ASSERT(false);
    return false;
  }

  // then ide_km_key_prog
  result = ide_km_key_prog(
      doe_context, spdm_context,
      session_id, ks, PCIE_IDE_STREAM_RX, PCIE_IDE_SUB_STREAM_PR,
      port_index,
      stream_id,
      kcbar_addr,
      k_set,
      rp_stream_index);
  TEEIO_ASSERT(result);

  result = ide_km_key_prog(
      doe_context, spdm_context,
      session_id, ks, PCIE_IDE_STREAM_RX, PCIE_IDE_SUB_STREAM_NPR,
      port_index,
      stream_id,
      kcbar_addr,
      k_set,
      rp_stream_index);
  TEEIO_ASSERT(result);

  result = ide_km_key_prog(
      doe_context, spdm_context,
      session_id, ks, PCIE_IDE_STREAM_RX, PCIE_IDE_SUB_STREAM_CPL,
      port_index,
      stream_id,
      kcbar_addr,
      k_set,
      rp_stream_index);
  TEEIO_ASSERT(result);

  prime_rp_ide_key_set(
      (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)kcbar_addr,
      rp_stream_index,
      PCIE_IDE_STREAM_RX,
      ks);

  result = ide_km_key_prog(
      doe_context, spdm_context,
      session_id, ks, PCIE_IDE_STREAM_TX, PCIE_IDE_SUB_STREAM_PR,
      port_index,
      stream_id,
      kcbar_addr,
      k_set,
      rp_stream_index);
  TEEIO_ASSERT(result);

  result = ide_km_key_prog(
      doe_context, spdm_context,
      session_id, ks, PCIE_IDE_STREAM_TX, PCIE_IDE_SUB_STREAM_NPR,
      port_index,
      stream_id,
      kcbar_addr,
      k_set,
      rp_stream_index);
  TEEIO_ASSERT(result);

  result = ide_km_key_prog(
      doe_context, spdm_context,
      session_id, ks, PCIE_IDE_STREAM_TX, PCIE_IDE_SUB_STREAM_CPL,
      port_index,
      stream_id,
      kcbar_addr,
      k_set,
      rp_stream_index);
  TEEIO_ASSERT(result);

  prime_rp_ide_key_set(
      (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)kcbar_addr,
      rp_stream_index,
      PCIE_IDE_STREAM_TX,
      ks);

  return true;
}

// case 3.1
bool pcie_ide_test_ksetgo_1_setup(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);
  TEEIO_ASSERT(group_context->spdm_doe.spdm_context);
  TEEIO_ASSERT(group_context->spdm_doe.session_id);

  return test_ksetgo_setup_common(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context, &group_context->spdm_doe.session_id,
    group_context->common.upper_port.mapped_kcbar_addr, group_context->stream_id, group_context->rp_stream_index,
    group_context->common.upper_port.ide_id, &group_context->k_set, group_context->common.lower_port.port->port_index, PCIE_IDE_STREAM_KS0);
}

void pcie_ide_test_ksetgo_1_run(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_run_test_case_t* test_case = case_context->test_case;
  TEEIO_ASSERT(test_case);
  int case_class = test_case->class_id;
  int case_id = test_case->case_id;

  pcie_ide_test_group_context_t *group_context = (pcie_ide_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  uint8_t stream_id = group_context->stream_id;
  void *doe_context = group_context->spdm_doe.doe_context;
  void *spdm_context = group_context->spdm_doe.spdm_context;
  uint32_t session_id = group_context->spdm_doe.session_id;
  uint8_t port_index = group_context->common.lower_port.port->port_index;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K0|RX|PR\n"));
  test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_PR, port_index,
                             true, "K0|RX|PR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K0|RX|NPR\n"));
  test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, port_index,
                              true, "K0|RX|NPR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K0|RX|CPL\n"));
  test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, port_index,
                              true, "K0|RX|CPL", case_class, case_id);

  // set key_set_select in host ide
  INTEL_KEYP_ROOT_COMPLEX_KCBAR *kcbar = (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)group_context->common.upper_port.mapped_kcbar_addr;
  if(teeio_test_case_result(case_class, case_id) == TEEIO_TEST_RESULT_PASS) {
    set_rp_ide_key_set_select(kcbar, group_context->rp_stream_index, PCIE_IDE_STREAM_KS0);
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K0|TX|PR\n"));
  test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_PR, port_index,
                             true, "K0|TX|PR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K0|TX|NPR\n"));
  test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, port_index,
                              true, "K0|TX|NPR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K0|TX|CPL\n"));
  test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, port_index,
                              true, "K0|TX|CPL", case_class, case_id);

  if(teeio_test_case_result(case_class, case_id) != TEEIO_TEST_RESULT_PASS) {
    return;
  }

  // enable dev ide
  TEST_IDE_TYPE ide_type = TEST_IDE_TYPE_SEL_IDE;
  if (group_context->common.top->type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE)
  {
    ide_type = IDE_TEST_TOPOLOGY_TYPE_LINK_IDE;
  }
  else if(group_context->common.top->type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE)
  {
    NOT_IMPLEMENTED("selective_and_link_ide topoplogy");
  }
  enable_ide_stream_in_ecap(group_context->common.lower_port.cfg_space_fd, group_context->common.lower_port.ecap_offset, ide_type, group_context->common.lower_port.ide_id, true);

  // enable host ide stream
  enable_rootport_ide_stream(group_context->common.upper_port.cfg_space_fd,
                          group_context->common.upper_port.ecap_offset,
                          ide_type, group_context->common.upper_port.ide_id,
                          group_context->common.upper_port.mapped_kcbar_addr,
                          group_context->rp_stream_index, true);

  // wait for 10 ms for device to get ide ready
  libspdm_sleep(10 * 1000);
}

void pcie_ide_test_ksetgo_1_teardown(void *test_context)
{
  pcie_ide_teardown_common(test_context, PCI_IDE_KM_KEY_SET_K0);
}
