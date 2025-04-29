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
#include "helperlib.h"
#include "cxl_tsp_internal.h"

static libcxltsp_device_capabilities_t m_device_capabilities = {0};

static void test_cxl_tsp_set_configuration(
  const void *pci_doe_context,
  void *spdm_context, const uint32_t *session_id,
  const libcxltsp_device_configuration_t *device_configuration,
  const libcxltsp_device_2nd_session_info_t *device_2nd_session_info,
  int case_class, int case_id)
{
    libspdm_return_t status;
    cxl_tsp_set_target_configuration_req_t request;
    size_t request_size;
    uint8_t res_buf[LIBCXLTSP_ERROR_MESSAGE_MAX_SIZE];
    cxl_tsp_set_target_configuration_rsp_t *response;
    size_t response_size;
    teeio_test_result_t assertion_result = TEEIO_TEST_RESULT_NOT_TESTED;
    bool res;

    libspdm_zero_mem (&request, sizeof(request));
    request.header.tsp_version = CXL_TSP_MESSAGE_VERSION_10;
    request.header.op_code = CXL_TSP_OPCODE_SET_TARGET_CONFIGURATION;

    request.memory_encryption_features_enable = device_configuration->memory_encryption_features_enable;
    request.memory_encryption_algorithm_select = device_configuration->memory_encryption_algorithm_select;
    request.te_state_change_and_access_control_features_enable = device_configuration->te_state_change_and_access_control_features_enable;
    request.explicit_oob_te_state_granularity = device_configuration->explicit_oob_te_state_granularity;
    request.configuration_features_enable = device_configuration->configuration_features_enable;
    request.ckid_base = device_configuration->ckid_base;
    request.number_of_ckids = device_configuration->number_of_ckids;
    libspdm_copy_mem (
        request.explicit_ib_te_state_granularity_entry,
        sizeof(request.explicit_ib_te_state_granularity_entry),
        device_configuration->explicit_ib_te_state_granularity_entry,
        sizeof(device_configuration->explicit_ib_te_state_granularity_entry));

    request.configuration_validity_flags = device_2nd_session_info->configuration_validity_flags;
    request.secondary_session_ckid_type = device_2nd_session_info->secondary_session_ckid_type;
    libspdm_copy_mem (
        request.secondary_session_psk_key_material,
        sizeof(request.secondary_session_psk_key_material),
        device_2nd_session_info->secondary_session_psk_key_material,
        sizeof(device_2nd_session_info->secondary_session_psk_key_material));

    request_size = sizeof(request);
    response = (void *)res_buf;
    response_size = sizeof(res_buf);
    status = cxl_tsp_send_receive_data(spdm_context, session_id,
                                       &request, request_size,
                                       response, &response_size);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_tsp_send_receive_data failed with status 0x%x\n", status));
        teeio_record_assertion_result(case_class, case_id, 0,
                                      IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED,
                                      "cxl_tsp_send_receive_data failed with 0x%x", status);
        return;
    }

    // Assertion.1
    res = response_size == sizeof(cxl_tsp_set_target_configuration_rsp_t);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 1,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                  "sizeof(TspMessage) = 0x%lx", response_size);

    if (!res) {
        return;
    }

    // Assertion.2
    res = response->header.op_code == CXL_TSP_OPCODE_SET_TARGET_CONFIGURATION_RSP;
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 2,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                  "TspMessage.Opcode = 0x%x", response->header.op_code);

    // Assertion.3
    res = response->header.tsp_version == request.header.tsp_version;
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 3,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                  "TspMessage.TSPVersion = 0x%x", response->header.tsp_version);
}

bool cxl_tsp_test_set_configuration_setup(void *test_context)
{
  libspdm_return_t status;

  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  cxl_tsp_test_group_context_t *group_context = (cxl_tsp_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  teeio_common_test_group_context_t *common = &group_context->common;
  TEEIO_ASSERT(common->signature == GROUP_CONTEXT_SIGNATURE);
  spdm_doe_context_t *spdm_doe = &group_context->spdm_doe;
  TEEIO_ASSERT(spdm_doe);

  status = cxl_tsp_get_version(spdm_doe->doe_context, spdm_doe->spdm_context, &spdm_doe->session_id);
  if(LIBSPDM_STATUS_IS_ERROR(status)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s: cxl_tsp_get_version failed with  status=0x%x\n", __func__, status));
    return false;
  }

  libspdm_zero_mem(&m_device_capabilities, sizeof(m_device_capabilities));
  status = cxl_tsp_get_capabilities(spdm_doe->doe_context, spdm_doe->spdm_context, &spdm_doe->session_id, &m_device_capabilities);
  if(LIBSPDM_STATUS_IS_ERROR(status)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s: cxl_tsp_get_capabilities failed with  status=0x%x\n", __func__, status));
    return false;
  }

  return true;
}

void cxl_tsp_test_set_configuration_run(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_run_test_case_t* test_case = case_context->test_case;
  TEEIO_ASSERT(test_case);
  int case_class = test_case->class_id;
  int case_id = test_case->case_id;

  cxl_tsp_test_group_context_t *group_context = (cxl_tsp_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  teeio_common_test_group_context_t *common = &group_context->common;
  TEEIO_ASSERT(common->signature == GROUP_CONTEXT_SIGNATURE);
  spdm_doe_context_t *spdm_doe = &group_context->spdm_doe;
  TEEIO_ASSERT(spdm_doe);

  libcxltsp_device_configuration_t device_configuration;
  libcxltsp_device_2nd_session_info_t device_2nd_session_info;

  libspdm_zero_mem (&device_configuration, sizeof(device_configuration));
  device_configuration.memory_encryption_features_enable =
    CXL_TSP_MEMORY_ENCRYPTION_FEATURES_ENABLE_ENCRYPTION;
  device_configuration.memory_encryption_algorithm_select =
    CXL_TSP_MEMORY_ENCRYPTION_ALGORITHMS_AES_XTS_256;
  device_configuration.te_state_change_and_access_control_features_enable =
    CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_READ_ACCESS_CONTROL |
    CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_IMPLICIT_TE_STATE_CHANGE |
    CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_IB_TE_STATE_CHANGE;
  device_configuration.explicit_oob_te_state_granularity = 0;
  device_configuration.configuration_features_enable =
    CXL_TSP_CONFIGURATION_FEATURES_ENABLE_LOCKED_TARGET_FW_UPDATE;
  device_configuration.ckid_base = 0;
  device_configuration.number_of_ckids = 0;
  device_configuration.explicit_ib_te_state_granularity_entry[0].te_state_granularity = 0;
  device_configuration.explicit_ib_te_state_granularity_entry[0].length_index = 0;
  device_configuration.explicit_ib_te_state_granularity_entry[1].length_index = 0xFF;
  device_configuration.explicit_ib_te_state_granularity_entry[2].length_index = 0xFF;
  device_configuration.explicit_ib_te_state_granularity_entry[3].length_index = 0xFF;
  device_configuration.explicit_ib_te_state_granularity_entry[4].length_index = 0xFF;
  device_configuration.explicit_ib_te_state_granularity_entry[5].length_index = 0xFF;
  device_configuration.explicit_ib_te_state_granularity_entry[6].length_index = 0xFF;
  device_configuration.explicit_ib_te_state_granularity_entry[7].length_index = 0xFF;
  libspdm_zero_mem (&device_2nd_session_info, sizeof(device_2nd_session_info));
  if ((m_device_capabilities.configuration_features_supported &
    CXL_TSP_CONFIGURATION_FEATURES_SUPPORT_TARGET_SUPPORT_ADDITIONAL_SPDM_SESSIONS) != 0) {
    switch (m_device_capabilities.number_of_secondary_sessions) {
    case 1:
      device_2nd_session_info.configuration_validity_flags = 0x1;
      break;
    case 2:
      device_2nd_session_info.configuration_validity_flags = 0x3;
      break;
    case 3:
      device_2nd_session_info.configuration_validity_flags = 0x7;
      break;
    case 4:
      device_2nd_session_info.configuration_validity_flags = 0xf;
      break;
    default:
      teeio_record_assertion_result(case_class, case_id, 0,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED,
                                  "device_capabilites.num_of_secondary_sessions is error - 0x%x", m_device_capabilities.number_of_secondary_sessions);

      return;
    }

    for (int index = 0; index < CXL_TSP_2ND_SESSION_COUNT; index++) {
      if ((device_2nd_session_info.configuration_validity_flags & (0x1 << index)) != 0) {
        bool result = libspdm_get_random_number(
          sizeof(device_2nd_session_info.secondary_session_psk_key_material[index]),
          (uint8_t *)&device_2nd_session_info.secondary_session_psk_key_material[index]);
        if (!result) {
          teeio_record_assertion_result(case_class, case_id, 0,
                                      IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED,
                                      "libspdm_get_random_number is error");
          return;
        }
      }
    }
  }
  test_cxl_tsp_set_configuration(spdm_doe->doe_context, spdm_doe->spdm_context, &spdm_doe->session_id,
                                      &device_configuration, &device_2nd_session_info, case_class, case_id);
}

void cxl_tsp_test_set_configuration_teardown(void *test_context)
{
}
