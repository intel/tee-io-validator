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
#include "helperlib.h"
#include "teeio_debug.h"
#include "cxl_tsp_internal.h"

void teeio_cxl_tsp_validate_capability(
    libcxltsp_device_capabilities_t *device_capabilities,
    int case_class, int case_id)
{
    uint16_t memory_encryption_features_supported;
    uint32_t memory_encryption_algorithms_supported;
    uint16_t te_state_change_and_access_control_features_supported;
    uint32_t supported_explicit_oob_te_state_granularity;
    uint32_t supported_explicit_ib_te_state_granularity;
    uint16_t configuration_features_supported;

    bool res;

    // Assertion.4
    memory_encryption_features_supported = device_capabilities->memory_encryption_features_supported &
        (CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_ENCRYPTION |
         CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_CKID_BASED_ENCRYPTION |
         CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_RANGE_BASED_ENCRYPTION |
         CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_INITIATOR_SUPPLIED_ENTROPY |
         CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_CKID_BASED_REQUIRED);
    res = (memory_encryption_features_supported & CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_ENCRYPTION) == 1;
    teeio_record_assertion_result(case_class, case_id, 4,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "TspMessage.MemoryEncryptionFeaturesSupported.Encryption = 0x%x",
                                  memory_encryption_features_supported & CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_ENCRYPTION);

    // Assertion.5
    memory_encryption_algorithms_supported = device_capabilities->memory_encryption_algorithms_supported &
                                            (CXL_TSP_MEMORY_ENCRYPTION_ALGORITHMS_AES_XTS_128 |
                                            CXL_TSP_MEMORY_ENCRYPTION_ALGORITHMS_AES_XTS_256);
    res = memory_encryption_algorithms_supported != 0;
    teeio_record_assertion_result(case_class, case_id, 5,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "TspMessage.MemoryEncryptionAlgorithmsSupported.AES-XTS-128 = 0x%x, TspMessage.MemoryEncryptionAlgorithmsSupported.AES-XTS-256 = 0x%x",
                                  memory_encryption_algorithms_supported & CXL_TSP_MEMORY_ENCRYPTION_ALGORITHMS_AES_XTS_128,
                                  memory_encryption_algorithms_supported & CXL_TSP_MEMORY_ENCRYPTION_ALGORITHMS_AES_XTS_256);


    // Assertion.6
    if ((memory_encryption_features_supported & CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_RANGE_BASED_ENCRYPTION) != 0) {
      res = device_capabilities->memory_encryption_number_of_range_based_keys != 0;
    } else {
      res = device_capabilities->memory_encryption_number_of_range_based_keys == 0;
    }
    teeio_record_assertion_result(case_class, case_id, 6,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "TspMessage.MemoryEncryptionFeaturesSupported.RangeBasedEncryption = 0x%x, TspMessage.MemoryEncryptionNumberOfRangeBasedKeys = 0x%x",
                                  memory_encryption_features_supported & CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_RANGE_BASED_ENCRYPTION,
                                  device_capabilities->memory_encryption_number_of_range_based_keys);


    te_state_change_and_access_control_features_supported = device_capabilities->te_state_change_and_access_control_features_supported & 
        (CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_WRITE_ACCESS_CONTROL |
         CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_READ_ACCESS_CONTROL |
         CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_IMPLICIT_TE_STATE_CHANGE |
         CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_OOB_TE_STATE_CHANGE |
         CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_IB_TE_STATE_CHANGE |
         CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_TE_STATE_CHANGE_SANITIZE);

    // Assertion.7
    if((te_state_change_and_access_control_features_supported & 
          CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_WRITE_ACCESS_CONTROL) != 0) {
      res = (te_state_change_and_access_control_features_supported &
           (CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_OOB_TE_STATE_CHANGE |
            CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_IB_TE_STATE_CHANGE)) != 0;
    } else {
      res = true;
    }
    teeio_record_assertion_result(case_class, case_id, 7,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "TspMessage.TEStateChangeAndAccessControlFeaturesSupported.WriteAccessControl = 0x%x, TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitOutOfBandTEStateChange = 0x%x, TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitInBandTEStateChange = 0x%x",
                                  te_state_change_and_access_control_features_supported & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_WRITE_ACCESS_CONTROL,
                                  te_state_change_and_access_control_features_supported & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_OOB_TE_STATE_CHANGE,
                                  te_state_change_and_access_control_features_supported & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_IB_TE_STATE_CHANGE);

    // Assertion.8
    if((te_state_change_and_access_control_features_supported &
          CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_READ_ACCESS_CONTROL) != 0) {
      res = (te_state_change_and_access_control_features_supported &
           (CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_IMPLICIT_TE_STATE_CHANGE |
            CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_OOB_TE_STATE_CHANGE |
            CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_IB_TE_STATE_CHANGE)) != 0;
    } else {
      res = true;
    }
    teeio_record_assertion_result(case_class, case_id, 8,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitOutOfBandTEStateChange = 0x%x, TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitInBandTEStateChange = 0x%x",
                                  te_state_change_and_access_control_features_supported & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_OOB_TE_STATE_CHANGE,
                                  te_state_change_and_access_control_features_supported & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_IB_TE_STATE_CHANGE);


    // Assertion.9
    supported_explicit_ib_te_state_granularity = device_capabilities->supported_explicit_ib_te_state_granularity;
    if(((te_state_change_and_access_control_features_supported &
        CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_IMPLICIT_TE_STATE_CHANGE)) != 0) {
      res = ((te_state_change_and_access_control_features_supported & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_IB_TE_STATE_CHANGE) != 0) &&
            ((supported_explicit_ib_te_state_granularity & CXL_TSP_EXPLICIT_IB_TE_STATE_CHANGE_GRANULARITY_64B) != 0);
    } else {
      res = true;
    }
    teeio_record_assertion_result(case_class, case_id, 9,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ImplicitTEStateChange = 0x%x, TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitInBandTEStateChange = 0x%x, (TspMessage.SupportedExplicitInBandTEStateGranularity & 0x1) = 0x%x",
                                  te_state_change_and_access_control_features_supported & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_IMPLICIT_TE_STATE_CHANGE,
                                  te_state_change_and_access_control_features_supported & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_IB_TE_STATE_CHANGE,
                                  supported_explicit_ib_te_state_granularity & CXL_TSP_EXPLICIT_IB_TE_STATE_CHANGE_GRANULARITY_64B);

    // Assertion.10
    if((te_state_change_and_access_control_features_supported &
          CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_TE_STATE_CHANGE_SANITIZE) != 0) {
      res = (te_state_change_and_access_control_features_supported &
           (CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_OOB_TE_STATE_CHANGE |
            CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_IB_TE_STATE_CHANGE)) != 0;
    } else {
      res = true;
    }
    teeio_record_assertion_result(case_class, case_id, 10,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitTEStateChangeSanitize = 0x%x, TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitOutOfBandTEStateChange = 0x%x, TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitInBandTEStateChange = 0x%x",
                                  te_state_change_and_access_control_features_supported & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_TE_STATE_CHANGE_SANITIZE,
                                  te_state_change_and_access_control_features_supported & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_OOB_TE_STATE_CHANGE,
                                  te_state_change_and_access_control_features_supported & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_IB_TE_STATE_CHANGE);


    // Assertion.11
    supported_explicit_oob_te_state_granularity = device_capabilities->supported_explicit_oob_te_state_granularity;
    if((te_state_change_and_access_control_features_supported & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_OOB_TE_STATE_CHANGE) != 0) {
      res = supported_explicit_oob_te_state_granularity != 0;
    } else {
      res = supported_explicit_oob_te_state_granularity == 0;
    }
    teeio_record_assertion_result(case_class, case_id, 11,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "spMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitOutOfBandTEStateChange = 0x%x, TspMessage.SupportedExplicitOutOfBandTEStateGranularity = 0x%x",
                                  te_state_change_and_access_control_features_supported & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_OOB_TE_STATE_CHANGE,
                                  supported_explicit_oob_te_state_granularity);


    // Assertion.12
    supported_explicit_ib_te_state_granularity = device_capabilities->supported_explicit_ib_te_state_granularity &
        (0x7FF | CXL_TSP_EXPLICIT_IB_TE_STATE_CHANGE_GRANULARITY_ENTIRE_MEMORY);
    if ((te_state_change_and_access_control_features_supported & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_IB_TE_STATE_CHANGE) != 0) {
        res = supported_explicit_ib_te_state_granularity != 0;
    } else {
        res = device_capabilities->supported_explicit_ib_te_state_granularity == 0;
    }
    teeio_record_assertion_result(case_class, case_id, 12,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitInBandTEStateChange = 0x%x, (TspMessage.SupportedExplicitInBandTEStateGranularity & 0x800007FF) = 0x%x, TspMessage.SupportedExplicitInBandTEStateGranularity = 0x%x",
                                  te_state_change_and_access_control_features_supported & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_IB_TE_STATE_CHANGE,
                                  supported_explicit_ib_te_state_granularity,
                                  device_capabilities->supported_explicit_ib_te_state_granularity);


    // Assertion.13
    if ((memory_encryption_features_supported & CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_CKID_BASED_ENCRYPTION) != 0) {
        res =  ((device_capabilities->number_of_ckids >= 2) &&
            (device_capabilities->number_of_ckids < 0x2000));
    } else {
        res = device_capabilities->number_of_ckids;
    }
    teeio_record_assertion_result(case_class, case_id, 13,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "TspMessage.MemoryEncryptionFeaturesSupported.CKIDBasedEncryption = 0x%x, TspMessage.NumberOfCKIDs = 0x%x",
                                  memory_encryption_features_supported & CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_CKID_BASED_ENCRYPTION,
                                  device_capabilities->number_of_ckids);


    // Assertion.14
    configuration_features_supported = device_capabilities->configuration_features_supported &
        (CXL_TSP_CONFIGURATION_FEATURES_SUPPORT_LOCKED_TARGET_FW_UPDATE |
         CXL_TSP_CONFIGURATION_FEATURES_SUPPORT_TARGET_SUPPORT_ADDITIONAL_SPDM_SESSIONS);
    if ((configuration_features_supported & CXL_TSP_CONFIGURATION_FEATURES_SUPPORT_TARGET_SUPPORT_ADDITIONAL_SPDM_SESSIONS) == 0) {
        res = (device_capabilities->number_of_secondary_sessions == 0);
    } else {
        res = (device_capabilities->number_of_secondary_sessions > 0) &&
            (device_capabilities->number_of_secondary_sessions <= 4);
    }
    teeio_record_assertion_result(case_class, case_id, 14,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "TspMessage.ConfigurationFeaturesSupported.TargetSupportsAdditionalSPDMSessions = 0x%x, TspMessage.NumberOfSecondarySessions = 0x%x",
                                  configuration_features_supported & CXL_TSP_CONFIGURATION_FEATURES_SUPPORT_TARGET_SUPPORT_ADDITIONAL_SPDM_SESSIONS,
                                  device_capabilities->number_of_secondary_sessions);

}

/**
 * Test TSP GET_CAPABILITIES
 **/
void test_cxl_tsp_get_capabilities(
    const void *pci_doe_context,
    void *spdm_context, const uint32_t *session_id,
    libcxltsp_device_capabilities_t *device_capabilities,
    int case_class, int case_id)
{
    libspdm_return_t status;
    cxl_tsp_get_target_capabilities_req_t request;
    size_t request_size;
    cxl_tsp_get_target_capabilities_rsp_t response;
    size_t response_size;
    teeio_test_result_t assertion_result = TEEIO_TEST_RESULT_NOT_TESTED;
    bool res;

    libspdm_zero_mem (&request, sizeof(request));
    request.header.tsp_version = CXL_TSP_MESSAGE_VERSION_10;
    request.header.op_code = CXL_TSP_OPCODE_GET_TARGET_CAPABILITIES;

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

    // Assertion.1
    res = response_size == sizeof(cxl_tsp_get_target_capabilities_rsp_t);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 1,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                  "sizeof(TspMessage) = 0x%lx", response_size);
    if(!res) {
        return;
    }

    // Assertion.2
    res = response.header.op_code == CXL_TSP_OPCODE_GET_TARGET_CAPABILITIES_RSP;
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

    device_capabilities->memory_encryption_features_supported = response.memory_encryption_features_supported;
    device_capabilities->memory_encryption_algorithms_supported = response.memory_encryption_algorithms_supported;
    device_capabilities->memory_encryption_number_of_range_based_keys = response.memory_encryption_number_of_range_based_keys;
    device_capabilities->te_state_change_and_access_control_features_supported = response.te_state_change_and_access_control_features_supported;
    device_capabilities->supported_explicit_oob_te_state_granularity = response.supported_explicit_oob_te_state_granularity;
    device_capabilities->supported_explicit_ib_te_state_granularity = response.supported_explicit_ib_te_state_granularity;
    device_capabilities->configuration_features_supported = response.configuration_features_supported;
    device_capabilities->number_of_ckids = response.number_of_ckids;
    device_capabilities->number_of_secondary_sessions = response.number_of_secondary_sessions;

    teeio_cxl_tsp_validate_capability (device_capabilities, case_class, case_id);
}

bool cxl_tsp_test_get_caps_setup(void *test_context)
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
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s: cxl_tsp_get_version failed with  status=0x%llx\n", __func__, status));
    return false;
  }

  return true;
}

void cxl_tsp_test_get_caps_run(void *test_context)
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

  libcxltsp_device_capabilities_t device_capabilities;
  memset(&device_capabilities, 0, sizeof(device_capabilities));
  test_cxl_tsp_get_capabilities(spdm_doe->doe_context, spdm_doe->spdm_context, &spdm_doe->session_id, &device_capabilities, case_class, case_id);
}

void cxl_tsp_test_get_caps_teardown(void *test_context)
{
}
