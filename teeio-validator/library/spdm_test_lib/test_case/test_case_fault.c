/**
 *  Copyright Notice:
 *  Copyright 2026 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>

#include "teeio_validator.h"
#include "teeio_fault_injection.h"
#include "teeio_spdmlib.h"
#include "spdm_test_lib.h"
#include "hal/library/cryptlib/cryptlib_cert.h"

#define TEEIO_FAULT_SET_CERTIFICATE_SCENARIO \
  "B5_certificate_chunk_bit_flip"
#define TEEIO_FAULT_SET_CERTIFICATE_ASSERTION_ID 1

typedef struct {
  uint8_t version;
  uint8_t heartbeat_period;
  uint8_t reserved[2];
  uint32_t session_id;
} teeio_fault_test_buffer_t;

void spdm_test_case_common_teardown(void *test_context);

static bool set_local_data(void *spdm_context, libspdm_data_type_t data_type,
                           const void *data, size_t data_size)
{
  libspdm_data_parameter_t parameter;

  libspdm_zero_mem(&parameter, sizeof(parameter));
  parameter.location = LIBSPDM_DATA_LOCATION_LOCAL;
  return !LIBSPDM_STATUS_IS_ERROR(libspdm_set_data(
    spdm_context, data_type, &parameter, data, data_size));
}

static bool get_connection_data(void *spdm_context,
                                libspdm_data_type_t data_type,
                                void *data, size_t data_size)
{
  libspdm_data_parameter_t parameter;

  libspdm_zero_mem(&parameter, sizeof(parameter));
  parameter.location = LIBSPDM_DATA_LOCATION_CONNECTION;
  return !LIBSPDM_STATUS_IS_ERROR(libspdm_get_data(
    spdm_context, data_type, &parameter, data, &data_size));
}

static bool select_alias_immutable_chain(void *cert_chain,
                                         size_t *cert_chain_size,
                                         uint32_t base_hash_algo)
{
  spdm_cert_chain_t *chain_header = cert_chain;
  const uint8_t *certs;
  const uint8_t *cert;
  size_t certs_size;
  size_t cert_size;
  size_t header_size;
  size_t immutable_size;
  int32_t cert_index;

  header_size = sizeof(spdm_cert_chain_t) +
                libspdm_get_hash_size(base_hash_algo);
  if (cert_chain == NULL || cert_chain_size == NULL ||
      *cert_chain_size <= header_size) {
    return false;
  }

  certs = (const uint8_t *)cert_chain + header_size;
  certs_size = *cert_chain_size - header_size;
  immutable_size = 0;
  cert_index = 0;
  while (immutable_size < certs_size) {
    if (!libspdm_x509_get_cert_from_cert_chain(
          certs, certs_size, cert_index, &cert, &cert_size)) {
      return false;
    }
    if ((size_t)(cert - certs) + cert_size == certs_size) {
      if (cert_index == 0) {
        return false;
      }
      *cert_chain_size = header_size + immutable_size;
      chain_header->length = (uint32_t)*cert_chain_size;
      return true;
    }
    immutable_size = (size_t)(cert - certs) + cert_size;
    cert_index++;
  }
  return false;
}

bool spdm_test_case_fault_setup(void *test_context)
{
  teeio_spdm_test_context_t *context = test_context;
  teeio_fault_test_buffer_t *test_buffer;
  libspdm_data_parameter_t parameter;
  libspdm_return_t status;
  uint8_t *cert_chain;
  size_t cert_chain_size;
  size_t data_size;
  spdm_version_number_t version;
  uint32_t capabilities;
  uint32_t data32;
  uint16_t data16;
  uint8_t data8;

  if (!teeio_fault_scenario_is(TEEIO_FAULT_SET_CERTIFICATE_SCENARIO) ||
      !teeio_spdm_apply_version_override(context->spdm_context)) {
    return false;
  }

  data8 = 0;
  if (!set_local_data(context->spdm_context,
                      LIBSPDM_DATA_CAPABILITY_CT_EXPONENT,
                      &data8, sizeof(data8))) {
    return false;
  }
  capabilities = SPDM_GET_CAPABILITIES_REQUEST_FLAGS_CERT_CAP |
                 SPDM_GET_CAPABILITIES_REQUEST_FLAGS_ENCRYPT_CAP |
                 SPDM_GET_CAPABILITIES_REQUEST_FLAGS_MAC_CAP |
                 SPDM_GET_CAPABILITIES_REQUEST_FLAGS_KEY_EX_CAP |
                 SPDM_GET_CAPABILITIES_REQUEST_FLAGS_ENCAP_CAP |
                 SPDM_GET_CAPABILITIES_REQUEST_FLAGS_HBEAT_CAP |
                 SPDM_GET_CAPABILITIES_REQUEST_FLAGS_KEY_UPD_CAP |
                 SPDM_GET_CAPABILITIES_REQUEST_FLAGS_CHUNK_CAP;
  if (!set_local_data(context->spdm_context, LIBSPDM_DATA_CAPABILITY_FLAGS,
                      &capabilities, sizeof(capabilities))) {
    return false;
  }

  data8 = SPDM_MEASUREMENT_SPECIFICATION_DMTF;
  if (!set_local_data(context->spdm_context, LIBSPDM_DATA_MEASUREMENT_SPEC,
                      &data8, sizeof(data8))) {
    return false;
  }
  data32 = SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;
  if (!set_local_data(context->spdm_context, LIBSPDM_DATA_BASE_ASYM_ALGO,
                      &data32, sizeof(data32))) {
    return false;
  }
  data32 = SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_384;
  if (!set_local_data(context->spdm_context, LIBSPDM_DATA_BASE_HASH_ALGO,
                      &data32, sizeof(data32))) {
    return false;
  }
  data32 = 0;
  if (!set_local_data(context->spdm_context, LIBSPDM_DATA_PQC_ASYM_ALGO,
                      &data32, sizeof(data32)) ||
      !set_local_data(context->spdm_context, LIBSPDM_DATA_KEM_ALG,
                      &data32, sizeof(data32))) {
    return false;
  }
  data16 = SPDM_ALGORITHMS_DHE_NAMED_GROUP_SECP_384_R1;
  if (!set_local_data(context->spdm_context, LIBSPDM_DATA_DHE_NAME_GROUP,
                      &data16, sizeof(data16))) {
    return false;
  }
  data16 = SPDM_ALGORITHMS_AEAD_CIPHER_SUITE_AES_256_GCM;
  if (!set_local_data(context->spdm_context, LIBSPDM_DATA_AEAD_CIPHER_SUITE,
                      &data16, sizeof(data16))) {
    return false;
  }
  data16 = 0;
  if (!set_local_data(context->spdm_context, LIBSPDM_DATA_REQ_BASE_ASYM_ALG,
                      &data16, sizeof(data16))) {
    return false;
  }
  data16 = SPDM_ALGORITHMS_KEY_SCHEDULE_SPDM;
  if (!set_local_data(context->spdm_context, LIBSPDM_DATA_KEY_SCHEDULE,
                      &data16, sizeof(data16))) {
    return false;
  }
  data8 = SPDM_ALGORITHMS_OPAQUE_DATA_FORMAT_1;
  if (!set_local_data(context->spdm_context, LIBSPDM_DATA_OTHER_PARAMS_SUPPORT,
                      &data8, sizeof(data8))) {
    return false;
  }

  status = libspdm_init_connection(context->spdm_context, false);
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    return false;
  }
  teeio_spdm_log_negotiated_version(context->spdm_context);

  capabilities = 0;
  if (!get_connection_data(context->spdm_context,
                           LIBSPDM_DATA_CAPABILITY_FLAGS,
                           &capabilities, sizeof(capabilities)) ||
      (capabilities & SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_SET_CERT_CAP) == 0 ||
      (capabilities & SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_CHUNK_CAP) == 0) {
    return false;
  }

  cert_chain = malloc(LIBSPDM_MAX_CERT_CHAIN_SIZE);
  if (cert_chain == NULL) {
    return false;
  }
  cert_chain_size = LIBSPDM_MAX_CERT_CHAIN_SIZE;
  status = libspdm_get_certificate(context->spdm_context, NULL, 0,
                                   &cert_chain_size, cert_chain);
  free(cert_chain);
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    return false;
  }

  test_buffer = (void *)context->test_scratch_buffer;
  if (sizeof(context->test_scratch_buffer) < sizeof(*test_buffer)) {
    return false;
  }
  libspdm_zero_mem(test_buffer, sizeof(*test_buffer));
  context->test_scratch_buffer_size = sizeof(*test_buffer);

  status = libspdm_start_session(
    context->spdm_context, false, NULL, 0,
    SPDM_KEY_EXCHANGE_REQUEST_NO_MEASUREMENT_SUMMARY_HASH, 0,
    SPDM_KEY_EXCHANGE_REQUEST_SESSION_POLICY_TERMINATION_POLICY_RUNTIME_UPDATE,
    &test_buffer->session_id, &test_buffer->heartbeat_period, NULL);
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    return false;
  }

  libspdm_zero_mem(&parameter, sizeof(parameter));
  parameter.location = LIBSPDM_DATA_LOCATION_CONNECTION;
  data_size = sizeof(version);
  status = libspdm_get_data(context->spdm_context, LIBSPDM_DATA_SPDM_VERSION,
                            &parameter, &version, &data_size);
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    return false;
  }
  test_buffer->version = (uint8_t)(version >> SPDM_VERSION_NUMBER_SHIFT_BIT);
  return true;
}

void spdm_test_case_fault_run(void *test_context)
{
  teeio_spdm_test_context_t *context = test_context;
  teeio_fault_test_buffer_t *test_buffer;
  libspdm_return_t status;
  void *cert_chain;
  size_t cert_chain_size;
  uint32_t base_hash_algo;
  uint32_t capabilities;
  uint32_t data_transfer_size;

  test_buffer = (void *)context->test_scratch_buffer;
  if (context->test_scratch_buffer_size != sizeof(*test_buffer) ||
      test_buffer->session_id == 0) {
    teeio_record_assertion_result(
      SPDM_TEST_CASE_FAULT, 1, TEEIO_FAULT_SET_CERTIFICATE_ASSERTION_ID,
      IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED,
      "SET_CERTIFICATE session is unavailable");
    return;
  }

  if (!get_connection_data(context->spdm_context,
                           LIBSPDM_DATA_BASE_HASH_ALGO,
                           &base_hash_algo, sizeof(base_hash_algo)) ||
      !get_connection_data(context->spdm_context,
                           LIBSPDM_DATA_CAPABILITY_FLAGS,
                           &capabilities, sizeof(capabilities)) ||
      !get_connection_data(context->spdm_context,
                           LIBSPDM_DATA_CAPABILITY_DATA_TRANSFER_SIZE,
                           &data_transfer_size, sizeof(data_transfer_size))) {
    teeio_record_assertion_result(
      SPDM_TEST_CASE_FAULT, 1, TEEIO_FAULT_SET_CERTIFICATE_ASSERTION_ID,
      IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED,
      "failed to read negotiated SPDM data");
    return;
  }

  cert_chain = malloc(LIBSPDM_MAX_CERT_CHAIN_SIZE);
  if (cert_chain == NULL) {
    teeio_record_assertion_result(
      SPDM_TEST_CASE_FAULT, 1, TEEIO_FAULT_SET_CERTIFICATE_ASSERTION_ID,
      IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED,
      "failed to allocate certificate chain");
    return;
  }
  cert_chain_size = LIBSPDM_MAX_CERT_CHAIN_SIZE;
  status = libspdm_get_certificate(context->spdm_context, NULL, 0,
                                   &cert_chain_size, cert_chain);
  if (LIBSPDM_STATUS_IS_ERROR(status) ||
      ((capabilities & SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_ALIAS_CERT_CAP) != 0 &&
       !select_alias_immutable_chain(cert_chain, &cert_chain_size,
                                     base_hash_algo)) ||
      cert_chain_size + sizeof(spdm_set_certificate_request_t) <=
        data_transfer_size) {
    free(cert_chain);
    teeio_record_assertion_result(
      SPDM_TEST_CASE_FAULT, 1, TEEIO_FAULT_SET_CERTIFICATE_ASSERTION_ID,
      IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED,
      "certificate chain does not trigger CHUNK_SEND");
    return;
  }

  status = libspdm_set_certificate(
    context->spdm_context, NULL, 0,
    cert_chain, cert_chain_size);
  free(cert_chain);

  teeio_record_assertion_result(
    SPDM_TEST_CASE_FAULT, 1, TEEIO_FAULT_SET_CERTIFICATE_ASSERTION_ID,
    IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
    teeio_fault_scenario_fired() && LIBSPDM_STATUS_IS_ERROR(status) ?
      TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
    "corrupted SET_CERTIFICATE status - 0x%x", (uint32_t)status);
}

void spdm_test_case_fault_teardown(void *test_context)
{
  teeio_spdm_test_context_t *context = test_context;
  teeio_fault_test_buffer_t *test_buffer;

  test_buffer = (void *)context->test_scratch_buffer;
  if (context->test_scratch_buffer_size == sizeof(*test_buffer) &&
      test_buffer->session_id != 0) {
    libspdm_stop_session(context->spdm_context, test_buffer->session_id, 0);
  }
  spdm_test_case_common_teardown(test_context);
}
