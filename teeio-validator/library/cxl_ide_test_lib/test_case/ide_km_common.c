/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "assert.h"
#include "hal/base.h"
#include "hal/library/debuglib.h"

#include "hal/library/memlib.h"
#include "library/spdm_transport_pcidoe_lib.h"
#include "library/cxl_ide_km_requester_lib.h"
#include "library/spdm_crypt_lib.h"
#include "ide_test.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "pcie_ide_lib.h"
#include "cxl_ide_lib.h"
#include "cxl_ide_test_common.h"

void cxl_dump_key_iv(cxl_ide_km_aes_256_gcm_key_buffer_t *key_buffer)
{
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Key:\n"));
  dump_hex_array((uint8_t *)key_buffer->key, sizeof(key_buffer->key));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "IV:\n"));
  dump_hex_array((uint8_t *)key_buffer->iv, sizeof(key_buffer->iv));
}

// setup cxl ide stream
bool cxl_stop_ide_stream(void *doe_context, void *spdm_context,
                         uint32_t *session_id, uint8_t *kcbar_addr,
                         uint8_t stream_id,
                         uint8_t port_index,
                         ide_common_test_port_context_t *upper_port,
                         ide_common_test_port_context_t *lower_port)
{
  libspdm_return_t status;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_stop_ide_stream"));

  status = cxl_ide_km_key_set_stop(doe_context, spdm_context,
                                   session_id, stream_id,
                                   CXL_IDE_KM_KEY_DIRECTION_RX | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
                                   port_index);
  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_key_set_stop RX failed with status=%08x\n", status));
    return false;
  }
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "key_set_stop RX\n"));

  status = cxl_ide_km_key_set_stop(doe_context, spdm_context,
                                   session_id, stream_id,
                                   CXL_IDE_KM_KEY_DIRECTION_TX | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
                                   port_index);
  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_key_set_stop TX failed with status=%08x\n", status));
    return false;
  }
  LIBSPDM_DEBUG((LIBSPDM_DEBUG_INFO, "key_set_stop TX\n"));

  return true;
}

// CXL Spec 3.1 Section 8.2.4.22
// CXL IDE Capability Structure
bool cxl_check_device_ide_reg_block(uint32_t* ide_reg_block, uint32_t ide_reg_block_count)
{
  int i;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "ide_reg_block:\n"));
  for (i = 0; i < ide_reg_block_count; i++)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "ide_reg_block %04x: 0x%08x\n", i, ide_reg_block[i]));
  }

  // Section 8.2.4.22.1
  // check CXL IDE Capability at offset 00h
  TEEIO_ASSERT(ide_reg_block_count > 1);
  CXL_IDE_CAPABILITY ide_cap = {.raw = ide_reg_block[0]};
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "CXL IDE Capability : 0x%08x\n", ide_cap.raw));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    cxl_ide_capable=0x%x, cxl_ide_modes=0x%02x\n",
                                  ide_cap.cxl_ide_capable, ide_cap.supported_cxl_ide_modes));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    supported_algo=0x%02x, ide_stop_capable=%d, lopt_ide_capable=%d\n",
                                  ide_cap.supported_algo, ide_cap.ide_stop_capable,
                                  ide_cap.lopt_ide_capable));

  if(ide_cap.cxl_ide_capable == 0) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "CXL IDE is not supported by device.\n"));
    return false;
  }

  return true;
}

// setup cxl ide stream
bool cxl_setup_ide_stream(void *doe_context, void *spdm_context,
                          uint32_t *session_id, uint8_t *kcbar_addr,
                          uint8_t stream_id,
                          uint8_t port_index,
                          ide_common_test_port_context_t *upper_port,
                          ide_common_test_port_context_t *lower_port,
                          bool skip_ksetgo)
{
  bool result;
  uint8_t kp_ack_status;
  libspdm_return_t status;

  uint8_t caps;
  uint8_t bus_num;
  uint8_t segment;
  uint8_t dev_func_num;
  uint8_t max_port_index;
  uint32_t ide_reg_block[CXL_IDE_KM_IDE_CAP_REG_BLOCK_MAX_COUNT] = {0};
  uint32_t ide_reg_block_count;
  INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr;

  cxl_ide_km_aes_256_gcm_key_buffer_t rx_key_buffer;
  cxl_ide_km_aes_256_gcm_key_buffer_t tx_key_buffer;

  INTEL_KEYP_KEY_SLOT keys = {0};

  // query
  caps = 0;
  ide_reg_block_count = CXL_IDE_KM_IDE_CAP_REG_BLOCK_MAX_COUNT;
  status = cxl_ide_km_query(doe_context,
                            spdm_context,
                            session_id,
                            port_index,
                            &dev_func_num,
                            &bus_num,
                            &segment,
                            &max_port_index,
                            &caps,
                            ide_reg_block,
                            &ide_reg_block_count);
  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_query failed with status=0x%x\n", status));
    return false;
  }
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "max_port_index - 0x%02x\n", max_port_index));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "caps - 0x%02x\n", caps));

  if(!cxl_check_device_ide_reg_block(ide_reg_block, ide_reg_block_count)) {
    return false;
  }

  // check CXL IDE Capability at offset 00h
  TEEIO_ASSERT(ide_reg_block_count > 1);
  CXL_IDE_CAPABILITY ide_cap = {.raw = ide_reg_block[0]};
  lower_port->cxl_data.memcache.ide_cap.raw = ide_cap.raw;

  kcbar_ptr = (INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)kcbar_addr;
  // // At current stage, we only set SKID mode
  // if(ide_cap.supported_cxl_ide_modes & CXL_IDE_MODE_SKID_MASK) {
  //   TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "SKID mode is not supported in device side. ide_cap.raw=0x%08x\n", ide_cap.raw));
  //   TEEIO_ASSERT(false);
  // }
  cxl_cfg_rp_mode(kcbar_ptr, INTEL_CXL_IDE_MODE_SKID);

  // get_key
  // status = cxl_ide_km_get_key(doe_context, spdm_context,
  //                             &session_id, stream_id,
  //                             CXL_IDE_KM_KEY_SUB_STREAM_CXL, port_index,
  //                             &key_buffer);
  // if (LIBSPDM_STATUS_IS_ERROR(status)) {
  //   TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_get_key failed with status=0x%08x\n", status));
  //   return false;
  // }
  // TEEIO_DEBUG((TEEIO_DEBUG_INFO, "get_key\n"));

  // ide_km_key_prog in RX
  result = libspdm_get_random_number(sizeof(rx_key_buffer.key), (void *)rx_key_buffer.key);
  if (!result)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "libspdm_get_random_number for rx_key_buffer failed.\n"));
    return false;
  }
  memset(rx_key_buffer.key, 0x11, sizeof(rx_key_buffer.key));
  rx_key_buffer.iv[0] = 0;
  rx_key_buffer.iv[1] = 1;
  rx_key_buffer.iv[2] = 2;
  cxl_dump_key_iv(&rx_key_buffer);

  status = cxl_ide_km_key_prog(
      doe_context, spdm_context,
      session_id, stream_id,
      CXL_IDE_KM_KEY_DIRECTION_RX | CXL_IDE_KM_KEY_IV_INITIAL | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
      port_index, // port_index
      &rx_key_buffer, &kp_ack_status);

  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_key_prog RX failed with status=0x%08x\n", status));
    TEEIO_ASSERT(false);
    return false;
  }
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "key_prog RX - %02x\n", kp_ack_status));

  // ide_km_key_prog in TX
  result = libspdm_get_random_number(sizeof(tx_key_buffer.key), (void *)tx_key_buffer.key);
  if (!result)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "libspdm_get_random_number for tx_key_buffer failed.\n"));
    return false;
  }
  memset(tx_key_buffer.key, 0x22, sizeof(tx_key_buffer.key));
  tx_key_buffer.iv[0] = 0;
  tx_key_buffer.iv[1] = 1;
  tx_key_buffer.iv[2] = 2;
  cxl_dump_key_iv(&tx_key_buffer);

  status = cxl_ide_km_key_prog(
      doe_context, spdm_context,
      session_id, stream_id,
      CXL_IDE_KM_KEY_DIRECTION_TX | CXL_IDE_KM_KEY_IV_INITIAL | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
      port_index,
      &tx_key_buffer, &kp_ack_status);
  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_key_prog TX failed with status=0x%08x\n", status));
    TEEIO_ASSERT(false);
    return false;
  }
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "key_prog TX - %02x\n", kp_ack_status));

  // TODO
  // Program TX/RX pending keys into Link_Enc_Key_Tx and Link_Enc_Key_Rx registers
  // Program TX/RX IV values
  revert_copy_by_dw(tx_key_buffer.key, sizeof(tx_key_buffer.key), keys.bytes, sizeof(keys.bytes));
  cxl_cfg_rp_link_enc_key_iv(kcbar_ptr, CXL_IDE_KM_KEY_DIRECTION_TX, 0, keys.bytes, sizeof(keys.bytes), (uint8_t *)tx_key_buffer.iv, sizeof(tx_key_buffer.iv));

  revert_copy_by_dw(rx_key_buffer.key, sizeof(rx_key_buffer.key), keys.bytes, sizeof(keys.bytes));
  cxl_cfg_rp_link_enc_key_iv(kcbar_ptr, CXL_IDE_KM_KEY_DIRECTION_RX, 0, keys.bytes, sizeof(keys.bytes), (uint8_t *)rx_key_buffer.iv, sizeof(rx_key_buffer.iv));

  // Set TxKeyValid and RxKeyValid bit
  cxl_cfg_rp_txrx_key_valid(kcbar_ptr, CXL_IDE_STREAM_DIRECTION_TX, true);
  cxl_cfg_rp_txrx_key_valid(kcbar_ptr, CXL_IDE_STREAM_DIRECTION_RX, true);

  // KSetGo in RX
  status = cxl_ide_km_key_set_go(doe_context, spdm_context,
                                 session_id, stream_id,
                                 CXL_IDE_KM_KEY_DIRECTION_RX | CXL_IDE_KM_KEY_MODE_CONTAINMENT | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
                                 port_index);
  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_key_set_go RX failed with status=0x%08x\n", status));
    TEEIO_ASSERT(false);
    return false;
  }
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "key_set_go RX\n"));

  // Set LinkEncEnable bit
  cxl_cfg_rp_linkenc_enable(kcbar_ptr, true);

  // Set StartTrigger bit
  cxl_cfg_rp_start_trigger(kcbar_ptr, true);

  // KSetGo in TX
  status = cxl_ide_km_key_set_go(doe_context, spdm_context,
                                 session_id, stream_id,
                                 CXL_IDE_KM_KEY_DIRECTION_TX | CXL_IDE_KM_KEY_MODE_CONTAINMENT | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
                                 port_index);
  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_key_set_go TX failed with status=0x%08x\n", status));
    TEEIO_ASSERT(false);
    return false;
  }
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "key_set_go TX\n"));

  // wait for 10 ms for device to get ide ready
  libspdm_sleep(10 * 1000);

  printf("cxl_setup_ide_stream is done. Press any key to continue.\n");
  getchar();
  return true;
}