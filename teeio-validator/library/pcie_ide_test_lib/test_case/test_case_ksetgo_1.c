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

static const char* mAssertionMsg[] = {
    "ide_km_set_go send_receive_data",
    "sizeof(IdeKmMessage) == sizeof(K_GOSTOP_ACK)",
    "IdeKmMessage.ObjectID == K_GOSTOP_ACK",
    "IdeKmMessage.PortIndex == KEY_PROG.PortIndex",
    "IdeKmMessage.StreamID == KEY_PROG.StreamID",
    "IdeKmMessage.KeySet == KEY_PROG.KeySet && IdeKmMessage.RxTx == KEY_PROG.RxTx && IdeKmMessage.SubStream == KEY_PROG.SubStream",
};

bool ide_km_key_prog(
    const void *pci_doe_context,
    void *spdm_context,
    const uint32_t *session_id,
    uint8_t ks,
    uint8_t direction,
    uint8_t substream,
    uint8_t port_index,
    uint8_t stream_id,
    uint8_t *kcbar_addr,
    ide_key_set_t *k_set,
    uint8_t rp_stream_index);

libspdm_return_t test_ide_km_key_set_go(const void *pci_doe_context,
                                   void *spdm_context, const uint32_t *session_id,
                                   uint8_t stream_id, uint8_t key_sub_stream,
                                   uint8_t port_index,
                                   bool phase1, const char *assertion_msg)
{
    libspdm_return_t status;
    pci_ide_km_k_set_go_t request;
    size_t request_size;
    pci_ide_km_k_gostop_ack_t response;
    size_t response_size;
    bool res;
    int base = phase1 ? 0 : 6;

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
    if (LIBSPDM_STATUS_IS_ERROR(status))
    {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[idetest] ide_km_set_go send_receive_data failed with 0x%x\n", status));
        return status;
    }

    res = response_size == sizeof(pci_ide_km_k_gostop_ack_t);
    TEEIO_PRINT(("         %s.%d: %s %s\n", assertion_msg, base + 1, mAssertionMsg[1], res ? "Pass" : "Fail"));
    if(!res) {
        return LIBSPDM_STATUS_INVALID_MSG_SIZE;
    }

    res = response.header.object_id == PCI_IDE_KM_OBJECT_ID_K_SET_GOSTOP_ACK;
    TEEIO_PRINT(("         %s.%d: %s %s\n", assertion_msg, base + 2, mAssertionMsg[2], res ? "Pass" : "Fail"));
    if(!res)
    {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    res = response.port_index == request.port_index;
    TEEIO_PRINT(("         %s.%d: %s %s\n", assertion_msg, base + 3, mAssertionMsg[3], res ? "Pass" : "Fail"));
    if(!res)
    {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    res = response.stream_id == request.stream_id;
    TEEIO_PRINT(("         %s.%d: %s %s\n", assertion_msg, base + 4, mAssertionMsg[4], res ? "Pass" : "Fail"));
    if(!res)
    {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    res = response.key_sub_stream == request.key_sub_stream;
    TEEIO_PRINT(("         %s.%d: %s %s\n", assertion_msg, base + 5, mAssertionMsg[5], res ? "Pass" : "Fail"));
    if(!res)
    {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    return LIBSPDM_STATUS_SUCCESS;
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

  ide_common_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);
  TEEIO_ASSERT(group_context->spdm_context);
  TEEIO_ASSERT(group_context->session_id);

  return test_ksetgo_setup_common(group_context->doe_context, group_context->spdm_context, &group_context->session_id,
    group_context->upper_port.mapped_kcbar_addr, group_context->stream_id, group_context->rp_stream_index,
    group_context->upper_port.ide_id, group_context->k_set, 0, PCIE_IDE_STREAM_KS0);
}

bool pcie_ide_test_ksetgo_1_run(void *test_context)
{
  libspdm_return_t status;
  bool res = false;

  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_common_test_group_context_t *group_context = (ide_common_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  uint8_t stream_id = group_context->stream_id;
  void *doe_context = group_context->doe_context;
  void *spdm_context = group_context->spdm_context;
  uint32_t session_id = group_context->session_id;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K0|RX|PR\n"));
  status = test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_PR, 0,
                             true, "  Assertion 3.1");
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    goto Done;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K0|RX|NPR\n"));
  status = test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, 0,
                              true, "  Assertion 3.1");
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    goto Done;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K0|RX|CPL\n"));
  status = test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, 0,
                              true, "  Assertion 3.1");
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    goto Done;
  }

  // set key_set_select in host ide
  INTEL_KEYP_ROOT_COMPLEX_KCBAR *kcbar = (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)group_context->upper_port.mapped_kcbar_addr;
  set_rp_ide_key_set_select(kcbar, group_context->rp_stream_index, PCIE_IDE_STREAM_KS0);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K0|TX|PR\n"));
  status = test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_PR, 0,
                             true, "  Assertion 3.1");
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    goto Done;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K0|TX|NPR\n"));
  status = test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, 0,
                              true, "  Assertion 3.1");
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    goto Done;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K0|TX|CPL\n"));
  status = test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, 0,
                              true, "  Assertion 3.1");
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    goto Done;
  }

  // enable dev ide
  TEST_IDE_TYPE ide_type = TEST_IDE_TYPE_SEL_IDE;
  if (group_context->top->type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE)
  {
    ide_type = IDE_TEST_TOPOLOGY_TYPE_LINK_IDE;
  }
  else if(group_context->top->type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE)
  {
    NOT_IMPLEMENTED("selective_and_link_ide topoplogy");
  }
  enable_ide_stream_in_ecap(group_context->lower_port.cfg_space_fd, group_context->lower_port.ecap_offset, ide_type, group_context->lower_port.ide_id, true);

  // enable host ide stream
  enable_rootport_ide_stream(group_context->upper_port.cfg_space_fd,
                         group_context->upper_port.ecap_offset,
                         ide_type, group_context->upper_port.ide_id,
                         group_context->upper_port.mapped_kcbar_addr,
                         group_context->rp_stream_index, true);

  res = true;

  // wait for 10 ms for device to get ide ready
  libspdm_sleep(10 * 1000);

Done:
  case_context->result = res ? IDE_COMMON_TEST_CASE_RESULT_SUCCESS : IDE_COMMON_TEST_CASE_RESULT_FAILED;

  return true;
}

bool pcie_ide_test_ksetgo_1_teardown(void *test_context)
{
  return true;
}
