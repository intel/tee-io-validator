/* Copyright 2026 Intel. SPDX-License-Identifier: BSD-3-Clause */
#include <stdlib.h>
#include "teeio_validator.h"
#include "teeio_fault_injection.h"
#include "teeio_spdmlib.h"
#include "spdm_test_lib.h"
#include "internal/libspdm_common_lib.h"

#define KEX_SET(type, value) \
  do { \
    if (!kex_set(spdm, type, &(value), sizeof(value))) { \
      return false; \
    } \
  } while (0)

/* Fault drivers for duplicate KEY_EXCHANGE and the cached-certificate control.
 * No protocol requirement to reject a repeated requester half is assumed.
 * This driver requires two successful RSPs with distinct composed IDs, then
 * authenticates the second RSP and completes FINISH using the real requester.
 * GET_VERSION recovers BOTH remote handshakes (including the orphan first).
 */
#define KEX_MAGIC 0x4b455844u
#define KEX_DOE sizeof(pci_doe_data_object_header_t)

void spdm_test_case_common_teardown(void *test_context);

typedef struct {
  uint32_t magic;
  bool ready, duplicate, target, pending, sent_twice, distinct, dirty;
  unsigned kex_count, cert_count, finish_count;
  uint16_t req_id;
  uint32_t first_id, second_id;
  uint8_t *chain, *wire, *reply;
  size_t chain_size, wire_size;
  libspdm_device_send_message_func send;
  libspdm_device_receive_message_func receive;
  libspdm_transport_encode_message_func encode;
  libspdm_transport_decode_message_func decode;
  void *app;
} kex_state_t;

typedef char kex_scratch_check[
  sizeof(kex_state_t) <= TEEIO_SPDM_TEST_SCRATCH_BUFFER_SIZE ? 1 : -1];

static bool kex_set(void *spdm, libspdm_data_type_t type,
                   const void *data, size_t size)
{
  libspdm_data_parameter_t p = {0};
  p.location = LIBSPDM_DATA_LOCATION_LOCAL;
  return libspdm_set_data(spdm, type, &p, data, size) == LIBSPDM_STATUS_SUCCESS;
}

static libspdm_return_t kex_send_base(libspdm_context_t *spdm, kex_state_t *s,
                                    size_t size, const void *msg, uint64_t timeout)
{
  libspdm_return_t status;
  spdm->app_context_data_ptr = s->app;
  status = s->send(spdm, size, msg, timeout);
  spdm->app_context_data_ptr = s;
  return status;
}

static libspdm_return_t kex_receive_base(libspdm_context_t *spdm, kex_state_t *s,
                                       size_t *size, void **msg, uint64_t timeout)
{
  libspdm_return_t status;
  spdm->app_context_data_ptr = s->app;
  status = s->receive(spdm, size, msg, timeout);
  spdm->app_context_data_ptr = s;
  return status;
}

static libspdm_return_t kex_encode(void *ctx, const uint32_t *sid, bool app,
  bool request, size_t size, void *msg, size_t *out_size, void **out)
{
  kex_state_t *s = ((libspdm_context_t *)ctx)->app_context_data_ptr;
  const uint8_t *bytes = msg;
  if (s->target && request && !app && size >= sizeof(spdm_message_header_t)) {
    s->cert_count += bytes[1] == SPDM_GET_CERTIFICATE;
    s->finish_count += bytes[1] == SPDM_FINISH;
    s->kex_count += bytes[1] == SPDM_KEY_EXCHANGE;
  }
  /* Bypass the generic plaintext injector: setup/recovery are never faults. */
  return libspdm_transport_pci_doe_encode_message(
    ctx, sid, app, request, size, msg, out_size, out);
}

static libspdm_return_t kex_send(void *ctx, size_t size, const void *msg,
                               uint64_t timeout)
{
  libspdm_context_t *spdm = ctx;
  kex_state_t *s = spdm->app_context_data_ptr;
  const uint8_t *bytes = msg;
  teeio_fault_result_t fault;
  if (s->target && s->duplicate && size >= KEX_DOE + sizeof(spdm_key_exchange_request_t) &&
      bytes[2] == PCI_DOE_DATA_OBJECT_TYPE_SPDM && bytes[KEX_DOE + 1] == SPDM_KEY_EXCHANGE) {
    if (s->pending || s->sent_twice || size > TEEIO_FAULT_MAX_MESSAGE_SIZE) {
      return LIBSPDM_STATUS_SEND_FAIL;
    }
    libspdm_copy_mem(s->wire, TEEIO_FAULT_MAX_MESSAGE_SIZE, msg, size);
    s->wire[2] = TEEIO_FAULT_DOE_TYPE_PLAIN_SPDM;
    fault = teeio_fault_apply(s->wire, size, TEEIO_FAULT_MAX_MESSAGE_SIZE);
    /* FI is a real side-effect decision, not a manufactured fired flag.
     * Require byte-identical primary and secondary, not a mutation/replay. */
    if (fault.disposition != TEEIO_FAULT_DISPOSITION_DUPLICATE ||
        fault.message == NULL || fault.secondary_message == NULL ||
        fault.message_size != size || fault.secondary_message_size != size ||
        teeio_fault_fire_count() != 1 ||
        !libspdm_consttime_is_mem_equal(fault.message, s->wire, size) ||
        !libspdm_consttime_is_mem_equal(fault.secondary_message, s->wire, size)) {
      return LIBSPDM_STATUS_SEND_FAIL;
    }
    s->wire[2] = bytes[2];
    s->wire_size = size;
    s->req_id = libspdm_read_uint16(bytes + KEX_DOE + 4);
    s->pending = true;
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Duplicate KEY_EXCHANGE: %s\n", teeio_fault_audit_record()));
  }
  s->dirty = true;
  return kex_send_base(spdm, s, size, msg, timeout);
}

static bool kex_response_id(const kex_state_t *s, const void *msg, size_t size,
                            uint32_t *id)
{
  const uint8_t *b = msg;
  /* Fixed P384/SHA384, no measurement summary or mutual authentication.
   * Validate the first (otherwise discarded) RSP's complete wire shape too.
   * The second RSP additionally undergoes libspdm signature/HMAC checking. */
  size_t opaque_offset = KEX_DOE + sizeof(spdm_key_exchange_response_t) + 96;
  size_t unpadded_size;
  uint16_t opaque_size;
  if (size < opaque_offset + sizeof(uint16_t) + 96 + 48 ||
      b[2] != PCI_DOE_DATA_OBJECT_TYPE_SPDM ||
      b[KEX_DOE] != s->wire[KEX_DOE] || b[KEX_DOE + 1] != SPDM_KEY_EXCHANGE_RSP ||
      b[KEX_DOE + 3] != 0 || b[KEX_DOE + 6] != 0 || b[KEX_DOE + 7] != 0 ||
      (libspdm_read_uint32(b + 4) & 0x3ffff) * 4 != size) {
    return false;
  }
  opaque_size = libspdm_read_uint16(b + opaque_offset);
  unpadded_size = opaque_offset + sizeof(uint16_t) + opaque_size + 96 + 48;
  if (opaque_size > SPDM_MAX_OPAQUE_DATA_SIZE || ((unpadded_size + 3) & ~(size_t)3) != size) {
    return false;
  }
  *id = libspdm_generate_session_id(s->req_id, libspdm_read_uint16(b + KEX_DOE + 4));
  return *id != INVALID_SESSION_ID && (*id >> 16) != 0;
}

static libspdm_return_t kex_receive(void *ctx, size_t *size, void **msg,
                                  uint64_t timeout)
{
  libspdm_context_t *spdm = ctx;
  kex_state_t *s = spdm->app_context_data_ptr;
  size_t capacity = *size, second_size = TEEIO_FAULT_MAX_MESSAGE_SIZE;
  void *second = s->reply;
  libspdm_return_t status = kex_receive_base(spdm, s, size, msg, timeout);
  if (!s->pending) {
    return status;
  }
  s->pending = false;
  if (status != LIBSPDM_STATUS_SUCCESS ||
      !kex_response_id(s, *msg, *size, &s->first_id)) {
    return LIBSPDM_STATUS_RECEIVE_FAIL;
  }
  status = kex_send_base(spdm, s, s->wire_size, s->wire, timeout);
  if (status != LIBSPDM_STATUS_SUCCESS) {
    return status;
  }
  s->sent_twice = true;
  status = kex_receive_base(spdm, s, &second_size, &second, timeout);
  if (status != LIBSPDM_STATUS_SUCCESS || second_size > capacity ||
      !kex_response_id(s, second, second_size, &s->second_id)) {
    return LIBSPDM_STATUS_RECEIVE_FAIL;
  }
  s->distinct = s->first_id != s->second_id;
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Duplicate KEY_EXCHANGE req=0x%04x first=0x%08x second=0x%08x\n",
               s->req_id, s->first_id, s->second_id));
  if (!s->distinct) {
    teeio_fault_record_actual("duplicate_composed_session_id");
    return LIBSPDM_STATUS_RECEIVE_FAIL;
  }
  /* The real requester authenticates the second signed RSP against the SAME
   * request transcript and DHE secret, then FINISH authenticates the session.
   * The first remote handshake remains live until explicit VERSION recovery. */
  libspdm_copy_mem(*msg, capacity, second, second_size);
  *size = second_size;
  return LIBSPDM_STATUS_SUCCESS;
}

static bool kex_restore_cache(void *spdm, kex_state_t *s)
{
  libspdm_data_parameter_t p = {0};
  p.location = LIBSPDM_DATA_LOCATION_CONNECTION;
  /* Slot zero. PUBLIC_CERT_CHAIN was removed by this libspdm revision.
   * This setter copies/hashes the SPDM-formatted chain and extracts its leaf
   * key; it does NOT verify authority. Only setup's trusted chain is allowed. */
  return libspdm_set_data(spdm, LIBSPDM_DATA_PEER_USED_CERT_CHAIN_BUFFER,
    &p, s->chain, s->chain_size) == LIBSPDM_STATUS_SUCCESS;
}

static bool kex_empty(libspdm_context_t *spdm)
{
  size_t i;
  for (i = 0; i < LIBSPDM_MAX_SESSION_COUNT; i++) {
    if (spdm->session_info[i].session_id != INVALID_SESSION_ID) {
      return false;
    }
  }
  return true;
}

static bool kex_session(void *spdm)
{
  uint32_t sid = 0;
  uint8_t heartbeat = 0;
  libspdm_return_t status = libspdm_start_session(spdm, false, NULL, 0,
    SPDM_KEY_EXCHANGE_REQUEST_NO_MEASUREMENT_SUMMARY_HASH, 0, 0,
    &sid, &heartbeat, NULL);
  return status == LIBSPDM_STATUS_SUCCESS &&
    libspdm_stop_session(spdm, sid, 0) == LIBSPDM_STATUS_SUCCESS;
}

static bool kex_version_reset(void *spdm, kex_state_t *s)
{
  bool ok;
  s->target = false;
  s->pending = false;
  ok = libspdm_init_connection(spdm, false) == LIBSPDM_STATUS_SUCCESS && kex_empty(spdm);
  s->dirty = !ok;
  return ok;
}

bool spdm_test_case_fault_key_exchange_setup(void *test_context)
{
  teeio_spdm_test_context_t *test = test_context;
  libspdm_context_t *spdm;
  kex_state_t *s;
  uint32_t n32;
  uint16_t n16;
  uint8_t n8;
  const void *anchor = NULL;
  size_t anchor_size = 0;
  if (test == NULL || test->spdm_context == NULL ||
      (teeio_fault_driver() != TEEIO_FAULT_DRIVER_KEY_EXCHANGE_DUPLICATE &&
       teeio_fault_driver() != TEEIO_FAULT_DRIVER_KEY_EXCHANGE_CACHED_CERT) ||
      teeio_fault_fire_count() != 0 || teeio_fault_same_session_recovery_configured()) {
    return false;
  }
  spdm = test->spdm_context;
  s = (void *)test->test_scratch_buffer;
  libspdm_zero_mem(s, sizeof(*s));
  test->test_scratch_buffer_size = sizeof(*s);
  s->chain = malloc(LIBSPDM_MAX_CERT_CHAIN_SIZE);
  s->wire = malloc(TEEIO_FAULT_MAX_MESSAGE_SIZE);
  s->reply = malloc(TEEIO_FAULT_MAX_MESSAGE_SIZE);
  if (!s->chain || !s->wire || !s->reply) {
    free(s->chain); free(s->wire); free(s->reply);
    libspdm_zero_mem(s, sizeof(*s));
    return false;
  }
  s->magic = KEX_MAGIC;
  s->duplicate = teeio_fault_driver() == TEEIO_FAULT_DRIVER_KEY_EXCHANGE_DUPLICATE;
  s->send = spdm->send_message;
  s->receive = spdm->receive_message;
  s->encode = spdm->transport_encode_message;
  s->decode = spdm->transport_decode_message;
  s->app = spdm->app_context_data_ptr;
  spdm->send_message = kex_send;
  spdm->receive_message = kex_receive;
  spdm->transport_encode_message = kex_encode;
  spdm->transport_decode_message = libspdm_transport_pci_doe_decode_message;
  spdm->app_context_data_ptr = s;
  if (!teeio_spdm_apply_version_override(spdm)) {
    return false;
  }
  n8 = 0; KEX_SET(LIBSPDM_DATA_CAPABILITY_CT_EXPONENT, n8);
  n32 = SPDM_GET_CAPABILITIES_REQUEST_FLAGS_CERT_CAP |
    SPDM_GET_CAPABILITIES_REQUEST_FLAGS_ENCRYPT_CAP |
    SPDM_GET_CAPABILITIES_REQUEST_FLAGS_MAC_CAP |
    SPDM_GET_CAPABILITIES_REQUEST_FLAGS_KEY_EX_CAP |
    SPDM_GET_CAPABILITIES_REQUEST_FLAGS_CHUNK_CAP;
  KEX_SET(LIBSPDM_DATA_CAPABILITY_FLAGS, n32);
  n8 = SPDM_MEASUREMENT_SPECIFICATION_DMTF; KEX_SET(LIBSPDM_DATA_MEASUREMENT_SPEC, n8);
  n32 = SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;
  KEX_SET(LIBSPDM_DATA_BASE_ASYM_ALGO, n32);
  n32 = SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_384; KEX_SET(LIBSPDM_DATA_BASE_HASH_ALGO, n32);
  n32 = 0; KEX_SET(LIBSPDM_DATA_PQC_ASYM_ALGO, n32);
  KEX_SET(LIBSPDM_DATA_KEM_ALG, n32); KEX_SET(LIBSPDM_DATA_REQ_PQC_ASYM_ALG, n32);
  n16 = 0; KEX_SET(LIBSPDM_DATA_REQ_BASE_ASYM_ALG, n16);
  n16 = SPDM_ALGORITHMS_DHE_NAMED_GROUP_SECP_384_R1; KEX_SET(LIBSPDM_DATA_DHE_NAME_GROUP, n16);
  n16 = SPDM_ALGORITHMS_AEAD_CIPHER_SUITE_AES_256_GCM; KEX_SET(LIBSPDM_DATA_AEAD_CIPHER_SUITE, n16);
  n16 = SPDM_ALGORITHMS_KEY_SCHEDULE_SPDM; KEX_SET(LIBSPDM_DATA_KEY_SCHEDULE, n16);
  n8 = SPDM_ALGORITHMS_OPAQUE_DATA_FORMAT_1; KEX_SET(LIBSPDM_DATA_OTHER_PARAMS_SUPPORT, n8);
  s->chain_size = LIBSPDM_MAX_CERT_CHAIN_SIZE;
  if (libspdm_init_connection(spdm, false) != LIBSPDM_STATUS_SUCCESS ||
      libspdm_get_certificate_ex(spdm, NULL, 0, 0, &s->chain_size, s->chain,
        &anchor, &anchor_size) != LIBSPDM_STATUS_SUCCESS ||
      anchor == NULL || anchor_size == 0 ||
      s->chain_size <= sizeof(spdm_cert_chain_t) + 48 ||
      !kex_session(spdm) || teeio_fault_fire_count() != 0) {
    return false;
  }
  teeio_spdm_log_negotiated_version(spdm);
  /* New connection, no GET_CERTIFICATE on it. Pinned P384/SHA384 algorithms
   * avoid importing a chain hash computed under a different negotiation. */
  if (!kex_version_reset(spdm, s) || !kex_restore_cache(spdm, s)) {
    return false;
  }
  s->ready = true;
  return true;
}

void spdm_test_case_fault_key_exchange_run(void *test_context)
{
  teeio_spdm_test_context_t *test = test_context;
  kex_state_t *s;
  bool passed = false, recovery = false;
  if (test != NULL && test->spdm_context != NULL && test->test_scratch_buffer_size == sizeof(*s)) {
    s = (void *)test->test_scratch_buffer;
    if (s->magic == KEX_MAGIC && s->ready && teeio_fault_fire_count() == 0 &&
        teeio_fault_driver() == (s->duplicate ?
          TEEIO_FAULT_DRIVER_KEY_EXCHANGE_DUPLICATE :
          TEEIO_FAULT_DRIVER_KEY_EXCHANGE_CACHED_CERT)) {
      s->ready = false;
      s->target = true;
      passed = kex_session(test->spdm_context);
      s->target = false;
      passed = passed && s->kex_count == 1 && s->finish_count == 1 && s->cert_count == 0 &&
        (s->duplicate ? (s->sent_twice && s->distinct && teeio_fault_fire_count() == 1) :
                        teeio_fault_fire_count() == 0);
      /* Recover even when the duplicate fails. Never turn a failed target
       * exchange into a pass merely because a subsequent reconnect works. */
      recovery = kex_version_reset(test->spdm_context, s) &&
        kex_restore_cache(test->spdm_context, s) && kex_session(test->spdm_context);
      recovery = kex_version_reset(test->spdm_context, s) && recovery;
      passed = passed && recovery;
      if (passed && s->duplicate) {
        teeio_fault_record_actual("duplicate_unique_session_ids_then_recover");
      }
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "KEY_EXCHANGE %s: kex=%u cert=%u finish=%u fires=%u recovery=%u\n",
        s->duplicate ? "duplicate" : "positive cached-certificate control",
        s->kex_count, s->cert_count, s->finish_count, teeio_fault_fire_count(), recovery));
    }
  }
  teeio_record_assertion_result(SPDM_TEST_CASE_FAULT, teeio_fault_driver(), 1,
    IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
    passed ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
    "Authenticated KEY_EXCHANGE/FINISH, exact duplicate or cached-cert positive control; VERSION recovery=%u",
    recovery);
}

void spdm_test_case_fault_key_exchange_teardown(void *test_context)
{
  teeio_spdm_test_context_t *test = test_context;
  if (test != NULL && test->spdm_context != NULL && test->test_scratch_buffer_size == sizeof(kex_state_t)) {
    kex_state_t *s = (void *)test->test_scratch_buffer;
    libspdm_context_t *spdm = test->spdm_context;
    if (s->magic == KEX_MAGIC) {
      if (s->dirty && !kex_version_reset(spdm, s)) {
        teeio_record_assertion_result(SPDM_TEST_CASE_FAULT, teeio_fault_driver(), 2,
          IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED,
          "KEY_EXCHANGE teardown VERSION recovery failed; remote cleanup unconfirmed");
      }
      spdm->send_message = s->send;
      spdm->receive_message = s->receive;
      spdm->transport_encode_message = s->encode;
      spdm->transport_decode_message = s->decode;
      spdm->app_context_data_ptr = s->app;
      free(s->chain); free(s->wire); free(s->reply);
      libspdm_zero_mem(s, sizeof(*s));
    }
  }
  spdm_test_case_common_teardown(test_context);
}