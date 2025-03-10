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

extern bool g_teeio_fixed_key;

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

static bool gen_default_iv(uint32_t* iv, int iv_size)
{
  if(iv_size != 12) {
    TEEIO_ASSERT(false);
    return false;
  }

  iv[0] = CXL_IDE_KM_KEY_SUB_STREAM_CXL<<24;
  iv[1] = 0;
  iv[2] = 1;

  return true;
}

/**
 * Generate CXL-IDE Key/IV.
 */
bool cxl_ide_generate_key(const void *pci_doe_context,
                          void *spdm_context, const uint32_t *session_id,
                          uint8_t stream_id, uint8_t key_sub_stream, uint8_t port_index,
                          cxl_ide_km_aes_256_gcm_key_buffer_t *key_buffer,
                          bool key_gen_capable, bool iv_gen_capable, uint8_t direction,
                          uint8_t *cxl_ide_km_iv
                          )
{
  bool result = true;
  libspdm_return_t status;

  if(direction != CXL_IDE_KM_KEY_DIRECTION_RX && direction != CXL_IDE_KM_KEY_DIRECTION_TX) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid CXL Direction (%d)\n", direction));
    return false;
  }

  gen_default_iv(key_buffer->iv, sizeof(key_buffer->iv));

  if(g_teeio_fixed_key) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Generate fixed key in rootport side.\n"));
    memset(key_buffer->key, direction == CXL_IDE_KM_KEY_DIRECTION_RX ? TEEIO_TEST_FIXED_RX_KEY_BYTE_VALUE : TEEIO_TEST_FIXED_TX_KEY_BYTE_VALUE, sizeof(key_buffer->key));
    *cxl_ide_km_iv = CXL_IDE_KM_KEY_IV_DEFAULT;
  } else {
    // According to CXL Spec 3.1 Section 11.4.5, if iv_generation_capable or key_generation_capable is supported,
    // key/iv for Device TX direction is recommended to be generated by sending CXL_GETKEY to device (which will then
    // generate the key/iv in device side).
    if((key_gen_capable || iv_gen_capable) && direction == CXL_IDE_KM_KEY_DIRECTION_TX) {
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Generate key/iv in device with cxl_ide_km_get_key.\n"));
      memset(key_buffer->iv, 0, sizeof(key_buffer->iv));
      status = cxl_ide_km_get_key(pci_doe_context, spdm_context, session_id,
                                  stream_id, key_sub_stream, port_index,
                                  key_buffer);

      if (LIBSPDM_STATUS_IS_ERROR(status)) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_get_key failed with status=0x%08x.\n", status));
        result = false;
        goto GenKeyIvDone;
      }

      // now let's check key_gen_capable and iv_gen_capable
      if(key_gen_capable) {
        // Keep the keys in response of CXL_GETKEY
      } else {
        // Generate the keys by host
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "KEY_GEN_CAPABLE is not set. So key shall be generated by host.\n"));
        result = libspdm_get_random_number(sizeof(key_buffer->key), (void *)key_buffer->key);
        if (!result) {
          TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "libspdm_get_random_number failed.\n"));
          goto GenKeyIvDone;
        }
      }

      if(iv_gen_capable) {
        // Keep the IV in response of CXL_GETKEY and set cxl_ide_km_iv as CXL_IDE_KM_KEY_IV_INITIAL
        *cxl_ide_km_iv = CXL_IDE_KM_KEY_IV_INITIAL;

        // Check the first 4 bytes of IV returned in IDEKM GET_KEY
        // In current stage, Intel Rootport requires it shall be [80 00 00 00]
        if(key_buffer->iv[0] != CXL_IDE_KM_KEY_SUB_STREAM_CXL << 24) {
          TEEIO_DEBUG((TEEIO_DEBUG_INFO, "The first 4 bytes of IV returned from IDEKM GET_KEY is random value.\n"));
          TEEIO_DEBUG((TEEIO_DEBUG_INFO, "In current stage Intel Rootport requires it shall be [80 00 00 00].\n"));
          TEEIO_DEBUG((TEEIO_DEBUG_INFO, "It will fall back to use DEFAULT_IV.\n"));
          *cxl_ide_km_iv = CXL_IDE_KM_KEY_IV_DEFAULT;
          result = gen_default_iv(key_buffer->iv, sizeof(key_buffer->iv));
        }
      } else {
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "IV_GEN_CAPABLE is not set. So IV shall be generated by host.\n"));
        *cxl_ide_km_iv = CXL_IDE_KM_KEY_IV_DEFAULT;
        result = gen_default_iv(key_buffer->iv, sizeof(key_buffer->iv));
      }
    } else {
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Generate dynamic key/iv in rootport side.\n"));
      result = libspdm_get_random_number(sizeof(key_buffer->key), (void *)key_buffer->key);
      if (!result) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "libspdm_get_random_number failed.\n"));
      }
      *cxl_ide_km_iv = CXL_IDE_KM_KEY_IV_DEFAULT;
    }
  }

GenKeyIvDone:
  return result;
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

// setup cxl ide stream
bool cxl_setup_ide_stream(void *doe_context, void *spdm_context,
                          uint32_t *session_id, uint8_t *kcbar_addr,
                          uint8_t stream_id,
                          uint8_t port_index,
                          ide_common_test_port_context_t *upper_port,
                          ide_common_test_port_context_t *lower_port,
                          bool skip_ksetgo, uint32_t config_bitmap,
                          CXL_IDE_MODE ide_mode, bool key_refresh)
{
  bool result;
  uint8_t kp_ack_status;
  libspdm_return_t status;
  INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr;
  cxl_ide_km_aes_256_gcm_key_buffer_t rx_key_buffer = {0};
  cxl_ide_km_aes_256_gcm_key_buffer_t tx_key_buffer = {0};
  INTEL_KEYP_KEY_SLOT keys = {0};
  uint32_t tx_iv[2] = {0};
  uint32_t rx_iv[2] = {0};
  CXL_QUERY_RESP_CAPS dev_caps = {0};

  kcbar_ptr = (INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)kcbar_addr;
  dev_caps.raw = lower_port->cxl_data.query_resp.caps;
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "dev_caps: ide_key_generation_capable=%d, iv_generation_capable=%d\n",
                                dev_caps.ide_key_generation_capable, dev_caps.iv_generation_capable));
  bool key_gen_capable = dev_caps.ide_key_generation_capable == 1;
  bool iv_gen_capable = dev_caps.iv_generation_capable == 1;

  bool get_key = config_bitmap & CXL_BIT_MASK(CXL_IDE_CONFIGURATION_TYPE_GET_KEY);

  if(g_teeio_fixed_key) {
    // Tester intends to set up CXL IDE stream with fixed key.
    // So ignore key_gen_capable and iv_gen_capable.
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Test with fixed IDE Key.\n"));
    key_gen_capable = false;
    iv_gen_capable = false;
    get_key = false;
  }

  if(key_gen_capable || iv_gen_capable) {
    // GET_KEY is supported by device.
    // Check if cxl_get_key is set in [Configuratioin] setion.
    if(!get_key) {
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Disable CXL GET_KEY though it is supported.\n"));
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "To enable CXL GET_KEY set \"cxl_get_key\" in [Configuration] section.\n"));
      key_gen_capable = false;
      iv_gen_capable = false;
    }
  } else {
    // GET_KEY is not supported by device.
    // If cxl_get_key is set in [Configuraition] section, it return false.
    if(get_key) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "CXL GET_KEY is not supported.\n"));
      return false;
    }
  }

  uint8_t cxl_ide_km_iv_rx = 0;
  uint8_t cxl_ide_km_iv_tx = 0;

  // generate cxl-ide key/iv for RX direction
  result = cxl_ide_generate_key(doe_context, spdm_context,
                               session_id, stream_id,
                               CXL_IDE_KM_KEY_SUB_STREAM_CXL, port_index,
                               &rx_key_buffer, key_gen_capable, iv_gen_capable,
                               CXL_IDE_KM_KEY_DIRECTION_RX, &cxl_ide_km_iv_rx
                               );
  if (!result) {
    return false;
  }
  // generate cxl-ide key/iv for TX direction
  result = cxl_ide_generate_key(doe_context, spdm_context,
                               session_id, stream_id,
                               CXL_IDE_KM_KEY_SUB_STREAM_CXL, port_index,
                               &tx_key_buffer, key_gen_capable, iv_gen_capable,
                               CXL_IDE_KM_KEY_DIRECTION_TX, &cxl_ide_km_iv_tx
                               );
  if (!result) {
    return false;
  }

  // ide_km_key_prog in RX
  status = cxl_ide_km_key_prog(
      doe_context, spdm_context,
      session_id, stream_id,
      CXL_IDE_KM_KEY_DIRECTION_RX | cxl_ide_km_iv_rx | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
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
  status = cxl_ide_km_key_prog(
      doe_context, spdm_context,
      session_id, stream_id,
      CXL_IDE_KM_KEY_DIRECTION_TX | cxl_ide_km_iv_tx | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
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
  cxl_construct_rp_iv(tx_key_buffer.iv, sizeof(tx_key_buffer.iv), tx_iv, sizeof(tx_iv));
  cxl_cfg_rp_link_enc_key_iv(kcbar_ptr, CXL_IDE_KM_KEY_DIRECTION_TX, 0, keys.bytes, sizeof(keys.bytes), (uint8_t *)tx_iv, sizeof(tx_iv));
  cxl_dump_key_iv_in_rp("Rx", keys.bytes, 32, (uint8_t *)tx_iv, 8);

  cxl_construct_rp_keys(rx_key_buffer.key, sizeof(rx_key_buffer.key), keys.bytes, sizeof(keys.bytes));
  cxl_construct_rp_iv(rx_key_buffer.iv, sizeof(rx_key_buffer.iv), rx_iv, sizeof(rx_iv));
  cxl_cfg_rp_link_enc_key_iv(kcbar_ptr, CXL_IDE_KM_KEY_DIRECTION_RX, 0, keys.bytes, sizeof(keys.bytes), (uint8_t *)rx_iv, sizeof(rx_iv));
  cxl_dump_key_iv_in_rp("Tx", keys.bytes, 32, (uint8_t *)rx_iv, 8);

  // Set TxKeyValid and RxKeyValid bit
  cxl_cfg_rp_txrx_key_valid(kcbar_ptr, CXL_IDE_STREAM_DIRECTION_TX, true);
  cxl_cfg_rp_txrx_key_valid(kcbar_ptr, CXL_IDE_STREAM_DIRECTION_RX, true);

  uint8_t ide_km_mode = CXL_IDE_KM_KEY_MODE_SKID;
  if(ide_mode == CXL_IDE_MODE_CONTAINMENT) {
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

  // Skip set link_enc_enable if key_refresh is true
  if(!key_refresh) {
    cxl_cfg_rp_linkenc_enable(kcbar_ptr, true);
  }

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

  return true;
}