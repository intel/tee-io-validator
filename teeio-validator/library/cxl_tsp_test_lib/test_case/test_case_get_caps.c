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
#include "cxl_tsp_internal.h"

// doc/tsp_test/TspTestCase/2.GetTargetTspCapabilitiesResponse.md
static const char* mCxlTspGetCapsAssersion[] = {
  "cxl_tsp_get_caps send_receive data",                               // .0
  "sizeof(TspMessage) == sizeof(GET_TARGET_TSP_CAPABILITIES_RSP)",    // .1
  "TspMessage.Opcode == GET_TARGET_TSP_CAPABILITIES_RSP",             // .2
  "TspMessage.TSPVersion == 0x10",                                    // .3
  "TspMessage.MemoryEncryptionFeaturesSupported.Encryption == 1",     // .4
  "(TspMessage.MemoryEncryptionAlgorithmsSupported.AES-XTS-128 == 1) || (TspMessage.MemoryEncryptionAlgorithmsSupported.AES-XTS-256 == 1)",
  "if (TspMessage.MemoryEncryptionFeaturesSupported.RangeBasedEncryption == 1), then (TspMessage.MemoryEncryptionNumberOfRangeBasedKeys != 0), else (TspMessage.MemoryEncryptionNumberOfRangeBasedKeys == 0)",
  "if (TspMessage.TEStateChangeAndAccessControlFeaturesSupported.WriteAccessControl == 1), then !((TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitOutOfBandTEStateChange == 0) && (TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitInBandTEStateChange == 0))",
  "if (TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ReadAccessControl == 1), then !((TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ImplicitTEStateChange == 0) && (TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitOutOfBandTEStateChange == 0) && (TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitInBandTEStateChange == 0))",
  "if (TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ImplicitTEStateChange == 1), then (TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitInBandTEStateChange == 1) && ((TspMessage.SupportedExplicitInBandTEStateGranularity & 0x1) != 0)",
  "if (TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitTEStateChangeSanitize == 1), then !((TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitOutOfBandTEStateChange == 0) && (TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitInBandTEStateChange == 0))",
  "if (TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitOutOfBandTEStateChange == 1), then (TspMessage.SupportedExplicitOutOfBandTEStateGranularity != 0), else (TspMessage.SupportedExplicitOutOfBandTEStateGranularity == 0)",
  "if (TspMessage.TEStateChangeAndAccessControlFeaturesSupported.ExplicitInBandTEStateChange == 1), then ((TspMessage.SupportedExplicitInBandTEStateGranularity & 0x800007FF) != 0), else (TspMessage.SupportedExplicitInBandTEStateGranularity == 0)",
  "if (TspMessage.MemoryEncryptionFeaturesSupported.CKIDBasedEncryption == 1), then (TspMessage.NumberOfCKIDs >= 2) && (TspMessage.NumberOfCKIDs < 2^13), else (TspMessage.NumberOfCKIDs == 0)",
  "if (TspMessage.ConfigurationFeaturesSupported.TargetSupportsAdditionalSPDMSessions == 1), then (TspMessage.NumberOfSecondarySessions > 0) && (TspMessage.NumberOfSecondarySessions <= 4), else (TspMessage.NumberOfSecondarySessions == 0)};"
};

libspdm_return_t
teeio_cxl_tsp_validate_capability (
    libcxltsp_device_capabilities_t *device_capabilities
    )
{
    uint16_t memory_encryption_features_supported;
    uint32_t memory_encryption_algorithms_supported;
    uint16_t te_state_change_and_access_control_features_supported;
    uint32_t supported_explicit_oob_te_state_granularity;
    uint32_t supported_explicit_ib_te_state_granularity;
    uint16_t configuration_features_supported;

    const char* case_msg = "  Assertion 2.1";
    bool res;

    // Assertion.4
    memory_encryption_features_supported = device_capabilities->memory_encryption_features_supported &
        (CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_ENCRYPTION |
         CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_CKID_BASED_ENCRYPTION |
         CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_RANGE_BASED_ENCRYPTION |
         CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_INITIATOR_SUPPLIED_ENTROPY |
         CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_CKID_BASED_REQUIRED);
    res = (memory_encryption_features_supported & CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_ENCRYPTION) != 0;
    TEEIO_PRINT(("         %s.4: %s %s\n", case_msg, mCxlTspGetCapsAssersion[4], res ? "Pass" : "Fail"));
    if (!res) {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    // Assertion.5
    memory_encryption_algorithms_supported = device_capabilities->memory_encryption_algorithms_supported &
        (CXL_TSP_MEMORY_ENCRYPTION_ALGORITHMS_AES_XTS_128 |
         CXL_TSP_MEMORY_ENCRYPTION_ALGORITHMS_AES_XTS_256);
    res = memory_encryption_algorithms_supported != 0;
    TEEIO_PRINT(("         %s.5: %s %s\n", case_msg, mCxlTspGetCapsAssersion[5], res ? "Pass" : "Fail"));
    if (!res) {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    // Assertion.6
    if ((memory_encryption_features_supported & CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_RANGE_BASED_ENCRYPTION) == 0) {
        res = device_capabilities->memory_encryption_number_of_range_based_keys == 0;
        TEEIO_PRINT(("         %s.6: %s %s\n", case_msg, mCxlTspGetCapsAssersion[6], res ? "Pass" : "Fail"));
        if (!res) {
            return LIBSPDM_STATUS_INVALID_MSG_FIELD;
        }
    } else {
        res = device_capabilities->memory_encryption_number_of_range_based_keys != 0;
        TEEIO_PRINT(("         %s.6: %s %s\n", case_msg, mCxlTspGetCapsAssersion[6], res ? "Pass" : "Fail"));
        if (!res) {
            return LIBSPDM_STATUS_INVALID_MSG_FIELD;
        }
    }

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
      TEEIO_PRINT(("         %s.7: %s %s\n", case_msg, mCxlTspGetCapsAssersion[7], res ? "Pass" : "Fail"));
      if (!res) {
          return LIBSPDM_STATUS_INVALID_MSG_FIELD;
      }
    }

    // Assertion.8
    if((te_state_change_and_access_control_features_supported &
          CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_READ_ACCESS_CONTROL) != 0) {
      res = (te_state_change_and_access_control_features_supported &
           (CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_IMPLICIT_TE_STATE_CHANGE |
            CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_OOB_TE_STATE_CHANGE |
            CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_IB_TE_STATE_CHANGE)) != 0;

      TEEIO_PRINT(("         %s.8: %s %s\n", case_msg, mCxlTspGetCapsAssersion[8], res ? "Pass" : "Fail"));
      if (!res) {
          return LIBSPDM_STATUS_INVALID_MSG_FIELD;
      }
    }

    // Assertion.9
    // NOT FOUND

    // Assertion.10
    if((te_state_change_and_access_control_features_supported &
          CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_TE_STATE_CHANGE_SANITIZE) != 0) {
      res = (te_state_change_and_access_control_features_supported &
           (CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_OOB_TE_STATE_CHANGE |
            CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_IB_TE_STATE_CHANGE)) != 0;
      TEEIO_PRINT(("         %s.10: %s %s\n", case_msg, mCxlTspGetCapsAssersion[10], res ? "Pass" : "Fail"));
      if (!res) {
          return LIBSPDM_STATUS_INVALID_MSG_FIELD;
      }
    }

    // Assertion.11
    supported_explicit_oob_te_state_granularity = device_capabilities->supported_explicit_oob_te_state_granularity;
    if((te_state_change_and_access_control_features_supported & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_OOB_TE_STATE_CHANGE) != 0) {
      res = supported_explicit_oob_te_state_granularity != 0;
    } else {
      res = supported_explicit_oob_te_state_granularity == 0;
    }
    TEEIO_PRINT(("         %s.11: %s %s\n", case_msg, mCxlTspGetCapsAssersion[11], res ? "Pass" : "Fail"));
    if (!res) {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    // Assertion.12
    supported_explicit_ib_te_state_granularity = device_capabilities->supported_explicit_ib_te_state_granularity &
        (0x7FF | CXL_TSP_EXPLICIT_IB_TE_STATE_CHANGE_GRANULARITY_ENTIRE_MEMORY);
    if ((te_state_change_and_access_control_features_supported & CXL_TSP_TE_STATE_CHANGE_AND_ACCESS_CONTROL_FEATURES_EXPLICIT_IB_TE_STATE_CHANGE) != 0) {
        res = supported_explicit_ib_te_state_granularity != 0;
    } else {
        res = supported_explicit_ib_te_state_granularity == 0;
    }
    TEEIO_PRINT(("         %s.12: %s %s\n", case_msg, mCxlTspGetCapsAssersion[12], res ? "Pass" : "Fail"));
    if (!res) {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    // Assertion.13
    if ((memory_encryption_features_supported & CXL_TSP_MEMORY_ENCRYPTION_FEATURES_SUPPORT_CKID_BASED_ENCRYPTION) != 0) {
        res =  ((device_capabilities->number_of_ckids >= 2) &&
            (device_capabilities->number_of_ckids < 0x2000));
    } else {
        res = device_capabilities->number_of_ckids;
    }
    TEEIO_PRINT(("         %s.13: %s %s\n", case_msg, mCxlTspGetCapsAssersion[13], res ? "Pass" : "Fail"));
    if (!res) {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }


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
    TEEIO_PRINT(("         %s.14: %s %s\n", case_msg, mCxlTspGetCapsAssersion[14], res ? "Pass" : "Fail"));
    if (!res) {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    return LIBSPDM_STATUS_SUCCESS;
}

/**
 * Send and receive an TSP message
 *
 * @param  spdm_context                 A pointer to the SPDM context.
 * @param  session_id                   Indicates if it is a secured message protected via SPDM session.
 *                                     If session_id is NULL, it is a normal message.
 *                                     If session_id is NOT NULL, it is a secured message.
 *
 * @retval LIBSPDM_STATUS_SUCCESS               The TSP request is sent and response is received.
 * @return ERROR                        The TSP response is not received correctly.
 **/
libspdm_return_t test_cxl_tsp_get_capabilities(
    const void *pci_doe_context,
    void *spdm_context, const uint32_t *session_id,
    libcxltsp_device_capabilities_t *device_capabilities)
{
    libspdm_return_t status;
    cxl_tsp_get_target_capabilities_req_t request;
    size_t request_size;
    cxl_tsp_get_target_capabilities_rsp_t response;
    size_t response_size;
    bool res;

    const char* case_msg = "  Assertion 2.1";

    libspdm_zero_mem (&request, sizeof(request));
    request.header.tsp_version = CXL_TSP_MESSAGE_VERSION_10;
    request.header.op_code = CXL_TSP_OPCODE_GET_TARGET_CAPABILITIES;

    request_size = sizeof(request);
    response_size = sizeof(response);
    status = cxl_tsp_send_receive_data(spdm_context, session_id,
                                       &request, request_size,
                                       &response, &response_size);
    // Assersion.0
    TEEIO_PRINT(("         %s.0: %s(0x%x) %s.\n", case_msg, mCxlTspGetCapsAssersion[0], status, !LIBSPDM_STATUS_IS_ERROR(status) ? "Pass" : "failed"));
    if (LIBSPDM_STATUS_IS_ERROR(status))
    {
        return status;
    }

    // Assertion.1
    res = response_size == sizeof(cxl_tsp_get_target_capabilities_rsp_t);
    TEEIO_PRINT(("         %s.1: %s %s\n", case_msg, mCxlTspGetCapsAssersion[1], res ? "Pass" : "Fail"));
    if (!res) {
        return LIBSPDM_STATUS_INVALID_MSG_SIZE;
    }

    // Assertion.2
    res = response.header.op_code == CXL_TSP_OPCODE_GET_TARGET_CAPABILITIES_RSP;
    TEEIO_PRINT(("         %s.2: %s %s\n", case_msg, mCxlTspGetCapsAssersion[2], res ? "Pass" : "Fail"));
    if (!res) {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    // Assertion.3
    res = response.header.tsp_version == request.header.tsp_version;
    TEEIO_PRINT(("         %s.3: %s %s\n", case_msg, mCxlTspGetCapsAssersion[3], res ? "Pass" : "Fail"));
    if (!res) {
        return LIBSPDM_STATUS_INVALID_MSG_FIELD;
    }

    device_capabilities->memory_encryption_features_supported = response.memory_encryption_features_supported;
    device_capabilities->memory_encryption_algorithms_supported = response.memory_encryption_algorithms_supported;
    device_capabilities->memory_encryption_number_of_range_based_keys = response.memory_encryption_number_of_range_based_keys;
    device_capabilities->te_state_change_and_access_control_features_supported = response.te_state_change_and_access_control_features_supported;
    device_capabilities->supported_explicit_oob_te_state_granularity = response.supported_explicit_oob_te_state_granularity;
    device_capabilities->supported_explicit_ib_te_state_granularity = response.supported_explicit_ib_te_state_granularity;
    device_capabilities->configuration_features_supported = response.configuration_features_supported;
    device_capabilities->number_of_ckids = response.number_of_ckids;
    device_capabilities->number_of_secondary_sessions = response.number_of_secondary_sessions;

    status = teeio_cxl_tsp_validate_capability (device_capabilities);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        return status;
    }

    return LIBSPDM_STATUS_SUCCESS;
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

bool cxl_tsp_test_get_caps_run(void *test_context)
{
  libspdm_return_t status;
  bool pass = true;

  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  cxl_tsp_test_group_context_t *group_context = (cxl_tsp_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  teeio_common_test_group_context_t *common = &group_context->common;
  TEEIO_ASSERT(common->signature == GROUP_CONTEXT_SIGNATURE);
  spdm_doe_context_t *spdm_doe = &group_context->spdm_doe;
  TEEIO_ASSERT(spdm_doe);

  libcxltsp_device_capabilities_t device_capabilities;
  memset(&device_capabilities, 0, sizeof(device_capabilities));
  status = test_cxl_tsp_get_capabilities(spdm_doe->doe_context, spdm_doe->spdm_context, &spdm_doe->session_id, &device_capabilities);

  pass = !LIBSPDM_STATUS_IS_ERROR(status);
  case_context->result = pass ? IDE_COMMON_TEST_CASE_RESULT_SUCCESS : IDE_COMMON_TEST_CASE_RESULT_FAILED;

  return true;
}

bool cxl_tsp_test_get_caps_teardown(void *test_context)
{
  return true;
}
