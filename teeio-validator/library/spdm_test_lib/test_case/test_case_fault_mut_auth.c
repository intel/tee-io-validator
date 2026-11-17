/**
 * Copyright 2026 Intel. All rights reserved.
 * License: BSD 3-Clause License.
 **/

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <stdlib.h>
#include "teeio_validator.h"
#include "teeio_fault_injection.h"
#include "teeio_spdmlib.h"
#include "spdm_test_lib.h"
#include "internal/libspdm_common_lib.h"
#include "internal/libspdm_requester_lib.h"
#include "internal/libspdm_device_secret_lib.h"
#include "hal/library/cryptlib/cryptlib_cert.h"
#include "teeio_fault_fixture.h"

#define FINISH_SIG_SET(type, value) \
  do { \
    if (!finish_sig_set(context->spdm_context, type, &(value), sizeof(value))) { \
      return false; \
    } \
  } while (0)

/* Integration: build the requester AND its secret/signing library with
 * LIBSPDM_ENABLE_CAPABILITY_MUT_AUTH_CAP=1; register these setup/run exports
 * only for the FINISH-signature Fault driver. Use the ordinary context teardown.
 * No trust-policy, global signing-mode, or capability defaults are changed.
 * The sample PEM signer must use the matching ecp384/end_requester.key.
 */
#define FINISH_SIG_HASH SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_384
#define FINISH_SIG_ASYM SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384
#define FINISH_SIG_SIGNATURE_SIZE 96
#define FINISH_SIG_HASH_SIZE 48
#define FINISH_SIG_CHAIN_SIZE 1656
#define FINISH_SIG_READY 0x42354d41u

#if LIBSPDM_ENABLE_CAPABILITY_MUT_AUTH_CAP
/* SHA-384 of the SPDM-formatted, SHA-384 sample requester chain. This pins
 * unit_test/sample_key/ecp384/bundle_requester.certchain.der, NOT any chain
 * supplied by a peer or an arbitrary file found in the working directory.
 */
static const uint8_t finish_sig_chain_hash[FINISH_SIG_HASH_SIZE] = {
  0x51,0x31,0x20,0xce,0x51,0xfe,0x84,0x76,0x64,0x9b,0x16,0xd5,
  0xfc,0x9e,0x6c,0x1c,0x9a,0x61,0x11,0xcd,0x3a,0x61,0x0b,0x56,
  0xe8,0x88,0xab,0xff,0x5f,0x12,0x67,0x0f,0x35,0xc7,0xa6,0xd6,
  0x4b,0x31,0xd0,0xf6,0x8d,0x40,0x97,0x81,0x5d,0x24,0x42,0x34
};

typedef struct {
  uint32_t ready;
  uint32_t session_id;
  unsigned int finish_count;
  bool inject;
  bool signature_present;
  bool signature_mutated;
  bool hmac_recomputed;
  bool decrypt_error;
  uint8_t chain[FINISH_SIG_CHAIN_SIZE];
} finish_sig_state_t;

typedef char finish_sig_scratch_size_check[
  sizeof(finish_sig_state_t) <= TEEIO_SPDM_TEST_SCRATCH_BUFFER_SIZE ? 1 : -1];

static bool finish_sig_set(void *spdm, libspdm_data_type_t type,
                   const void *data, size_t size)
{
  libspdm_data_parameter_t parameter = {0};
  parameter.location = LIBSPDM_DATA_LOCATION_LOCAL;
  return libspdm_set_data(spdm, type, &parameter, data, size) ==
         LIBSPDM_STATUS_SUCCESS;
}

static bool finish_sig_get(void *spdm, libspdm_data_type_t type,
                   void *data, size_t size)
{
  libspdm_data_parameter_t parameter = {0};
  size_t actual_size = size;
  parameter.location = LIBSPDM_DATA_LOCATION_CONNECTION;
  return libspdm_get_data(spdm, type, &parameter, data, &actual_size) ==
         LIBSPDM_STATUS_SUCCESS && actual_size == size;
}

/* req_finish appends prefix, signature and verify_data before transport encode.
 * Rewind F, NOT K: in hash-only builds reset_message_f restores the pre-F hash
 * backup (A + responder certificate hash + K). The first append reintroduces
 * the requester certificate hash exactly once. In recording builds only the
 * F managed buffer is cleared. Never hash the old signature/HMAC again or
 * reconstruct K from wire fragments; its signature/verify_data also matter.
 */
static bool finish_sig_finish_hmac(libspdm_context_t *spdm,
                           libspdm_session_info_t *session,
                           const uint8_t *message, size_t size, uint8_t *hmac)
{
#if !LIBSPDM_RECORD_TRANSCRIPT_DATA_SUPPORT
  if (!session->session_transcript.message_f_initialized ||
      session->session_transcript.digest_context_th == NULL ||
      session->session_transcript.digest_context_th_backup == NULL) {
    return false;
  }
#endif
  libspdm_reset_message_f(spdm, session);
  return libspdm_append_message_f(spdm, session, true, message,
                                  size - FINISH_SIG_HASH_SIZE) == LIBSPDM_STATUS_SUCCESS &&
    libspdm_generate_finish_req_hmac(spdm, session, hmac);
}

/* Intercept plaintext BEFORE AEAD protection. Only FINISH is offered to FI;
 * setup's positive handshake never consumes the occurrence=1 FINISH rule.
 * First require exactly the configured signature XOR and an untouched HMAC;
 * only then recompute HMAC over the altered transcript. HMAC-only/wrong-field
 * rules must fail closed, not masquerade as a signature-verification test.
 */
static libspdm_return_t finish_sig_encode(
  void *spdm, const uint32_t *session_id, bool app, bool request,
  size_t size, void *message, size_t *transport_size, void **transport)
{
  finish_sig_state_t *state = ((libspdm_context_t *)spdm)->app_context_data_ptr;
  const uint8_t *bytes = message;
  uint8_t frame[sizeof(pci_doe_data_object_header_t) +
                sizeof(spdm_finish_request_t) + sizeof(uint16_t) +
                SPDM_MAX_OPAQUE_DATA_SIZE + FINISH_SIG_SIGNATURE_SIZE + FINISH_SIG_HASH_SIZE];
  size_t prefix, framed_size, index, target;
  uint8_t hmac[FINISH_SIG_HASH_SIZE];
  libspdm_session_info_t *session;
  teeio_fault_result_t fault;

  if (!app && size >= sizeof(spdm_message_header_t) && bytes[1] == SPDM_FINISH) {
    state->finish_count++;
    prefix = sizeof(spdm_finish_request_t);
    if (bytes[0] >= SPDM_MESSAGE_VERSION_14) {
      if (size < prefix + sizeof(uint16_t)) {
        return LIBSPDM_STATUS_SEND_FAIL;
      }
      prefix += sizeof(uint16_t) + libspdm_read_uint16(bytes + prefix);
    }
    if (!request || session_id == NULL || state->finish_count != 1 ||
        bytes[2] != SPDM_FINISH_REQUEST_ATTRIBUTES_SIGNATURE_INCLUDED ||
        bytes[3] != 0 || size != prefix + FINISH_SIG_SIGNATURE_SIZE + FINISH_SIG_HASH_SIZE ||
        size + sizeof(pci_doe_data_object_header_t) > sizeof(frame)) {
      return LIBSPDM_STATUS_SEND_FAIL;
    }
    session = libspdm_get_session_info_via_session_id(spdm, *session_id);
    if (session == NULL || session->mut_auth_requested !=
        SPDM_KEY_EXCHANGE_RESPONSE_MUT_AUTH_REQUESTED) {
      return LIBSPDM_STATUS_SEND_FAIL;
    }
    state->signature_present = true;
    state->session_id = *session_id;
    if (state->inject) {
      framed_size = sizeof(pci_doe_data_object_header_t) + size;
      libspdm_zero_mem(frame, sizeof(pci_doe_data_object_header_t));
      frame[2] = TEEIO_FAULT_DOE_TYPE_PLAIN_SECURED_SPDM;
      libspdm_write_uint32(frame + 4, (uint32_t)((framed_size + 3) / 4));
      libspdm_copy_mem(frame + sizeof(pci_doe_data_object_header_t),
                       sizeof(frame) - sizeof(pci_doe_data_object_header_t),
                       message, size);
      fault = teeio_fault_apply(frame, framed_size, sizeof(frame));
      if (fault.disposition != TEEIO_FAULT_DISPOSITION_MUTATE ||
          fault.message == NULL || fault.message_size != framed_size ||
          teeio_fault_fire_count() != 1) {
        return LIBSPDM_STATUS_SEND_FAIL;
      }
      target = framed_size - FINISH_SIG_HASH_SIZE - 1;
      for (index = 0; index < framed_size; index++) {
        if (fault.message[index] !=
            (uint8_t)(frame[index] ^ (index == target ? 1 : 0))) {
          return LIBSPDM_STATUS_SEND_FAIL;
        }
      }
      /* Check the saved transcript reproduces the original verify_data before
       * using it for the fault. This also fails closed on unexpected state. */
      if (!finish_sig_finish_hmac(spdm, session, bytes, size, hmac) ||
          !libspdm_consttime_is_mem_equal(hmac, bytes + size - FINISH_SIG_HASH_SIZE,
                                          FINISH_SIG_HASH_SIZE) ||
          !finish_sig_finish_hmac(spdm, session,
                          fault.message + sizeof(pci_doe_data_object_header_t),
                          size, hmac)) {
        return LIBSPDM_STATUS_CRYPTO_ERROR;
      }
      /* Copy into the original sender buffer; preserve its transport space.
       * Keep the live requester transcript in sync too: a verifier-bypassing
       * peer must complete FINISH/END_SESSION, not fail on stale TH2 keys. */
      libspdm_copy_mem(message, size,
                       fault.message + sizeof(pci_doe_data_object_header_t), size);
      libspdm_copy_mem((uint8_t *)message + size - FINISH_SIG_HASH_SIZE,
                       FINISH_SIG_HASH_SIZE, hmac, sizeof(hmac));
      if (libspdm_append_message_f(spdm, session, true, hmac, sizeof(hmac)) !=
          LIBSPDM_STATUS_SUCCESS) {
        return LIBSPDM_STATUS_CRYPTO_ERROR;
      }
      state->signature_mutated = true;
      state->hmac_recomputed = true;
      TEEIO_DEBUG((TEEIO_DEBUG_INFO,
                   "FINISH signature: ECP384 signature=96 recomputed verify_data=48; %s\n",
                   teeio_fault_audit_record()));
    }
  }
  return libspdm_transport_pci_doe_encode_message(
    spdm, session_id, app, request, size, message, transport_size, transport);
}

static libspdm_return_t finish_sig_decode(
  void *spdm, uint32_t **session_id, bool *app, bool request,
  size_t transport_size, void *transport, size_t *size, void **message)
{
  finish_sig_state_t *state = ((libspdm_context_t *)spdm)->app_context_data_ptr;
  libspdm_return_t status;
  const spdm_message_header_t *header;

  status = teeio_fault_transport_decode_message(
    spdm, session_id, app, request, transport_size, transport, size, message);
  if (status == LIBSPDM_STATUS_SUCCESS && !*app &&
      *size == sizeof(spdm_message_header_t) && state->signature_mutated) {
    header = *message;
    state->decrypt_error = header->request_response_code == SPDM_ERROR &&
      header->param1 == SPDM_ERROR_CODE_DECRYPT_ERROR && header->param2 == 0;
  }
  return status;
}

static libspdm_return_t finish_sig_session(teeio_spdm_test_context_t *context,
                                  finish_sig_state_t *state, bool inject)
{
  libspdm_context_t *spdm = context->spdm_context;
  libspdm_transport_encode_message_func saved_encode = spdm->transport_encode_message;
  libspdm_transport_decode_message_func saved_decode = spdm->transport_decode_message;
  void *saved_app = spdm->app_context_data_ptr;
  uint32_t session_id = 0;
  uint8_t heartbeat = 0;
  libspdm_return_t status;

  if (!teeio_fault_fixture_sample_paths()) {
    return LIBSPDM_STATUS_INVALID_PARAMETER;
  }

  state->session_id = 0;
  state->finish_count = 0;
  state->signature_present = false;
  state->signature_mutated = false;
  state->hmac_recomputed = false;
  state->decrypt_error = false;
  state->inject = inject;
  /* The local libspdm has no transport getter / nullable APP_CONTEXT setter.
   * Save and restore these fields even on failures; no process-global observer.
   */
  spdm->app_context_data_ptr = state;
  spdm->transport_encode_message = finish_sig_encode;
  spdm->transport_decode_message = finish_sig_decode;
  status = libspdm_start_session(
    spdm, false, NULL, 0, SPDM_KEY_EXCHANGE_REQUEST_NO_MEASUREMENT_SUMMARY_HASH,
    0, 0, &session_id, &heartbeat, NULL);
  if (status == LIBSPDM_STATUS_SUCCESS) {
    libspdm_return_t stop_status = libspdm_stop_session(spdm, session_id, 0);
    /* An unexpectedly accepted negative FINISH remains a failure, regardless
     * of END_SESSION's result. Positive setup requires successful teardown. */
    if (!inject) {
      status = stop_status;
    }
  }
  spdm->transport_encode_message = saved_encode;
  spdm->transport_decode_message = saved_decode;
  spdm->app_context_data_ptr = saved_app;
  return status;
}

static bool finish_sig_connect_and_get_certificate(void *spdm)
{
  void *chain;
  size_t chain_size;
  uint32_t data32;
  uint16_t data16;
  libspdm_return_t status;

  /* GET_VERSION also clears any pending responder handshake left after a
   * rejected FINISH. Do not assume the peer frees it on DECRYPT_ERROR. */
  if (libspdm_init_connection(spdm, false) != LIBSPDM_STATUS_SUCCESS ||
      !finish_sig_get(spdm, LIBSPDM_DATA_CAPABILITY_FLAGS, &data32, sizeof(data32)) ||
      !(data32 & SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_MUT_AUTH_CAP) ||
      !finish_sig_get(spdm, LIBSPDM_DATA_REQ_BASE_ASYM_ALG, &data16, sizeof(data16)) ||
      data16 != FINISH_SIG_ASYM ||
      !finish_sig_get(spdm, LIBSPDM_DATA_BASE_HASH_ALGO, &data32, sizeof(data32)) ||
      data32 != FINISH_SIG_HASH ||
      !finish_sig_get(spdm, LIBSPDM_DATA_REQ_PQC_ASYM_ALG, &data32, sizeof(data32)) ||
      data32 != 0) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR,
      "FINISH signature prerequisites unavailable: require negotiated MUT_AUTH, requester P-384 and SHA-384; no signature mutation performed\n"));
    return false;
  }
  teeio_spdm_log_negotiated_version(spdm);
  chain = malloc(LIBSPDM_MAX_CERT_CHAIN_SIZE);
  if (chain == NULL) {
    return false;
  }
  chain_size = LIBSPDM_MAX_CERT_CHAIN_SIZE;
  status = libspdm_get_certificate(spdm, NULL, 0, &chain_size, chain);
  free(chain);
  return status == LIBSPDM_STATUS_SUCCESS;
}
#endif

bool spdm_test_case_fault_mut_auth_setup(void *test_context)
{
#if LIBSPDM_ENABLE_CAPABILITY_MUT_AUTH_CAP
  teeio_spdm_test_context_t *context = test_context;
  finish_sig_state_t *state;
  void *chain = NULL;
  size_t chain_size = 0;
  uint8_t digest[FINISH_SIG_HASH_SIZE];
  spdm_version_number_t negotiated_version;
  uint8_t data8;
  uint16_t data16;
  uint32_t data32;

  if (context == NULL || context->spdm_context == NULL) {
    return false;
  }
  state = (void *)context->test_scratch_buffer;
  libspdm_zero_mem(state, sizeof(*state));
  context->test_scratch_buffer_size = sizeof(*state);
  if (teeio_fault_driver() != TEEIO_FAULT_DRIVER_FINISH_SIGNATURE ||
      teeio_fault_fire_count() != 0 ||
      !teeio_spdm_apply_version_override(context->spdm_context)) {
    return false;
  }
  data8 = 0;
  FINISH_SIG_SET(LIBSPDM_DATA_CAPABILITY_CT_EXPONENT, data8);
  /* Basic mutual auth: CERT_CAP + MUT_AUTH_CAP; no ENCAP or PUB_KEY_ID. */
  data32 = SPDM_GET_CAPABILITIES_REQUEST_FLAGS_CERT_CAP |
           SPDM_GET_CAPABILITIES_REQUEST_FLAGS_MUT_AUTH_CAP |
           SPDM_GET_CAPABILITIES_REQUEST_FLAGS_ENCRYPT_CAP |
           SPDM_GET_CAPABILITIES_REQUEST_FLAGS_MAC_CAP |
           SPDM_GET_CAPABILITIES_REQUEST_FLAGS_KEY_EX_CAP |
           SPDM_GET_CAPABILITIES_REQUEST_FLAGS_CHUNK_CAP;
  FINISH_SIG_SET(LIBSPDM_DATA_CAPABILITY_FLAGS, data32);
  data8 = SPDM_MEASUREMENT_SPECIFICATION_DMTF;
  FINISH_SIG_SET(LIBSPDM_DATA_MEASUREMENT_SPEC, data8);
  data32 = FINISH_SIG_ASYM;
  FINISH_SIG_SET(LIBSPDM_DATA_BASE_ASYM_ALGO, data32);
  data32 = FINISH_SIG_HASH;
  FINISH_SIG_SET(LIBSPDM_DATA_BASE_HASH_ALGO, data32);
  data32 = 0;
  FINISH_SIG_SET(LIBSPDM_DATA_PQC_ASYM_ALGO, data32);
  FINISH_SIG_SET(LIBSPDM_DATA_REQ_PQC_ASYM_ALG, data32);
  FINISH_SIG_SET(LIBSPDM_DATA_KEM_ALG, data32);
  data16 = FINISH_SIG_ASYM;
  FINISH_SIG_SET(LIBSPDM_DATA_REQ_BASE_ASYM_ALG, data16);
  data16 = SPDM_ALGORITHMS_DHE_NAMED_GROUP_SECP_384_R1;
  FINISH_SIG_SET(LIBSPDM_DATA_DHE_NAME_GROUP, data16);
  data16 = SPDM_ALGORITHMS_AEAD_CIPHER_SUITE_AES_256_GCM;
  FINISH_SIG_SET(LIBSPDM_DATA_AEAD_CIPHER_SUITE, data16);
  data16 = SPDM_ALGORITHMS_KEY_SCHEDULE_SPDM;
  FINISH_SIG_SET(LIBSPDM_DATA_KEY_SCHEDULE, data16);
  data8 = SPDM_ALGORITHMS_OPAQUE_DATA_FORMAT_1;
  FINISH_SIG_SET(LIBSPDM_DATA_OTHER_PARAMS_SUPPORT, data8);
  data8 = 1;
  FINISH_SIG_SET(LIBSPDM_DATA_LOCAL_SUPPORTED_SLOT_MASK, data8);
  if (!teeio_fault_fixture_sample_paths() ||
      !libspdm_read_requester_public_certificate_chain(
        FINISH_SIG_HASH, FINISH_SIG_ASYM, &chain, &chain_size, NULL, NULL)) {
    return false;
  }
  if (chain_size != sizeof(state->chain) ||
      !libspdm_hash_all(FINISH_SIG_HASH, chain, chain_size, digest) ||
      !libspdm_consttime_is_mem_equal(digest, finish_sig_chain_hash, sizeof(digest))) {
    free(chain);
    return false;
  }
  libspdm_copy_mem(state->chain, sizeof(state->chain), chain, chain_size);
  free(chain);
  if (!finish_sig_set(context->spdm_context, LIBSPDM_DATA_LOCAL_PUBLIC_CERT_CHAIN,
               state->chain, sizeof(state->chain))) {
    return false;
  }
  if (!finish_sig_connect_and_get_certificate(context->spdm_context) ||
      !finish_sig_get(context->spdm_context, LIBSPDM_DATA_SPDM_VERSION,
                      &negotiated_version, sizeof(negotiated_version)) ||
      !libspdm_verify_cert_chain_data(
        (uint8_t)(negotiated_version >> SPDM_VERSION_NUMBER_SHIFT_BIT),
        state->chain + sizeof(spdm_cert_chain_t) + FINISH_SIG_HASH_SIZE,
        sizeof(state->chain) - sizeof(spdm_cert_chain_t) - FINISH_SIG_HASH_SIZE,
        FINISH_SIG_ASYM, 0, FINISH_SIG_HASH, true,
        SPDM_CERTIFICATE_INFO_CERT_MODEL_DEVICE_CERT) ||
      finish_sig_session(context, state, false) != LIBSPDM_STATUS_SUCCESS ||
      !state->signature_present || teeio_fault_fire_count() != 0) {
    return false;
  }
  state->ready = FINISH_SIG_READY;
  return true;
#else
  (void)test_context;
  TEEIO_DEBUG((TEEIO_DEBUG_ERROR,
               "FINISH signature driver requires LIBSPDM_ENABLE_CAPABILITY_MUT_AUTH_CAP=1 in all requester libraries\n"));
  return false;
#endif
}

void spdm_test_case_fault_mut_auth_run(void *test_context)
{
  bool passed = false;
  libspdm_return_t status = LIBSPDM_STATUS_UNSUPPORTED_CAP;
#if LIBSPDM_ENABLE_CAPABILITY_MUT_AUTH_CAP
  teeio_spdm_test_context_t *context = test_context;
  finish_sig_state_t *state;
  if (context != NULL && context->spdm_context != NULL &&
      context->test_scratch_buffer_size == sizeof(finish_sig_state_t)) {
    state = (void *)context->test_scratch_buffer;
    if (state->ready == FINISH_SIG_READY &&
        teeio_fault_driver() == TEEIO_FAULT_DRIVER_FINISH_SIGNATURE &&
        teeio_fault_fire_count() == 0) {
      state->ready = 0;
      status = finish_sig_session(context, state, true);
      passed = state->signature_present && state->signature_mutated &&
        state->hmac_recomputed && state->decrypt_error && teeio_fault_scenario_fired() &&
        teeio_fault_fire_count() == 1 && status == LIBSPDM_STATUS_SESSION_MSG_ERROR &&
        state->session_id != 0 && libspdm_get_session_info_via_session_id(
          context->spdm_context, state->session_id) == NULL;
      if (passed) {
        /* Explicit new-connection recovery, not same-session recovery. Keep
         * the original FI audit/count; the positive FINISH bypasses injection.
         * Never convert a generic failure into a pass by merely reconnecting. */
        passed = finish_sig_connect_and_get_certificate(context->spdm_context) &&
          finish_sig_session(context, state, false) == LIBSPDM_STATUS_SUCCESS &&
          state->signature_present && teeio_fault_fire_count() == 1;
      }
    }
  }
#else
  (void)test_context;
#endif
  teeio_record_assertion_result(
    SPDM_TEST_CASE_FAULT, teeio_fault_driver(), 1,
    IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
    passed ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
    "FINISH signature driver requires signature FI + valid recomputed HMAC + DECRYPT_ERROR + local cleanup + signed reconnect/END_SESSION; status=0x%x",
    (unsigned int)status);
}
