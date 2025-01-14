/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "ide_test.h"
#include "teeio_debug.h"

#include "library/spdm_requester_lib.h"
#include "library/spdm_crypt_lib.h"
#include "hal/library/memlib.h"
#include "helperlib.h"
#include "cxl_ide_lib.h"
#include "cxl_ide_test_common.h"
#include "cxl_ide_test_internal.h"

/**
 * Prepare the CXL IDE Keys with random values generated in host side
 */
bool cxl_ide_prepare_dynamic_keys(cxl_ide_km_aes_256_gcm_key_buffer_t* key_buffer, uint8_t* cxl_ide_km_iv)
{
  key_buffer->iv[0] = CXL_IDE_KM_KEY_SUB_STREAM_CXL<<24;
  key_buffer->iv[1] = 0;
  key_buffer->iv[2] = 1;
  *cxl_ide_km_iv = CXL_IDE_KM_KEY_IV_DEFAULT;
  bool result = libspdm_get_random_number(sizeof(key_buffer->key), (void *)key_buffer->key);
  if (!result) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "libspdm_get_random_number failed for key_buffer.\n"));
  }

  return result;
}

static void test_cxl_ide_key_prog_6 (
  const void *doe_context,  void *spdm_context,
  const uint32_t *session_id, uint8_t port_index,
  int case_class, int case_id)
{
  bool result = false;
  uint8_t kp_ack_status;
  uint8_t cxl_ide_km_iv_rx = 0;
  uint8_t cxl_ide_km_iv_tx = 0;
  cxl_ide_km_aes_256_gcm_key_buffer_t rx_key_buffer = {0};
  cxl_ide_km_aes_256_gcm_key_buffer_t tx_key_buffer = {0};
  char case_info[MAX_LINE_LENGTH] = {0};

  if(!cxl_ide_prepare_dynamic_keys(&rx_key_buffer, &cxl_ide_km_iv_rx)) {
    sprintf(case_info, "%s", "cxl_ide_prepare_dynamic_keys for RX failed");
    goto PrepareDone;
  }

  if(!cxl_ide_prepare_dynamic_keys(&tx_key_buffer, &cxl_ide_km_iv_tx)) {
    sprintf(case_info, "%s", "cxl_ide_prepare_dynamic_keys for TX failed");
    goto PrepareDone;
  }

  // ide_km_key_prog in RX
  libspdm_return_t status = cxl_ide_km_key_prog(
      doe_context, spdm_context,
      session_id, 0,
      CXL_IDE_KM_KEY_DIRECTION_RX | cxl_ide_km_iv_rx | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
      port_index, // port_index
      &rx_key_buffer, &kp_ack_status);

  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    sprintf(case_info, "cxl_ide_km_key_prog RX failed with status=0x%x\n", status);
    goto PrepareDone;
  }

  // ide_km_key_prog in TX
  status = cxl_ide_km_key_prog(
      doe_context, spdm_context,
      session_id, 0,
      CXL_IDE_KM_KEY_DIRECTION_TX | cxl_ide_km_iv_tx | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
      port_index,
      &tx_key_buffer, &kp_ack_status);
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    sprintf(case_info, "cxl_ide_km_key_prog TX failed with status=0x%x\n", status);
    goto PrepareDone;
  }

  result = true;

PrepareDone:
  if(!result) {
    teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_SEPARATOR, TEEIO_TEST_RESULT_NOT_TESTED, "  port_index = 0x%02x", port_index);
    teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED, "%s", case_info);
    return;
  }

  // Now do the actual test (key_prog) with the same context above
  teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_SEPARATOR, TEEIO_TEST_RESULT_NOT_TESTED, "  port_index = 0x%02x RX", port_index);
  test_cxl_ide_key_prog_invalid_params(doe_context, spdm_context, session_id, 0,
                                       CXL_IDE_KM_KEY_DIRECTION_RX | cxl_ide_km_iv_rx | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
                                       port_index,
                                       &rx_key_buffer, &kp_ack_status,
                                       case_class, case_id);

  teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_SEPARATOR, TEEIO_TEST_RESULT_NOT_TESTED, "  port_index = 0x%02x TX", port_index);
  test_cxl_ide_key_prog_invalid_params(doe_context, spdm_context, session_id, 0,
                                       CXL_IDE_KM_KEY_DIRECTION_TX | cxl_ide_km_iv_tx | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
                                       port_index,
                                       &rx_key_buffer, &kp_ack_status,
                                       case_class, case_id);
}

bool cxl_ide_test_key_prog_6_setup(void *test_context)
{
  // Cxl.Query has been called in test_group.setup()
  return true;
}

bool cxl_ide_test_key_prog_6_run(void *test_context)
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
    test_cxl_ide_key_prog_6(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context,
                          &group_context->spdm_doe.session_id, i, case_class, case_id);
  }

  return true;
}

bool cxl_ide_test_key_prog_6_teardown(void *test_context)
{
  return true;
}
