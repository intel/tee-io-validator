/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

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

void cxl_dump_key_iv_in_rp(const char* direction, uint8_t *key, int key_size, uint8_t *iv, int iv_size)
{
  int i = 0;
  TEEIO_ASSERT(key_size == 32);
  TEEIO_ASSERT(iv_size == 8);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "CXL IDE Key in rootport registers:\n"));
  for(i = 0; i < 4; i++) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, " %s Encryption key %d: 0x%016"PRIx64"\n", direction, i, *(uint64_t *)(key + i*8)));
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "CXL IDE IV in rootport registers:\n"));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  %s IF: 0x%016"PRIx64"\n", direction, *(uint64_t *)iv));
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

static void dump_cxl_ide_status(ide_common_test_port_context_t* upper_port, ide_common_test_port_context_t* lower_port)
{
  TEEIO_PRINT(("\n"));
  TEEIO_PRINT(("Print host cxl ide status\n"));
  cxl_dump_ide_status(upper_port->cxl_data.memcache.cap_headers, upper_port->cxl_data.memcache.cap_headers_cnt, upper_port->cxl_data.memcache.mapped_memcache_reg_block);

  TEEIO_PRINT(("\n"));
  TEEIO_PRINT(("Print device cxl ide status.\n"));
  cxl_dump_ide_status(lower_port->cxl_data.memcache.cap_headers, lower_port->cxl_data.memcache.cap_headers_cnt, lower_port->cxl_data.memcache.mapped_memcache_reg_block);
}

// setup cxl ide stream
bool cxl_setup_ide_stream(void *doe_context, void *spdm_context,
                          uint32_t *session_id, uint8_t *kcbar_addr,
                          uint8_t stream_id,
                          uint8_t port_index,
                          ide_common_test_port_context_t *upper_port,
                          ide_common_test_port_context_t *lower_port,
                          bool skip_ksetgo, uint32_t config_bitmap)
{
  bool result;
  uint8_t kp_ack_status;
  libspdm_return_t status;
  INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr;
  cxl_ide_km_aes_256_gcm_key_buffer_t rx_key_buffer = {0};
  cxl_ide_km_aes_256_gcm_key_buffer_t tx_key_buffer = {0};
  INTEL_KEYP_KEY_SLOT keys = {0};

  kcbar_ptr = (INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)kcbar_addr;

  // ide_km_key_prog in RX
  result = libspdm_get_random_number(sizeof(rx_key_buffer.key), (void *)rx_key_buffer.key);
  if (!result)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "libspdm_get_random_number for rx_key_buffer failed.\n"));
    return false;
  }

  status = cxl_ide_km_key_prog(
      doe_context, spdm_context,
      session_id, stream_id,
      CXL_IDE_KM_KEY_DIRECTION_RX | CXL_IDE_KM_KEY_IV_DEFAULT | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
      port_index, // port_index
      &rx_key_buffer, &kp_ack_status);

  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_key_prog RX failed with status=0x%08x\n", status));
    TEEIO_ASSERT(false);
    return false;
  }
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "key_prog RX - %02x\n", kp_ack_status));
  dump_key_iv_in_key_prog(rx_key_buffer.key, sizeof(rx_key_buffer.key)/sizeof(uint32_t), rx_key_buffer.iv, sizeof(rx_key_buffer.iv)/sizeof(uint32_t));

  // ide_km_key_prog in TX
  result = libspdm_get_random_number(sizeof(tx_key_buffer.key), (void *)tx_key_buffer.key);
  if (!result)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "libspdm_get_random_number for tx_key_buffer failed.\n"));
    return false;
  }

  status = cxl_ide_km_key_prog(
      doe_context, spdm_context,
      session_id, stream_id,
      CXL_IDE_KM_KEY_DIRECTION_TX | CXL_IDE_KM_KEY_IV_DEFAULT | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
      port_index,
      &tx_key_buffer, &kp_ack_status);
  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_key_prog TX failed with status=0x%08x\n", status));
    TEEIO_ASSERT(false);
    return false;
  }
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "key_prog TX - %02x\n", kp_ack_status));
  dump_key_iv_in_key_prog(tx_key_buffer.key, sizeof(tx_key_buffer.key)/sizeof(uint32_t), tx_key_buffer.iv, sizeof(tx_key_buffer.iv)/sizeof(uint32_t));

  // Program TX/RX pending keys into Link_Enc_Key_Tx and Link_Enc_Key_Rx registers
  // Program TX/RX IV values
  cxl_construct_rp_keys(tx_key_buffer.key, sizeof(tx_key_buffer.key), keys.bytes, sizeof(keys.bytes));
  tx_key_buffer.iv[0] = 1;
  cxl_cfg_rp_link_enc_key_iv(kcbar_ptr, CXL_IDE_KM_KEY_DIRECTION_TX, 0, keys.bytes, sizeof(keys.bytes), (uint8_t *)tx_key_buffer.iv, sizeof(tx_key_buffer.iv));
  cxl_dump_key_iv_in_rp("Rx", keys.bytes, 32, (uint8_t *)tx_key_buffer.iv, 8);

  cxl_construct_rp_keys(rx_key_buffer.key, sizeof(rx_key_buffer.key), keys.bytes, sizeof(keys.bytes));
  rx_key_buffer.iv[0] = 1;
  cxl_cfg_rp_link_enc_key_iv(kcbar_ptr, CXL_IDE_KM_KEY_DIRECTION_RX, 0, keys.bytes, sizeof(keys.bytes), (uint8_t *)rx_key_buffer.iv, sizeof(rx_key_buffer.iv));
  cxl_dump_key_iv_in_rp("Tx", keys.bytes, 32, (uint8_t *)rx_key_buffer.iv, 8);

  // Set TxKeyValid and RxKeyValid bit
  cxl_cfg_rp_txrx_key_valid(kcbar_ptr, CXL_IDE_STREAM_DIRECTION_TX, true);
  cxl_cfg_rp_txrx_key_valid(kcbar_ptr, CXL_IDE_STREAM_DIRECTION_RX, true);

  uint8_t ide_km_mode = CXL_IDE_KM_KEY_MODE_SKID;
  if(config_bitmap & CXL_BIT_MASK(CXL_IDE_CONFIGURATION_TYPE_CONTAINMENT_MODE)) {
    ide_km_mode = CXL_IDE_KM_KEY_MODE_CONTAINMENT;
  }

  // KSetGo in RX
  status = cxl_ide_km_key_set_go(doe_context, spdm_context,
                                 session_id, stream_id,
                                 CXL_IDE_KM_KEY_DIRECTION_RX | ide_km_mode | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
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
                                 CXL_IDE_KM_KEY_DIRECTION_TX | ide_km_mode | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
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

  dump_cxl_ide_status(upper_port, lower_port);

  printf("cxl_setup_ide_stream is done. Press any key to continue.\n");
  getchar();
  return true;
}