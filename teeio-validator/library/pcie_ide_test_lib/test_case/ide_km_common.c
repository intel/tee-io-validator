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
#include "library/spdm_transport_pcidoe_lib.h"
#include "library/pci_ide_km_requester_lib.h"
#include "library/spdm_crypt_lib.h"
#include "ide_test.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "pcie_ide_lib.h"
#include "pcie_ide_test_lib.h"

const char *k_set_names[] = {
    "KS0", "KS1"};

const char *direction_names[] = {
    "RX", "TX"};

const char *substream_names[] = {
    "PR", "NPR", "CPL"};

void dump_key_iv(pci_ide_km_aes_256_gcm_key_buffer_t* key_buffer)
{
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Key:\n"));
  dump_hex_array((uint8_t *)key_buffer->key, sizeof(key_buffer->key));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "IV:\n"));
  dump_hex_array((uint8_t *)key_buffer->iv, sizeof(key_buffer->iv));
}

// program keys to device card and root port
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
    uint8_t rp_stream_index)
{
    bool result;
    pci_ide_km_aes_256_gcm_key_buffer_t key_buffer;
    uint8_t kp_ack_status;
    uint8_t slot_id;
    libspdm_return_t status;
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *kcbar_ptr = 0;
    INTEL_KEYP_KEY_SLOT keys = {0};
    INTEL_KEYP_IV_SLOT iv = {0};

    uint8_t k_sets[] = {PCI_IDE_KM_KEY_SET_K0, PCI_IDE_KM_KEY_SET_K1};
    uint8_t directions[] = {PCI_IDE_KM_KEY_DIRECTION_RX, PCI_IDE_KM_KEY_DIRECTION_TX};
    uint8_t substreams[] = {PCI_IDE_KM_KEY_SUB_STREAM_PR, PCI_IDE_KM_KEY_SUB_STREAM_NPR, PCI_IDE_KM_KEY_SUB_STREAM_CPL};

    iv.bytes[0] = PCIE_IDE_IV_INIT_VALUE;

    TEEIO_ASSERT(ks < PCIE_IDE_STREAM_KS_NUM);
    TEEIO_ASSERT(direction < PCIE_IDE_STREAM_DIRECTION_NUM);
    TEEIO_ASSERT(substream < PCIE_IDE_SUB_STREAM_NUM);

    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "pcie_ide_key_prog %s|%s|%s\n", k_set_names[ks], direction_names[direction], substream_names[substream]));

    result = libspdm_get_random_number(sizeof(key_buffer.key), (void *)key_buffer.key);
    if(!result) {
      return false;
    }

    key_buffer.iv[0] = 0;
    key_buffer.iv[1] = PCIE_IDE_IV_INIT_VALUE;

    dump_key_iv(&key_buffer);

    status = pci_ide_km_key_prog(pci_doe_context, spdm_context, session_id,
                                 stream_id,
                                 k_sets[ks] | directions[direction] | substreams[substream],
                                 port_index,
                                 &key_buffer,
                                 &kp_ack_status);
    if(LIBSPDM_STATUS_IS_ERROR(status)) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "pci_ide_km_key_prog failed with status=0x%x\n", status));
      return false;
    }

    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "dev key_prog %s|%s|%s - sts=%02x\n", k_set_names[ks], direction_names[direction], substream_names[substream], kp_ack_status));
    if(kp_ack_status != PCI_IDE_KM_KP_ACK_STATUS_SUCCESS) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "pci_ide_km_key_prog failed with kp_ack_status=0x%x\n", kp_ack_status));
      return false;
    }

    kcbar_ptr = (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)kcbar_addr;

    // program key in root port kcbar registers
    revert_copy_by_dw(key_buffer.key, sizeof(key_buffer.key), keys.bytes, sizeof(keys.bytes));
    slot_id = k_set[ks].slot_id[direction][substream];
    cfg_rootport_ide_keys(kcbar_ptr, rp_stream_index, direction, ks, substream, slot_id, &keys, &iv);
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "rp key_prog %s|%s|%s - @key/iv slot[%02x]\n", k_set_names[ks], direction_names[direction], substream_names[substream], slot_id));
    // dump_key_iv(RP_TYPE, (uint8_t *)keys.bytes, sizeof(keys.bytes), (uint8_t *)iv.bytes, sizeof(iv.bytes));

    return true;
}

libspdm_return_t ide_km_key_set_go(const void *pci_doe_context,
                                   void *spdm_context, const uint32_t *session_id,
                                   uint8_t stream_id, uint8_t key_sub_stream,
                                   uint8_t port_index)
{
    libspdm_return_t status;
    pci_ide_km_k_set_go_t request;
    size_t request_size;
    pci_ide_km_k_gostop_ack_t response;
    size_t response_size;
    bool res;

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
      return status;
    }

    res = response_size == sizeof(pci_ide_km_k_gostop_ack_t);
    if(!res) {
        return LIBSPDM_STATUS_INVALID_MSG_SIZE;
    }

    res = response.header.object_id == PCI_IDE_KM_OBJECT_ID_K_SET_GOSTOP_ACK;
    if(!res)
    {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    res = response.port_index == request.port_index;
    if(!res)
    {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    res = response.stream_id == request.stream_id;
    if(!res)
    {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    res = response.key_sub_stream == request.key_sub_stream;
    if(!res)
    {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    return LIBSPDM_STATUS_SUCCESS;
}

// setup ide stream
bool setup_ide_stream(void* doe_context, void* spdm_context,
                    uint32_t* session_id, uint8_t* kcbar_addr,
                    uint8_t stream_id, uint8_t ks,
                    ide_key_set_t* k_set, uint8_t rp_stream_index,
                    uint8_t port_index, IDE_TEST_TOPOLOGY_TYPE top_type,
                    ide_common_test_port_context_t* upper_port,
                    ide_common_test_port_context_t* lower_port,
                    bool skip_ksetgo)
{
  libspdm_return_t status;
  bool result;

  int upper_port_cfg_space_fd = upper_port->cfg_space_fd;
  uint32_t upper_port_ecap_offset = upper_port->ecap_offset;
  int lower_port_cfg_space_fd = lower_port->cfg_space_fd;
  uint32_t lower_port_ecap_offset = lower_port->ecap_offset;

  uint8_t dev_func_num;
  uint8_t bus_num;
  uint8_t segment;
  uint8_t max_port_index;
  uint32_t ide_reg_block[PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT] = {0};
  uint32_t ide_reg_block_count;

  // first query
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "pcie_ide_km_query.\n"));
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
    return false;
  }

  // ide_km_key_prog
  result = ide_km_key_prog(
      doe_context, spdm_context,
      session_id, ks, PCIE_IDE_STREAM_RX, PCIE_IDE_SUB_STREAM_PR,
      port_index, // port_index
      stream_id,
      kcbar_addr,
      k_set,
      rp_stream_index);
  if(!result) {
    return false;
  }

  result = ide_km_key_prog(
      doe_context, spdm_context,
      session_id, ks, PCIE_IDE_STREAM_RX, PCIE_IDE_SUB_STREAM_NPR,
      port_index,
      stream_id,
      kcbar_addr,
      k_set,
      rp_stream_index);
  if(!result) {
    return false;
  }

  result = ide_km_key_prog(
      doe_context, spdm_context,
      session_id, ks, PCIE_IDE_STREAM_RX, PCIE_IDE_SUB_STREAM_CPL,
      port_index,
      stream_id,
      kcbar_addr,
      k_set,
      rp_stream_index);
  if(!result) {
    return false;
  }

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
  if(!result) {
    return false;
  }

  result = ide_km_key_prog(
      doe_context, spdm_context,
      session_id, ks, PCIE_IDE_STREAM_TX, PCIE_IDE_SUB_STREAM_NPR,
      port_index,
      stream_id,
      kcbar_addr,
      k_set,
      rp_stream_index);
  if(!result) {
    return false;
  }

  result = ide_km_key_prog(
      doe_context, spdm_context,
      session_id, ks, PCIE_IDE_STREAM_TX, PCIE_IDE_SUB_STREAM_CPL,
      port_index,
      stream_id,
      kcbar_addr,
      k_set,
      rp_stream_index);
  if(!result) {
    return false;
  }

  prime_rp_ide_key_set(
      (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)kcbar_addr,
      rp_stream_index,
      PCIE_IDE_STREAM_TX,
      ks);
  
  if(skip_ksetgo) {
    return true;
  }

  // Now KSetGo
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "pcie_ide KSetGo %s|RX|PR\n", k_set_names[ks]));
  status = ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_PR, port_index);
  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "KSetGo %s|RX|PR failed with 0x%x\n", k_set_names[ks], status));
    return false;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "pcie_ide KSetGo %s|RX|NPR\n", k_set_names[ks]));
  status = ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, port_index);

  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "pcie_ide KSetGo %s|RX|NPR failed with 0x%x\n", k_set_names[ks], status));
    return false;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "pcie_ide KSetGo %s|RX|CPL\n", k_set_names[ks]));
  status = ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, port_index);

  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "pcie_ide KSetGo %s|RX|CPL failed with 0x%x\n", k_set_names[ks], status));
    return false;
  }

  // set key_set_select in host ide
  INTEL_KEYP_ROOT_COMPLEX_KCBAR *kcbar = (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)kcbar_addr;
  set_rp_ide_key_set_select(kcbar, rp_stream_index, ks);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "pcie_ide KSetGo %s|TX|PR\n", k_set_names[ks]));
  status = ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_PR, port_index);
  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "pcie_ide KSetGo %s|TX|PR failed with 0x%x\n", k_set_names[ks], status));
    return false;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "pcie_ide KSetGo %s|TX|NPR\n", k_set_names[ks]));
  status = ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, port_index);

  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "pcie_ide KSetGo %s|TX|NPR failed with 0x%x\n", k_set_names[ks], status));
    return false;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "pcie_ide KSetGo %s|TX|CPL\n", k_set_names[ks]));
  status = ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, port_index);

  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "pcie_ide KSetGo %s|TX|CPL failed with 0x%x\n", k_set_names[ks], status));
    return false;
  }

  // enable dev ide
  TEST_IDE_TYPE ide_type = TEST_IDE_TYPE_SEL_IDE;
  if (top_type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE)
  {
    ide_type = IDE_TEST_TOPOLOGY_TYPE_LINK_IDE;
  }
  else if(top_type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE)
  {
    NOT_IMPLEMENTED("selective_and_link_ide topoplogy");
  }
  enable_ide_stream_in_ecap(lower_port_cfg_space_fd, lower_port_ecap_offset, ide_type, lower_port->priv_data.pcie.ide_id, true);

  // enable host ide stream
  enable_rootport_ide_stream(upper_port_cfg_space_fd,
                         upper_port_ecap_offset,
                         ide_type, upper_port->priv_data.pcie.ide_id,
                         kcbar_addr,
                         rp_stream_index, true);

  // wait for 10 ms for device to get ide ready
  libspdm_sleep(10 * 1000);

  // Now ide stream shall be in secure state
  uint32_t data = read_stream_status_in_rp_ecap(upper_port_cfg_space_fd, upper_port_ecap_offset, ide_type, upper_port->priv_data.pcie.ide_id);
  PCIE_SEL_IDE_STREAM_STATUS stream_status = {.raw = data};
  if (stream_status.state != IDE_STREAM_STATUS_SECURE)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "ide_stream state is %x.\n", stream_status.state));
    return false;
  }

  return true;
}

// key switch to @ks
bool ide_key_switch_to(void* doe_context, void* spdm_context,
                    uint32_t* session_id, uint8_t* kcbar_addr,
                    uint8_t stream_id, ide_key_set_t* k_set, uint8_t rp_stream_index,
                    uint8_t port_index, IDE_TEST_TOPOLOGY_TYPE top_type,
                    ide_common_test_port_context_t* upper_port,
                    ide_common_test_port_context_t* lower_port,
                    uint8_t ks, bool skip_ksetgo)
{
    TEST_IDE_TYPE ide_type = TEST_IDE_TYPE_SEL_IDE;
    if(top_type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE) {
        ide_type = TEST_IDE_TYPE_LNK_IDE;
    } else if(top_type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE) {
        NOT_IMPLEMENTED("selective_and_link_ide topoplogy");
    }

    // step1: ensure host_ide and dev_ide is enabled
    if(!is_ide_enabled(upper_port->cfg_space_fd, ide_type, upper_port->priv_data.pcie.ide_id, upper_port->ecap_offset)) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "upper_port ide is not enabled.\n"));
        return false;
    }
    if(!is_ide_enabled(lower_port->cfg_space_fd, ide_type, lower_port->priv_data.pcie.ide_id, lower_port->ecap_offset)) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "lower_port ide is not enabled.\n"));
        return false;
    }

    // step 2: program keys in rp/dev in RX/TX direction

  bool result = ide_km_key_prog(doe_context, spdm_context, session_id,
                                ks, PCIE_IDE_STREAM_RX, PCIE_IDE_SUB_STREAM_PR,
                                port_index, stream_id, kcbar_addr, k_set, rp_stream_index);
  if(!result) {
    return false;
  }

  result = ide_km_key_prog(doe_context, spdm_context, session_id,
                            ks, PCIE_IDE_STREAM_RX, PCIE_IDE_SUB_STREAM_NPR,
                            port_index, stream_id, kcbar_addr, k_set, rp_stream_index);
  if(!result) {
    return false;
  }

  result = ide_km_key_prog(doe_context, spdm_context, session_id,
                            ks, PCIE_IDE_STREAM_RX, PCIE_IDE_SUB_STREAM_CPL,
                            port_index, stream_id, kcbar_addr, k_set, rp_stream_index);
  if(!result) {
    return false;
  }

  prime_rp_ide_key_set((INTEL_KEYP_ROOT_COMPLEX_KCBAR *)kcbar_addr, rp_stream_index, PCIE_IDE_STREAM_RX, ks);

  result = ide_km_key_prog(doe_context, spdm_context,session_id,
                           ks, PCIE_IDE_STREAM_TX, PCIE_IDE_SUB_STREAM_PR,
                           port_index, stream_id, kcbar_addr, k_set, rp_stream_index);
  if(!result) {
    return false;
  }

  result = ide_km_key_prog(doe_context, spdm_context, session_id,
                        ks, PCIE_IDE_STREAM_TX, PCIE_IDE_SUB_STREAM_NPR,
                        port_index, stream_id, kcbar_addr, k_set, rp_stream_index);
  if(!result) {
    return false;
  }

  result = ide_km_key_prog(doe_context, spdm_context, session_id,
                            ks, PCIE_IDE_STREAM_TX, PCIE_IDE_SUB_STREAM_CPL,
                            port_index, stream_id, kcbar_addr, k_set, rp_stream_index);
  if(!result) {
    return false;
  }

  prime_rp_ide_key_set((INTEL_KEYP_ROOT_COMPLEX_KCBAR *)kcbar_addr, rp_stream_index, PCIE_IDE_STREAM_TX, ks);

  if(skip_ksetgo) {
    return true;
  }

  // Now KSetGo
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "KSetGo %s|RX|PR\n", k_set_names[ks]));
  libspdm_return_t status = ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_PR, port_index);
  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "KSetGo %s|RX|PR failed with 0x%x\n", k_set_names[ks], status));
    return false;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "KSetGo %s|RX|NPR\n", k_set_names[ks]));
  status = ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, port_index);

  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "KSetGo %s|RX|NPR failed with 0x%x\n", k_set_names[ks], status));
    return false;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "KSetGo %s|RX|CPL\n", k_set_names[ks]));
  status = ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, port_index);

  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "KSetGo %s|RX|CPL failed with 0x%x\n", k_set_names[ks], status));
    return false;
  }

  // set key_set_select in host ide
  INTEL_KEYP_ROOT_COMPLEX_KCBAR *kcbar = (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)kcbar_addr;
  set_rp_ide_key_set_select(kcbar, rp_stream_index, ks);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "KSetGo %s|TX|PR\n", k_set_names[ks]));
  status = ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_PR, port_index);
  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "KSetGo %s|TX|PR failed with 0x%x\n", k_set_names[ks], status));
    return false;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "KSetGo %s|TX|NPR\n", k_set_names[ks]));
  status = ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, port_index);

  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "KSetGo %s|TX|NPR failed with 0x%x\n", k_set_names[ks], status));
    return false;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "KSetGo %s|TX|CPL\n", k_set_names[ks]));
  status = ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, port_index);

  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "KSetGo %s|TX|CPL failed with 0x%x\n", k_set_names[ks], status));
    return false;
  }

  return true;
}
