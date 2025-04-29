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

static void test_cxl_ide_key_prog_9 (
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

  if(caps.ide_key_generation_capable == 1) {
    if(!cxl_ide_prepare_keys_with_get_key(doe_context, spdm_context,
                                          session_id, 0,
                                          CXL_IDE_KM_KEY_SUB_STREAM_CXL, port_index,
                                          &tx_key_buffer, &cxl_ide_km_iv_tx)) {
      sprintf(case_info, "%s", "cxl_ide_prepare_keys_with_get_key for TX failed");
      goto PrepareDone;
    }
  } else {
    if(!cxl_ide_prepare_dynamic_keys(&tx_key_buffer, &cxl_ide_km_iv_tx)) {
      sprintf(case_info, "%s", "cxl_ide_prepare_dynamic_keys for TX failed");
      goto PrepareDone;
    }
  }

  // ide_km_key_prog in RX.
  // This is expected to succeed because we are using dynamic keys intead of GET_KEY.
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
  // Set the TX IV as Intial IV based on test step
  cxl_ide_km_iv_tx = CXL_IDE_KM_KEY_IV_INITIAL;

  teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_SEPARATOR, TEEIO_TEST_RESULT_NOT_TESTED, "  port_index= 0x%02x", port_index);

  // ide_km_key_prog in TX
  test_cxl_ide_key_prog_invalid_params(
      doe_context, spdm_context,
      session_id, 0,
      CXL_IDE_KM_KEY_DIRECTION_TX | cxl_ide_km_iv_tx | CXL_IDE_KM_KEY_SUB_STREAM_CXL,
      port_index,
      &tx_key_buffer, &kp_ack_status,
      case_class, case_id);
}

bool cxl_ide_test_key_prog_9_setup(void *test_context)
{
  // Cxl.Query has been called in test_group.setup()
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

  bool iv_generation_capable = true;
  for(int i = 0; i <= max_port_index; i++) {
    CXL_QUERY_RESP_CAPS* dev_cap = m_dev_caps + i;
    if(dev_cap->iv_generation_capable == 0) {
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "CXL IDE IV Generation (for port_index=%d) is not supported.\n", i));
      iv_generation_capable = false;
    }
  }

  if(iv_generation_capable == true) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "CXL IDE IV Generation is supported in all the ports of the device. Skip the case.\n"));
    case_context->action = IDE_COMMON_TEST_ACTION_SKIP;
  }

  return true;
}

void cxl_ide_test_key_prog_9_run(void *test_context)
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
    if(m_dev_caps[i].iv_generation_capable == 0) {
      test_cxl_ide_key_prog_9(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context,
                            &group_context->spdm_doe.session_id, i,
                            case_class, case_id);
    }
  }
}

void cxl_ide_test_key_prog_9_teardown(void *test_context)
{
  if(m_dev_caps) {
    free(m_dev_caps);
    m_dev_caps = NULL;
  }
}
