/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ide_test.h"
#include "teeio_debug.h"
#include "helperlib.h"
#include "cxl_tsp_internal.h"

static libcxltsp_device_capabilities_t m_device_capabilities = {0};

static void test_cxl_tsp_get_configuration(
  const void *pci_doe_context, void *spdm_context, const uint32_t *session_id,
  libcxltsp_device_configuration_t *device_configuration,
  int case_class, int case_id)
{
    libspdm_return_t status;
    cxl_tsp_get_target_configuration_req_t request;
    size_t request_size;
    cxl_tsp_get_target_configuration_rsp_t response;
    size_t response_size;
    bool res;
    teeio_test_result_t assertion_result = TEEIO_TEST_RESULT_NOT_TESTED;

    TEEIO_ASSERT(device_configuration);

    libspdm_zero_mem (&request, sizeof(request));
    request.header.tsp_version = CXL_TSP_MESSAGE_VERSION_10;
    request.header.op_code = CXL_TSP_OPCODE_GET_TARGET_CONFIGURATION;

    request_size = sizeof(request);
    response_size = sizeof(response);
    status = cxl_tsp_send_receive_data(spdm_context, session_id,
                                       &request, request_size,
                                       &response, &response_size);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_tsp_send_receive_data failed with status 0x%x\n", status));
        teeio_record_assertion_result(case_class, case_id, 0,
                                      IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED,
                                      "cxl_tsp_send_receive_data failed with 0x%x", status);
        return;
    }

    res = response_size == sizeof(cxl_tsp_get_target_capabilities_rsp_t);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 1,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                  "sizeof(TspMessage) = 0x%lx", response_size);
    if(!res) {
        return;
    }

    // Assertion.2
    res = response.header.op_code == CXL_TSP_OPCODE_GET_TARGET_CONFIGURATION;
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 2,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                  "TspMessage.Opcode = 0x%x", response.header.op_code);

    // Assertion.3
    res = response.header.tsp_version == request.header.tsp_version;
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 3,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                  "TspMessage.TSPVersion = 0x%x", response.header.tsp_version);

    device_configuration->memory_encryption_features_enable = response.memory_encryption_features_enabled;
    device_configuration->memory_encryption_algorithm_select = response.memory_encryption_algorithm_selected;
    device_configuration->te_state_change_and_access_control_features_enable = response.te_state_change_and_access_control_features_enabled;
    device_configuration->explicit_oob_te_state_granularity = response.explicit_oob_te_state_granularity_enabled;
    device_configuration->configuration_features_enable = response.configuration_features_enabled;
    device_configuration->ckid_base = response.ckid_base;
    device_configuration->number_of_ckids = response.number_of_ckids;
    libspdm_copy_mem (
        device_configuration->explicit_ib_te_state_granularity_entry,
        sizeof(device_configuration->explicit_ib_te_state_granularity_entry),
        response.explicit_ib_te_state_granularity_entry,
        sizeof(response.explicit_ib_te_state_granularity_entry));

    // Assertion.4
    res = (device_configuration->memory_encryption_features_enable & m_device_capabilities.memory_encryption_features_supported) == device_configuration->memory_encryption_features_enable;
    teeio_record_assertion_result(case_class, case_id, 4,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "TspMessage.MemoryEncryptionFeaturesEnabled & GET_TARGET_TSP_CAPABILITIES_RSP.MemoryEncryptionFeaturesSupported = 0x%x, TspMessage.MemoryEncryptionFeaturesEnabled = 0x%x",
                                  device_configuration->memory_encryption_features_enable & m_device_capabilities.memory_encryption_features_supported,
                                  device_configuration->memory_encryption_features_enable);

    // Assertion.5
    uint16_t data16 = ((device_configuration->memory_encryption_features_enable & CXL_TSP_MEMORY_ENCRYPTION_FEATURES_ENABLE_CKID_BASED_ENCRYPTION) ^
      (device_configuration->memory_encryption_features_enable & CXL_TSP_MEMORY_ENCRYPTION_FEATURES_ENABLE_RANGE_BASED_ENCRYPTION));
    res = data16 != 0;
    teeio_record_assertion_result(case_class, case_id, 5,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "(TspMessage.MemoryEncryptionFeaturesEnabled.CKIDBasedEncryption ^ TspMessage.MemoryEncryptionFeaturesEnabled.RangeBasedEncryption) = 0x%x",
                                  data16);

    // Assertion.6
    res = (device_configuration->memory_encryption_algorithm_select & m_device_capabilities.memory_encryption_algorithms_supported) == device_configuration->memory_encryption_algorithm_select;
    teeio_record_assertion_result(case_class, case_id, 6,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "TspMessage.MemoryEncryptionAlgorithmsSelected & GET_TARGET_TSP_CAPABILITIES_RSP.MemoryEncryptionAlgorithmsSupported = 0x%x, TspMessage.MemoryEncryptionAlgorithmsSelected = 0x%x",
                                  device_configuration->memory_encryption_algorithm_select & m_device_capabilities.memory_encryption_algorithms_supported,
                                  device_configuration->memory_encryption_algorithm_select);

    // Assertion.7
    data16 = (device_configuration->memory_encryption_algorithm_select & CXL_TSP_MEMORY_ENCRYPTION_ALGORITHMS_AES_XTS_128) ^
      (device_configuration->memory_encryption_algorithm_select & CXL_TSP_MEMORY_ENCRYPTION_ALGORITHMS_AES_XTS_256);
    res = data16 != 0;
    teeio_record_assertion_result(case_class, case_id, 7,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "(TspMessage.MemoryEncryptionAlgorithmsSelected.AES-XTS-128 ^ TspMessage.MemoryEncryptionAlgorithmsSelected.AES-XTS-256) = 0x%x",
                                  data16);

    // Assertion.8
    data16 = device_configuration->te_state_change_and_access_control_features_enable & m_device_capabilities.te_state_change_and_access_control_features_supported;
    res = data16 == device_configuration->te_state_change_and_access_control_features_enable;
    teeio_record_assertion_result(case_class, case_id, 8,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "(TspMessage.TEStateChangeAndAccessControlFeaturesEnabled & GET_TARGET_TSP_CAPABILITIES_RSP.TEStateChangeAndAccessControlFeaturesSupported) = 0x%x, TspMessage.TEStateChangeAndAccessControlFeaturesEnabled = 0x%x",
                                  data16, device_configuration->te_state_change_and_access_control_features_enable);

    // Assertion.9
    data16 = device_configuration->explicit_oob_te_state_granularity & m_device_capabilities.supported_explicit_oob_te_state_granularity;
    res = data16 == device_configuration->explicit_oob_te_state_granularity;
    teeio_record_assertion_result(case_class, case_id, 9,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "(TspMessage.ExplicitOutOfBandTEStateGranularityEnabled & GET_TARGET_TSP_CAPABILITIES_RSP.SupportedExplicitOutOfBandTEStateGranularity) = 0x%x, TspMessage.ExplicitOutOfBandTEStateGranularityEnabled = 0x%x",
                                  data16, device_configuration->explicit_oob_te_state_granularity);

    // Assertion.10
    data16 = device_configuration->configuration_features_enable & m_device_capabilities.configuration_features_supported & 0x1;
    res = data16 == device_configuration->configuration_features_enable;
    teeio_record_assertion_result(case_class, case_id, 9,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "(TspMessage.ConfigurationFeaturesEnabled & GET_TARGET_TSP_CAPABILITIES_RSP.ConfigurationFeaturesSupported & 0x1) = 0x%x, TspMessage.ConfigurationFeaturesEnabled = 0x%x",
                                  data16, device_configuration->configuration_features_enable);

    // Assertion.11
    if ((device_configuration->memory_encryption_features_enable & CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_CKID_BASED_ENCRYPTION) != 0) {
        res =  ((device_configuration->ckid_base < 0x2000) &&
            (device_configuration->ckid_base + device_configuration->number_of_ckids < 0x2000));
    } else {
        res = true;
    }
    teeio_record_assertion_result(case_class, case_id, 11,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "TspMessage.MemoryEncryptionFeaturesEnabled.CKIDBaseRequired = 0x%x, TspMessage.CKIDBase = 0x%x, TspMessage.NumberOfCKIDs = 0x%x",
                                  device_configuration->memory_encryption_features_enable & CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_CKID_BASED_ENCRYPTION,
                                  device_configuration->ckid_base,
                                  device_configuration->number_of_ckids);

    // Assertion.12
    res = response.current_tsp_state <= CXL_TSP_STATE_ERROR;
    teeio_record_assertion_result(case_class, case_id, 12,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "TspMessage.CurrentTSPState = 0x%x",
                                  response.current_tsp_state);

    // Assertion.13
    cxl_tsp_explicit_ib_te_state_granularity_entry_t
        explicit_ib_te_state_granularity_entry[8];
    size_t index;
    uint8_t length_index_bit;
    char buffer[MAX_LINE_LENGTH] = {0};
    int offset = 0;

    libspdm_copy_mem(
        explicit_ib_te_state_granularity_entry,
        sizeof(explicit_ib_te_state_granularity_entry),
        device_configuration->explicit_ib_te_state_granularity_entry,
        sizeof(device_configuration->explicit_ib_te_state_granularity_entry)
        );
    if ((device_configuration->te_state_change_and_access_control_features_enable & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_IB_TE_STATE_CHANGE) != 0) {
        length_index_bit = 0;
        sprintf(buffer, "TspMessage.ExplicitInBandTEStateGranularityEntry[0~7]: ");
        offset = strlen(buffer);

        for (index = 0; index < 8; index++) {
            sprintf(buffer + offset, "%02x ", explicit_ib_te_state_granularity_entry[index].length_index);
            offset = strlen(buffer);
            if (explicit_ib_te_state_granularity_entry[index].length_index == 0xff) {
                continue;
            }
            if (explicit_ib_te_state_granularity_entry[index].length_index > 7) {
                res = false;
            }
            if ((length_index_bit & (1 << explicit_ib_te_state_granularity_entry[index].length_index)) != 0) {
                res = false;
            }
            length_index_bit |= (1 << explicit_ib_te_state_granularity_entry[index].length_index);
        }
    } else {
      res = false;
      sprintf(buffer, "TspMessage.ExplicitInBandTEStateChange = 0x%x", device_configuration->te_state_change_and_access_control_features_enable & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_IB_TE_STATE_CHANGE);
    }
    teeio_record_assertion_result(case_class, case_id, 13,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "%s",
                                  buffer);
}


bool cxl_tsp_test_get_configuration_setup(void *test_context)
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

void cxl_tsp_test_get_configuration_run(void *test_context)
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

  libcxltsp_device_configuration_t current_device_configuration = {0};

  test_cxl_tsp_get_configuration(spdm_doe->doe_context, spdm_doe->spdm_context,
                                &spdm_doe->session_id, &current_device_configuration,
                                case_class, case_id);
}

void cxl_tsp_test_get_configuration_teardown(void *test_context)
{
}
