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
#include "library/spdm_crypt_lib.h"
#include "ide_test.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "pcie_ide_lib.h"
#include "pcie_ide_test_lib.h"
#include "pcie_ide_test_internal.h"

uint8_t m_keyprog_max_port = 0;

bool test_keyprog_setup_common(void *test_context)
{
  uint8_t dev_func_num;
  uint8_t bus_num;
  uint8_t segment;
  uint8_t max_port_index;
  uint32_t ide_reg_block[PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT] = {0};
  uint32_t ide_reg_block_count;
  libspdm_return_t status;

  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);
  TEEIO_ASSERT(group_context->spdm_doe.spdm_context);
  TEEIO_ASSERT(group_context->spdm_doe.session_id);

  // query
  ide_reg_block_count = PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT;
  status = pci_ide_km_query(group_context->spdm_doe.doe_context,
                            group_context->spdm_doe.spdm_context,
                            &group_context->spdm_doe.session_id,
                            0,
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

  m_keyprog_max_port = max_port_index;

  return true;
}

bool test_ide_km_key_prog_case1(const void *pci_doe_context,
                                     void *spdm_context, const uint32_t *session_id,
                                     uint8_t stream_id, uint8_t key_sub_stream, uint8_t port_index,
                                     const pci_ide_km_aes_256_gcm_key_buffer_t *key_buffer,
                                     uint8_t *kp_ack_status, const char* case_info,
                                     int case_class, int case_id)
{
    libspdm_return_t status;
    test_pci_ide_km_key_prog_t request;
    size_t request_size;
    pci_ide_km_kp_ack_t response;
    size_t response_size;
    bool res = false;
    teeio_test_result_t assertion_result = TEEIO_TEST_RESULT_NOT_TESTED;

    teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_SEPARATOR, TEEIO_TEST_RESULT_NOT_TESTED, case_info);

    libspdm_zero_mem (&request, sizeof(request));
    request.header.object_id = PCI_IDE_KM_OBJECT_ID_KEY_PROG;
    request.stream_id = stream_id;
    request.key_sub_stream = key_sub_stream;
    request.port_index = port_index;
    libspdm_copy_mem (&request.key_buffer, sizeof(request.key_buffer),
                      key_buffer, sizeof(pci_ide_km_aes_256_gcm_key_buffer_t));

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
    teeio_record_assertion_result(case_class, case_id, 1, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "sizeof(IdeKmMessage) = 0x%lx", response_size);
    if(!res) {
        return true;
    }

    // Assertion.2
    res = response.header.object_id == PCI_IDE_KM_OBJECT_ID_KP_ACK;
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 2, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "IdeKmMessage.ObjectID = 0x%x", response.header.object_id);

    // Assertion.3
    res = response.status == PCI_IDE_KM_KP_ACK_STATUS_SUCCESS;
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 3, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "IdeKmMessage.Status = 0x%02x", response.status);

    // Assertion.4
    res = (response.port_index == request.port_index);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 4, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "IdeKmMessage.PortIndex = 0x%x", response.port_index);

    // Assertion.5
    res = (response.stream_id == request.stream_id);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 5, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "IdeKmMessage.StreamID = 0x%x", response.stream_id);

    // Assertion.6
    res = (response.key_sub_stream == request.key_sub_stream);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 6, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                  "IdeKmMessage.KeySet = 0x%02x && IdeKmMessage.RxTx = 0x%02x && IdeKmMessage.SubStream = 0x%02x",
                                   response.key_sub_stream & PCI_IDE_KM_KEY_SET_MASK,
                                   response.key_sub_stream & PCI_IDE_KM_KEY_DIRECTION_MASK,
                                   response.key_sub_stream & PCI_IDE_KM_KEY_SUB_STREAM_MASK);

    return true;
}

// KeyProg Case 2.1
bool pcie_ide_test_keyprog_1_setup(void *test_context)
{
  return test_keyprog_setup_common(test_context);
}

void pcie_ide_test_keyprog_1_run(void *test_context)
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

  INTEL_KEYP_KEY_SLOT keys = {0};
  INTEL_KEYP_IV_SLOT iv = {0};
  pci_ide_km_aes_256_gcm_key_buffer_t key_buffer;
  uint8_t kp_ack_status;
  INTEL_KEYP_ROOT_COMPLEX_KCBAR *kcbar_ptr = (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)group_context->common.upper_port.mapped_kcbar_addr;
  uint8_t slot_id;
  uint8_t ks;
  uint8_t direction;
  uint8_t substream;
  uint8_t port_index = group_context->common.lower_port.port->port_index;

  iv.bytes[0] = PCIE_IDE_IV_INIT_VALUE;

  uint8_t k_sets[] = {PCI_IDE_KM_KEY_SET_K0, PCI_IDE_KM_KEY_SET_K1};
  uint8_t directions[] = {PCI_IDE_KM_KEY_DIRECTION_RX, PCI_IDE_KM_KEY_DIRECTION_TX};
  uint8_t substreams[] = {PCI_IDE_KM_KEY_SUB_STREAM_PR, PCI_IDE_KM_KEY_SUB_STREAM_NPR, PCI_IDE_KM_KEY_SUB_STREAM_CPL};
  bool result = libspdm_get_random_number(sizeof(key_buffer.key), (void *)key_buffer.key);
  TEEIO_ASSERT(result);

  key_buffer.iv[0] = 0;
  key_buffer.iv[1] = 1;

  // KS0|RX|PR
  ks = PCIE_IDE_STREAM_KS0;
  direction = PCIE_IDE_STREAM_RX;
  substream = PCIE_IDE_SUB_STREAM_PR;
  result = libspdm_get_random_number(sizeof(key_buffer.key), (void *)key_buffer.key);
  TEEIO_ASSERT(result);
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KeyProg K0|RX|PR\n"));
  test_ide_km_key_prog_case1(doe_context, spdm_context, &session_id, stream_id,
                                k_sets[ks] | directions[direction] | substreams[substream],
                                port_index, &key_buffer, &kp_ack_status, "K0|RX|PR", case_class, case_id);
  dump_key_iv_in_key_prog(key_buffer.key, sizeof(key_buffer.key)/sizeof(uint32_t), key_buffer.iv, sizeof(key_buffer.iv)/sizeof(uint32_t));

  // program key in root port kcbar registers
  pcie_construct_rp_keys(key_buffer.key, sizeof(key_buffer.key), keys.bytes, sizeof(keys.bytes));
  slot_id = group_context->k_set.slot_id[direction][substream];
  cfg_rootport_ide_keys(kcbar_ptr, group_context->rp_stream_index, direction, ks, substream, slot_id, &keys, &iv);
  pcie_dump_key_iv_in_rp(direction == PCIE_IDE_STREAM_RX ? "TX" : "RX", (uint8_t *)keys.bytes, sizeof(keys.bytes), (uint8_t *)iv.bytes, sizeof(iv.bytes));

  // KS0|RX|NPR
  ks = PCIE_IDE_STREAM_KS0;
  direction = PCIE_IDE_STREAM_RX;
  substream = PCIE_IDE_SUB_STREAM_NPR;
  result = libspdm_get_random_number(sizeof(key_buffer.key), (void *)key_buffer.key);
  TEEIO_ASSERT(result);
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KeyProg K0|RX|NPR\n"));
  test_ide_km_key_prog_case1(doe_context, spdm_context, &session_id, stream_id,
                                k_sets[ks] | directions[direction] | substreams[substream],
                                port_index, &key_buffer, &kp_ack_status, "K0|RX|NPR", case_class, case_id);
  dump_key_iv_in_key_prog(key_buffer.key, sizeof(key_buffer.key)/sizeof(uint32_t), key_buffer.iv, sizeof(key_buffer.iv)/sizeof(uint32_t));

  // program key in root port kcbar registers
  pcie_construct_rp_keys(key_buffer.key, sizeof(key_buffer.key), keys.bytes, sizeof(keys.bytes));
  slot_id = group_context->k_set.slot_id[direction][substream];
  cfg_rootport_ide_keys(kcbar_ptr, group_context->rp_stream_index, direction, ks, substream, slot_id, &keys, &iv);
  pcie_dump_key_iv_in_rp(direction == PCIE_IDE_STREAM_RX ? "TX" : "RX", (uint8_t *)keys.bytes, sizeof(keys.bytes), (uint8_t *)iv.bytes, sizeof(iv.bytes));

  // KS0|RX|CPL
  ks = PCIE_IDE_STREAM_KS0;
  direction = PCIE_IDE_STREAM_RX;
  substream = PCIE_IDE_SUB_STREAM_CPL;
  result = libspdm_get_random_number(sizeof(key_buffer.key), (void *)key_buffer.key);
  TEEIO_ASSERT(result);
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KeyProg K0|RX|CPL\n"));
  test_ide_km_key_prog_case1(doe_context, spdm_context, &session_id, stream_id,
                                k_sets[ks] | directions[direction] | substreams[substream],
                                port_index, &key_buffer, &kp_ack_status, "K0|RX|CPL", case_class, case_id);
  dump_key_iv_in_key_prog(key_buffer.key, sizeof(key_buffer.key)/sizeof(uint32_t), key_buffer.iv, sizeof(key_buffer.iv)/sizeof(uint32_t));

  // program key in root port kcbar registers
  pcie_construct_rp_keys(key_buffer.key, sizeof(key_buffer.key), keys.bytes, sizeof(keys.bytes));
  slot_id = group_context->k_set.slot_id[direction][substream];
  cfg_rootport_ide_keys(kcbar_ptr, group_context->rp_stream_index, direction, ks, substream, slot_id, &keys, &iv);
  pcie_dump_key_iv_in_rp(direction == PCIE_IDE_STREAM_RX ? "TX" : "RX", (uint8_t *)keys.bytes, sizeof(keys.bytes), (uint8_t *)iv.bytes, sizeof(iv.bytes));

  prime_rp_ide_key_set(
      kcbar_ptr,
      group_context->rp_stream_index,
      PCIE_IDE_STREAM_RX,
      PCIE_IDE_STREAM_KS0);

  // KS0|TX|PR
  ks = PCIE_IDE_STREAM_KS0;
  direction = PCIE_IDE_STREAM_TX;
  substream = PCIE_IDE_SUB_STREAM_PR;
  result = libspdm_get_random_number(sizeof(key_buffer.key), (void *)key_buffer.key);
  TEEIO_ASSERT(result);
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KeyProg K0|TX|PR\n"));

  test_ide_km_key_prog_case1(doe_context, spdm_context, &session_id, stream_id,
                                k_sets[ks] | directions[direction] | substreams[substream],
                                port_index, &key_buffer, &kp_ack_status, "K0|TX|PR", case_class, case_id);
  dump_key_iv_in_key_prog(key_buffer.key, sizeof(key_buffer.key)/sizeof(uint32_t), key_buffer.iv, sizeof(key_buffer.iv)/sizeof(uint32_t));

  // program key in root port kcbar registers
  pcie_construct_rp_keys(key_buffer.key, sizeof(key_buffer.key), keys.bytes, sizeof(keys.bytes));
  slot_id = group_context->k_set.slot_id[direction][substream];
  cfg_rootport_ide_keys(kcbar_ptr, group_context->rp_stream_index, direction, ks, substream, slot_id, &keys, &iv);
  pcie_dump_key_iv_in_rp(direction == PCIE_IDE_STREAM_RX ? "TX" : "RX", (uint8_t *)keys.bytes, sizeof(keys.bytes), (uint8_t *)iv.bytes, sizeof(iv.bytes));

  // KS0|TX|NPR
  ks = PCIE_IDE_STREAM_KS0;
  direction = PCIE_IDE_STREAM_TX;
  substream = PCIE_IDE_SUB_STREAM_NPR;
  result = libspdm_get_random_number(sizeof(key_buffer.key), (void *)key_buffer.key);
  TEEIO_ASSERT(result);
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KeyProg K0|TX|NPR\n"));
  test_ide_km_key_prog_case1(doe_context, spdm_context, &session_id, stream_id,
                                k_sets[ks] | directions[direction] | substreams[substream],
                                port_index, &key_buffer, &kp_ack_status, "K0|TX|NPR", case_class, case_id);
  dump_key_iv_in_key_prog(key_buffer.key, sizeof(key_buffer.key)/sizeof(uint32_t), key_buffer.iv, sizeof(key_buffer.iv)/sizeof(uint32_t));

  // program key in root port kcbar registers
  pcie_construct_rp_keys(key_buffer.key, sizeof(key_buffer.key), keys.bytes, sizeof(keys.bytes));
  slot_id = group_context->k_set.slot_id[direction][substream];
  cfg_rootport_ide_keys(kcbar_ptr, group_context->rp_stream_index, direction, ks, substream, slot_id, &keys, &iv);
  pcie_dump_key_iv_in_rp(direction == PCIE_IDE_STREAM_RX ? "TX" : "RX", (uint8_t *)keys.bytes, sizeof(keys.bytes), (uint8_t *)iv.bytes, sizeof(iv.bytes));

  // KS0|TX|CPL
  ks = PCIE_IDE_STREAM_KS0;
  direction = PCIE_IDE_STREAM_TX;
  substream = PCIE_IDE_SUB_STREAM_CPL;
  result = libspdm_get_random_number(sizeof(key_buffer.key), (void *)key_buffer.key);
  TEEIO_ASSERT(result);
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KeyProg K0|TX|CPL\n"));
  test_ide_km_key_prog_case1(doe_context, spdm_context, &session_id, stream_id,
                                k_sets[ks] | directions[direction] | substreams[substream],
                                port_index, &key_buffer, &kp_ack_status, "K0|TX|CPL", case_class, case_id);
  dump_key_iv_in_key_prog(key_buffer.key, sizeof(key_buffer.key)/sizeof(uint32_t), key_buffer.iv, sizeof(key_buffer.iv)/sizeof(uint32_t));

  // program key in root port kcbar registers
  pcie_construct_rp_keys(key_buffer.key, sizeof(key_buffer.key), keys.bytes, sizeof(keys.bytes));
  slot_id = group_context->k_set.slot_id[direction][substream];
  cfg_rootport_ide_keys(kcbar_ptr, group_context->rp_stream_index, direction, ks, substream, slot_id, &keys, &iv);
  pcie_dump_key_iv_in_rp(direction == PCIE_IDE_STREAM_RX ? "TX" : "RX", (uint8_t *)keys.bytes, sizeof(keys.bytes), (uint8_t *)iv.bytes, sizeof(iv.bytes));

  prime_rp_ide_key_set(
      kcbar_ptr,
      group_context->rp_stream_index,
      PCIE_IDE_STREAM_TX,
      PCIE_IDE_STREAM_KS0);
}

void pcie_ide_test_keyprog_1_teardown(void *test_context)
{
}
