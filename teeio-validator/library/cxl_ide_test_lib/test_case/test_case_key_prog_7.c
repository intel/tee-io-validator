/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "ide_test.h"
#include "teeio_debug.h"

#include "library/spdm_requester_lib.h"
#include "library/spdm_crypt_lib.h"
#include "hal/library/memlib.h"
#include "helperlib.h"
#include "cxl_ide_lib.h"
#include "cxl_ide_test_common.h"
#include "cxl_ide_test_internal.h"

static CXL_QUERY_RESP_CAPS* m_dev_caps = NULL;

/**
 * Prepare the CXL IDE Keys with random values generated in host side
 */
bool cxl_ide_prepare_keys_with_get_key (
  const void *doe_context, void *spdm_context,
  const uint32_t *session_id, int stream_id,
  uint8_t sub_stream, uint8_t port_index, 
  cxl_ide_km_aes_256_gcm_key_buffer_t* key_buffer, uint8_t* cxl_ide_km_iv)
{
  libspdm_return_t status;
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Generate key/iv in device with cxl_ide_km_get_key.\n"));
  memset(key_buffer->iv, 0, sizeof(key_buffer->iv));

  status = cxl_ide_km_get_key(doe_context, spdm_context,
                              session_id, stream_id,
                              CXL_IDE_KM_KEY_SUB_STREAM_CXL, port_index,
                              key_buffer);

  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_get_key failed with status=0x%08x.\n", status));
    return false;
  }

  // Check the first 4 bytes of IV returned in IDEKM GET_KEY
  // In current stage, Intel Rootport requires it shall be [80 00 00 00]
  if(key_buffer->iv[0] != CXL_IDE_KM_KEY_SUB_STREAM_CXL << 24) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Returned IV from GET_KEY is NOT [80 00 00 00] which is required by Intel RootPort in current stage.\n"));
    return false;
  }

  *cxl_ide_km_iv = CXL_IDE_KM_KEY_IV_INITIAL;

  return true;
}

static void test_cxl_ide_key_prog_7 (
  const void *doe_context,  void *spdm_context,
  const uint32_t *session_id, uint8_t port_index,
  int case_class, int case_id)
{
  bool result = false;
  libspdm_return_t status;
  uint8_t kp_ack_status;
  uint8_t cxl_ide_km_iv_rx = 0;
  uint8_t cxl_ide_km_iv_tx = 0;
  cxl_ide_km_aes_256_gcm_key_buffer_t rx_key_buffer = {0};
  cxl_ide_km_aes_256_gcm_key_buffer_t tx_key_buffer = {0};
  char case_info[MAX_LINE_LENGTH] = {0};

  CXL_QUERY_RESP_CAPS caps = {.raw = m_dev_caps[port_index].raw};

  if(!cxl_ide_prepare_dynamic_keys(&rx_key_buffer, &cxl_ide_km_iv_rx)) {
    sprintf(case_info, "%s", "cxl_ide_prepare_dynamic_keys for RX failed");
    goto PrepareDone;
  }

  if(!cxl_ide_prepare_keys_with_get_key(doe_context, spdm_context,
                                        session_id, 0,
                                        CXL_IDE_KM_KEY_SUB_STREAM_CXL, port_index,
                                        &tx_key_buffer, &cxl_ide_km_iv_tx)) {
    sprintf(case_info, "%s", "cxl_ide_prepare_keys_with_get_key for TX failed");
    goto PrepareDone;
  }

  // If the device is not capable of generating IV, then use the DEFAULT_IV (which is used by rx_key_buffer)
  if(caps.iv_generation_capable == 0) {
    cxl_ide_km_iv_tx = cxl_ide_km_iv_rx;
    memcpy(&tx_key_buffer.iv, &rx_key_buffer.iv, sizeof(rx_key_buffer.iv));
  }

  // ide_km_key_prog in RX.
  // This is expected to succeed because we are using dynamic keys intead of GET_KEY.
  // This case requires to reverse the keys which is generated by GET_KEY.
  status = cxl_ide_km_key_prog(
      doe_context, spdm_context,
      session_id, 0,
      CXL_IDE_KM_KEY_DIRECTION_RX | cxl_ide_km_iv_rx | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
      port_index, // port_index
      &rx_key_buffer, &kp_ack_status);

  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    sprintf(case_info, "cxl_ide_km_key_prog RX failed with status=0x%08x\n", status);
    goto PrepareDone;
  }

  result = true;

PrepareDone:
  if(!result) {
    teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_SEPARATOR, TEEIO_TEST_RESULT_NOT_TESTED, "  port_index = 0x%02x", port_index);
    teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED, "%s", case_info);
    return;
  }

  // Now do the actual test
  teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_SEPARATOR, TEEIO_TEST_RESULT_NOT_TESTED, "  port_index = 0x%02x", port_index);

  // reverse the tx_key_buffer according to test steps
  for(int i = 0; i < sizeof(tx_key_buffer.key)/sizeof(uint32_t); i++) {
    tx_key_buffer.key[i] = ~tx_key_buffer.key[i];
  }

  // ide_km_key_prog in TX
  test_cxl_ide_key_prog_invalid_params(
      doe_context, spdm_context,
      session_id, 0,
      CXL_IDE_KM_KEY_DIRECTION_TX | cxl_ide_km_iv_tx | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
      port_index,
      &tx_key_buffer, &kp_ack_status,
      case_class, case_id);
}

/**
 * get the dev_caps of the device
 */
bool cxl_ide_get_dev_caps(const void *doe_context,
                        void *spdm_context, const uint32_t *session_id,
                        int max_port_index, CXL_QUERY_RESP_CAPS* dev_caps)
{
  libspdm_return_t status;
  CXL_QUERY_RESP_CAPS caps;
  uint8_t bus_num;
  uint8_t segment;
  uint8_t dev_func_num;
  uint8_t max_port;
  uint32_t ide_reg_block[CXL_IDE_KM_IDE_CAP_REG_BLOCK_MAX_COUNT] = {0};
  uint32_t ide_reg_block_count;

  for(int port_index = 0; port_index <= max_port_index; port_index++) {
    caps.raw = 0;
    ide_reg_block_count = CXL_IDE_KM_IDE_CAP_REG_BLOCK_MAX_COUNT;

    status = cxl_ide_km_query(doe_context,
                              spdm_context,
                              session_id,
                              port_index,
                              &dev_func_num,
                              &bus_num,
                              &segment,
                              &max_port,
                              &caps.raw,
                              ide_reg_block,
                              &ide_reg_block_count);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_query failed with status=0x%x\n", status));
      return false;
    }
    dev_caps[port_index].raw = caps.raw;
  }

  return true;
}

bool cxl_ide_test_key_prog_7_setup(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  cxl_ide_test_group_context_t *group_context = (cxl_ide_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  int max_port_index = group_context->common.lower_port.cxl_data.query_resp.max_port_index;
  m_dev_caps = (CXL_QUERY_RESP_CAPS*)malloc(sizeof(CXL_QUERY_RESP_CAPS) * (max_port_index + 1));
  if(!cxl_ide_get_dev_caps(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context,
                          &group_context->spdm_doe.session_id, max_port_index, m_dev_caps)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_get_dev_caps failed.\n"));
    return false;
  }

  bool ide_key_generation_capable = false;
  for(int i = 0; i <= max_port_index; i++) {
    CXL_QUERY_RESP_CAPS* dev_cap = m_dev_caps + i;
    if(dev_cap->ide_key_generation_capable == 0) {
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "CXL IDE Key Generation (for port_index=%d) is not supported.\n", i));
    } else {
      ide_key_generation_capable = true;
    }
  }

  if(ide_key_generation_capable == false) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "CXL IDE Key Generation is not supported in all the ports of the device. Skip the case.\n"));
    case_context->action = IDE_COMMON_TEST_ACTION_SKIP;
  }

  return true;
}

bool cxl_ide_test_key_prog_7_run(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_run_test_case_t* test_case = case_context->test_case;
  TEEIO_ASSERT(test_case);
  int case_class = test_case->class_id;
  int case_id = test_case->case_id;

  cxl_ide_test_group_context_t *group_context = (cxl_ide_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  IDE_TEST_CONFIGURATION *configuration = get_configuration_by_id(group_context->common.suite_context->test_config, group_context->common.config_id);
  TEEIO_ASSERT(configuration);

  ide_common_test_port_context_t *lower_port = &group_context->common.lower_port;

  for(int i = 0; i <=lower_port->cxl_data.query_resp.max_port_index; i++) {
    if(m_dev_caps[i].ide_key_generation_capable == 1) {
      test_cxl_ide_key_prog_7(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context,
                            &group_context->spdm_doe.session_id, i,
                            case_class, case_id);
    }
  }

  return true;
}

bool cxl_ide_test_key_prog_7_teardown(void *test_context)
{
  if(m_dev_caps) {
    free(m_dev_caps);
    m_dev_caps = NULL;
  }

  return true;
}
