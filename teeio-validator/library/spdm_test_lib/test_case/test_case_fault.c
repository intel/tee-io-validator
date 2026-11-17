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
#include "internal/libspdm_common_lib.h"

#define TEEIO_FAULT_SET_CERTIFICATE_ASSERTION_ID 1

typedef struct {
  uint8_t version;
  uint8_t heartbeat_period;
  uint8_t reserved[2];
  uint32_t session_id;
  libspdm_transport_encode_message_func saved_encode;
  libspdm_transport_decode_message_func saved_decode;
  void *saved_app;
  uint8_t *frame;
  bool installed;
  bool armed;
  bool abandon;
  bool missing;
  bool oversized;
  bool partial_response;
  bool selected;
  bool invalid;
  bool first_early_invalid_request;
  bool terminal_certificate_rejection;
  bool last_send_final;
  uint8_t handle;
  uint32_t last_sequence;
  uint32_t next_get_sequence;
  uint32_t send_count;
  uint32_t ack_count;
  uint32_t data_transfer_size;
  size_t certificate_target;
  size_t certificate_request_size;
  size_t certificate_sent;
  uint8_t certificate_original;
  bool certificate_target_valid;
  bool backend_probe;
  bool backend_rejection;
} teeio_fault_test_buffer_t;

void spdm_test_case_common_teardown(void *test_context);
bool spdm_test_case_fault_key_exchange_setup(void *test_context);
void spdm_test_case_fault_key_exchange_run(void *test_context);
void spdm_test_case_fault_key_exchange_teardown(void *test_context);
bool spdm_test_case_fault_mut_auth_setup(void *test_context);
void spdm_test_case_fault_mut_auth_run(void *test_context);

static bool is_key_exchange_scenario(void)
{
  return teeio_fault_driver() == TEEIO_FAULT_DRIVER_KEY_EXCHANGE_DUPLICATE ||
         teeio_fault_driver() == TEEIO_FAULT_DRIVER_KEY_EXCHANGE_CACHED_CERT;
}

static bool is_set_certificate_scenario(void)
{
  return teeio_fault_driver() >= TEEIO_FAULT_DRIVER_CERT_SIGNATURE_CHUNK &&
         teeio_fault_driver() <= TEEIO_FAULT_DRIVER_CHUNK_LAST_WITHHELD;
}

/* An ML-DSA SET_CERTIFICATE exceeds a typical MaxSPDMmsgSize. In PQC the
 * transfer-level drivers use the chunked ML-KEM KEY_EXCHANGE instead. */
static bool is_key_exchange_chunk_target(void)
{
  return teeio_spdm_pqc_profile() &&
         (teeio_fault_driver() == TEEIO_FAULT_DRIVER_CHUNK_OVERSIZED ||
          teeio_fault_driver() == TEEIO_FAULT_DRIVER_CHUNK_LAST_WITHHELD);
}

/* The chunk drivers stay here; key-exchange and mutual-auth entry points are
 * deliberately separate. Do not fold their setup/run/teardown into this state. */

static uint32_t chunk_sequence(const uint8_t *message)
{
  return message[0] >= SPDM_MESSAGE_VERSION_14 ?
    libspdm_read_uint32(message + 4) : libspdm_read_uint16(message + 4);
}

static void chunk_assert(uint32_t id, bool passed, const char *description)
{
  teeio_record_assertion_result(
    SPDM_TEST_CASE_FAULT, teeio_fault_driver(), id,
    IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
    passed ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
    "%s", description);
}

/* Strict, bounded DER TLV reader: no indefinite/nonminimal lengths, overflow,
 * trailing data, or speculative reads. Only the tags used below are accepted. */
static bool certificate_der(const uint8_t **cursor, const uint8_t *end,
                            uint8_t tag, const uint8_t **value, size_t *length)
{
  const uint8_t *p = *cursor;
  size_t n, size;
  if (end - p < 2 || *p++ != tag) return false;
  size = *p++;
  if (size & 0x80) {
    n = size & 0x7f;
    if (n == 0 || n > sizeof(size_t) || (size_t)(end - p) < n || *p == 0)
      return false;
    size = 0;
    while (n--) size = (size << 8) | *p++;
    if (size < 128) return false;
  }
  if (size > (size_t)(end - p)) return false;
  *value = p;
  *length = size;
  *cursor = p + size;
  return true;
}

static bool certificate_signature_target(teeio_fault_test_buffer_t *state,
                                         const uint8_t *chain, size_t size,
                                         uint32_t hash)
{
  const uint8_t *cert, *p, *end, *value, *signature, *integer;
  uint8_t constraints[64];
  size_t constraints_size = sizeof(constraints);
  const uint8_t *second = NULL;
  size_t second_size = 0;
  unsigned count = 0;
  size_t cert_size, length, signature_size, integer_size;
  size_t header = sizeof(spdm_cert_chain_t) + libspdm_get_hash_size(hash);
  state->certificate_target_valid = false;
  if (chain == NULL || size <= header || size > LIBSPDM_MAX_CERT_CHAIN_SIZE ||
      libspdm_read_uint32(chain) != size) return false;
  /* Bound the entire outer certificate list BEFORE calling the platform's
   * X.509 walker; do not rely on backend-specific malformed-DER behavior. */
  p = chain + header;
  end = chain + size;
  while (p < end) {
    const uint8_t *start = p;
    if (!certificate_der(&p, end, 0x30, &value, &length) || length == 0) return false;
    if (count++ == 1) { second = start; second_size = (size_t)(p - start); }
  }
  if (second == NULL || !libspdm_x509_get_cert_from_cert_chain(
        chain + header, size - header, 1, &cert, &cert_size)) return false;
  if (cert != second || cert_size != second_size) return false;
    /* Index 1 must actually be a CA, not an end-entity certificate on a shorter
     * platform chain. Require canonical BasicConstraints cA=TRUE. */
    if (!libspdm_x509_get_extended_basic_constraints(cert, cert_size,
      constraints, &constraints_size) || constraints_size > sizeof(constraints)) return false;
    p = constraints;
    end = constraints + constraints_size;
    if (!certificate_der(&p, end, 0x30, &value, &length) || p != end) return false;
    p = value;
    if (!certificate_der(&p, end, 0x01, &value, &length) ||
    length != 1 || value[0] != 0xff) return false;
  p = cert;
  end = cert + cert_size;
  if (!certificate_der(&p, end, 0x30, &value, &length) || p != end) return false;
  p = value;
  if (!certificate_der(&p, end, 0x30, &value, &length) || /* TBSCertificate */
      !certificate_der(&p, end, 0x30, &value, &length) || /* signatureAlgorithm */
      !certificate_der(&p, end, 0x03, &signature, &signature_size) || p != end ||
      signature_size < 2 || signature[0] != 0) return false;
  /* Negotiation pins ECDSA-P384. Keep the BIT STRING and both DER INTEGER
   * encodings intact: flip a low-order r byte, never its sign/length byte. */
  p = signature + 1;
  if (!certificate_der(&p, end, 0x30, &value, &length) || p != end) return false;
  p = value;
  if (!certificate_der(&p, end, 0x02, &integer, &integer_size) ||
      integer_size < 2 || (integer[0] & 0x80) ||
      (integer[0] == 0 && !(integer[1] & 0x80))) return false;
  state->certificate_target = sizeof(spdm_set_certificate_request_t) +
    (size_t)(integer + integer_size - 1 - chain);
  state->certificate_original = integer[integer_size - 1];
  if (!certificate_der(&p, end, 0x02, &integer, &integer_size) || p != end ||
      integer_size == 0 || (integer[0] & 0x80) ||
      (integer_size > 1 && integer[0] == 0 && !(integer[1] & 0x80))) return false;
  state->certificate_request_size = sizeof(spdm_set_certificate_request_t) + size;
  state->certificate_sent = 0;
  state->certificate_target_valid = true;
  return true;
}

/* Returns -1 on invalid transfer, 0 for no target, or 1 for a target chunk. */
static int select_chunk_fault(teeio_fault_test_buffer_t *state,
                              const uint8_t *bytes, size_t size,
                              uint32_t *sequence, size_t *target_in_chunk)
{
  bool candidate = false;

  if (bytes[1] == SPDM_CHUNK_SEND && size >= sizeof(spdm_chunk_send_request_t)) {
    *sequence = chunk_sequence(bytes);
    state->send_count++;
    state->handle = bytes[3];
    state->last_sequence = *sequence;
    state->last_send_final =
      (bytes[2] & SPDM_CHUNK_SEND_REQUEST_ATTRIBUTE_LAST_CHUNK) != 0;
    if (state->missing) {
      candidate = (bytes[2] & SPDM_CHUNK_SEND_REQUEST_ATTRIBUTE_LAST_CHUNK) != 0;
      if (candidate && (*sequence == 0 || state->ack_count == 0)) {
        state->invalid = true;
      }
    } else if (state->oversized) {
      candidate = state->send_count == 1 && *sequence == 0;
    } else if (!state->abandon && !state->backend_probe) {
      size_t header = sizeof(spdm_chunk_send_request_t) + (*sequence == 0 ? 4 : 0);
      size_t payload = libspdm_read_uint32(bytes + 8);
      if (!state->certificate_target_valid || *sequence != state->send_count - 1 ||
          size < header || payload != size - header ||
          state->certificate_sent > state->certificate_request_size ||
          payload > state->certificate_request_size - state->certificate_sent ||
          (*sequence == 0 && (libspdm_read_uint32(bytes + 12) !=
            state->certificate_request_size || payload < 4 ||
            bytes[17] != SPDM_SET_CERTIFICATE)) ||
          state->last_send_final !=
            (payload == state->certificate_request_size - state->certificate_sent)) {
        state->invalid = true;
        return -1;
      }
      candidate = state->certificate_target >= state->certificate_sent &&
        state->certificate_target - state->certificate_sent < payload;
      if (candidate) {
        *target_in_chunk = header + state->certificate_target - state->certificate_sent;
        if (bytes[*target_in_chunk] != state->certificate_original) {
          state->invalid = true;
          return -1;
        }
      }
      state->certificate_sent += payload;
    }
  } else if (state->abandon && bytes[1] == SPDM_CHUNK_GET &&
             size >= sizeof(spdm_chunk_get_request_t)) {
      /* A validated non-final CHUNK_RESPONSE, not an absolute occurrence,
       * proves an actual partial GET_CERTIFICATE transfer is active. */
      candidate = state->partial_response && bytes[3] == state->handle &&
        chunk_sequence(bytes) == state->next_get_sequence;
  }
  return candidate;
}

static libspdm_return_t encode_corrupted_signature(
  teeio_fault_test_buffer_t *state, void *spdm, const uint32_t *session_id,
  bool app, bool request, size_t size, void *message,
  size_t *transport_size, void **transport,
  uint32_t sequence, size_t target_in_chunk)
{
  const uint8_t *bytes = message;
  teeio_fault_result_t result;
  /* Candidate-slice FI framing: real CHUNK_SEND selector plus exactly the
    * DER-selected byte. offset=12 addresses this slice, NOT a wire offset.
    * FI owns the XOR and audit; copy back only its one verified changed bit. */
  uint8_t slice[13] = {0};
  slice[2] = TEEIO_FAULT_DOE_TYPE_PLAIN_SPDM;
  libspdm_write_uint32(slice + 4, 4);
  memcpy(slice + 8, bytes, 4);
  slice[12] = bytes[target_in_chunk];
  result = teeio_fault_apply(slice, sizeof(slice), sizeof(slice));
  if (size + 8 > TEEIO_FAULT_MAX_MESSAGE_SIZE ||
      result.disposition != TEEIO_FAULT_DISPOSITION_MUTATE ||
      result.message_size != sizeof(slice) ||
      memcmp(result.message, slice, 12) != 0 ||
      (result.message[12] ^ slice[12]) != 1 || teeio_fault_fire_count() != 1) {
    state->invalid = true;
    return LIBSPDM_STATUS_SEND_FAIL;
  }
  memcpy(state->frame + 8, message, size);
  state->frame[8 + target_in_chunk] = result.message[12];
  state->selected = true;
  TEEIO_DEBUG((TEEIO_DEBUG_INFO,
    "Certificate signature target: non-root cert index=1 ECDSA r LSB request_offset=%zu chunk=%u wire_offset=%zu bits=1\n",
    state->certificate_target, sequence, target_in_chunk + 8));
  return libspdm_transport_pci_doe_encode_message(spdm, session_id, app,
    request, size, state->frame + 8, transport_size, transport);
}

static bool extend_chunk_from_request(teeio_fault_test_buffer_t *state,
                                      void *spdm, const uint8_t *bytes,
                                      size_t size)
{
  libspdm_chunk_info_t *send = &((libspdm_context_t *)spdm)->chunk_context.send;
  size_t header = sizeof(spdm_chunk_send_request_t) + sizeof(uint32_t);
  uint32_t payload = libspdm_read_uint32(bytes + 8);
  if (size < header || payload != size - header ||
      !send->chunk_in_use || send->large_message == NULL ||
      send->large_message_size > send->large_message_capacity ||
      send->large_message_size != libspdm_read_uint32(bytes + 12) ||
      send->large_message_size <= (size_t)payload + 8 ||
      send->chunk_bytes_transferred != payload ||
      memcmp(bytes + header, send->large_message, payload) != 0) {
    state->invalid = true;
    return false;
  }
  /* Extend the declared chunk with its real continuation, not transport junk. */
  memcpy(state->frame + 8 + size,
         (const uint8_t *)send->large_message + payload, 8);
  libspdm_write_uint32(state->frame + 8 + 8, payload + 8);
  send->chunk_bytes_transferred += 8;
  return true;
}

static bool apply_chunk_fault(teeio_fault_test_buffer_t *state, void *spdm,
                              uint32_t sequence, size_t *size, void **message)
{
  teeio_fault_result_t result;
  const uint8_t *bytes = *message;

  if (*size + 8 + 4 > TEEIO_FAULT_MAX_MESSAGE_SIZE) {
    state->invalid = true;
    return false;
  }
  memset(state->frame, 0, 8);
  state->frame[2] = TEEIO_FAULT_DOE_TYPE_PLAIN_SPDM;
  libspdm_write_uint32(state->frame + 4, (uint32_t)((*size + 11) / 4));
  memcpy(state->frame + 8, *message, *size);
  result = teeio_fault_apply(state->frame, *size + 8,
                             TEEIO_FAULT_MAX_MESSAGE_SIZE - 3);
  if (result.disposition == TEEIO_FAULT_DISPOSITION_PASS) {
    return true;
  }
  state->selected = true;
  if (state->abandon || state->missing) {
    if (result.disposition != (state->abandon ?
          TEEIO_FAULT_DISPOSITION_ABANDON : TEEIO_FAULT_DISPOSITION_DROP)) {
      state->invalid = true;
    }
    teeio_fault_record_actual(state->abandon ?
      "chunk_transfer_abandoned" : "final_chunk_withheld");
    /* Fail the send locally: no hardware transmission or receive timeout,
     * and no attempt to send a non-chunk request before GET_VERSION. */
    return false;
  }
  if (result.disposition != TEEIO_FAULT_DISPOSITION_MUTATE ||
      result.message_size < 8 ||
      result.message_size > TEEIO_FAULT_MAX_MESSAGE_SIZE - 3) {
    state->invalid = true;
    return false;
  }
  if (sequence != 0 || *size != state->data_transfer_size ||
      result.message_size != *size + 16 ||
      result.message_size - 8 <= state->data_transfer_size ||
      memcmp(result.message + 8, *message, *size) != 0) {
    /* Fail closed if the configured mutation does not exceed the peer's
     * actual receive limit (the local limit may be smaller). */
    state->invalid = true;
    return false;
  }
  memcpy(state->frame, result.message, result.message_size);
  if (!extend_chunk_from_request(state, spdm, bytes, *size)) {
    return false;
  }
  *message = state->frame + 8;
  *size = result.message_size - 8;
  return true;
}

/* Scope plaintext FI to the operation under test, not setup, positive control,
 * or recovery. A per-context frame provides DOE headroom and padding even when
 * the encoder changes the returned buffer pointer (libspdm's zero-copy API).
 * No FI reset/re-init: those APIs erase the original observed outcome. */
static libspdm_return_t chunk_encode(
  void *spdm, const uint32_t *session_id, bool app, bool request,
  size_t size, void *message, size_t *transport_size, void **transport)
{
  teeio_fault_test_buffer_t *state =
    ((libspdm_context_t *)spdm)->app_context_data_ptr;
  int candidate = 0;
  uint32_t sequence = 0;
  size_t target_in_chunk = 0;
  libspdm_return_t status;

  if ((state->armed || state->backend_probe) && !app && session_id == NULL && size >= 4) {
    candidate = select_chunk_fault(state, message, size, &sequence, &target_in_chunk);
    if (candidate < 0) return LIBSPDM_STATUS_SEND_FAIL;
  }
  if (candidate && !state->selected) {
    if (!state->abandon && !state->missing && !state->oversized) {
      return encode_corrupted_signature(state, spdm, session_id, app, request,
        size, message, transport_size, transport, sequence, target_in_chunk);
    }
    if (!apply_chunk_fault(state, spdm, sequence, &size, &message)) {
      return LIBSPDM_STATUS_SEND_FAIL;
    }
  }
  if (state->armed && state->oversized && state->selected) {
    /* The normal libspdm path passes the negotiated DataTransferSize as the
     * output capacity. The oversized driver deliberately exceeds that logical limit, so expose
     * the larger physical DOE buffer to the encoder only for this mutation. */
    *transport_size = LIBSPDM_RECEIVER_BUFFER_SIZE;
  }
  status = libspdm_transport_pci_doe_encode_message(
    spdm, session_id, app, request, size, message, transport_size, transport);
  if (state->armed && state->oversized && state->selected &&
      !LIBSPDM_STATUS_IS_ERROR(status) &&
      size > state->data_transfer_size) {
    TEEIO_DEBUG((TEEIO_DEBUG_WARN,
      "Oversized chunk encode: plain=%zu transport=%zu doe_dw=%u doe_type=%u session=%u app=%u limit=%u chunk_size=%u\n",
      size, *transport_size,
      libspdm_read_uint32((const uint8_t *)*transport + 4) & 0x3ffff,
      ((const uint8_t *)*transport)[2], session_id != NULL, app,
      state->data_transfer_size,
      ((const spdm_chunk_send_request_t *)message)->chunk_size));
  }
  if (state->armed && state->oversized && state->selected &&
      !LIBSPDM_STATUS_IS_ERROR(status) &&
      (*transport_size < size + 8 ||
       (libspdm_read_uint32((uint8_t *)*transport + 4) & 0x3ffff) * 4 !=
         *transport_size)) {
    state->invalid = true;
    return LIBSPDM_STATUS_SEND_FAIL;
  }
  return status;
}

static libspdm_return_t chunk_decode(
  void *spdm, uint32_t **session_id, bool *app, bool request,
  size_t transport_size, void *transport, size_t *size, void **message)
{
  teeio_fault_test_buffer_t *state =
    ((libspdm_context_t *)spdm)->app_context_data_ptr;
  libspdm_return_t status;
  const uint8_t *bytes;
  size_t header_size;
  bool terminal_error_size;

  /* Keep the existing observer: its first recorded actual must survive all
   * recovery traffic. Inspect the same unmodified decoded response below. */
  status = state->saved_decode(spdm, session_id, app, request,
                               transport_size, transport, size, message);
  if (LIBSPDM_STATUS_IS_ERROR(status) || *app ||
      (!state->armed && !state->backend_probe) || *size < 4) {
    return status;
  }
  bytes = *message;
  if (bytes[0] != state->version) {
    state->invalid = true;
    return status;
  }
  if (state->abandon && bytes[1] == SPDM_CHUNK_RESPONSE &&
      *size >= sizeof(spdm_chunk_response_response_t) + 4 &&
      chunk_sequence(bytes) == 0 &&
      !(bytes[2] & SPDM_CHUNK_GET_RESPONSE_ATTRIBUTE_LAST_CHUNK)) {
    uint32_t payload = libspdm_read_uint32(bytes + 8);
    uint32_t total = libspdm_read_uint32(bytes + 12);
    /* First response must actually carry part of CERTIFICATE, not a chunk
     * from GET_DIGESTS, setup, or session establishment. */
    state->partial_response = payload >= 4 && payload < total &&
      payload <= *size - 16 && bytes[17] == SPDM_CERTIFICATE;
    if (state->partial_response) {
      state->handle = bytes[3];
      state->next_get_sequence = chunk_sequence(bytes) + 1;
    }
  }
  /* DSP0274 1.4.1 par. 819: an invalid CHUNK_SEND may get a bare ERROR. */
  if (state->oversized && state->selected && state->send_count == 1 &&
      state->ack_count == 0 && bytes[1] == SPDM_ERROR &&
      bytes[2] == SPDM_ERROR_CODE_INVALID_REQUEST && bytes[3] == 0) {
    state->first_early_invalid_request = true;
    return status;
  }
  if (bytes[1] != SPDM_CHUNK_SEND_ACK) {
    return status;
  }
  state->ack_count++;
  header_size = bytes[0] >= SPDM_MESSAGE_VERSION_14 ?
    sizeof(spdm_chunk_send_ack_response_14_t) : sizeof(spdm_chunk_send_ack_response_t);
  if (*size < header_size || bytes[3] != state->handle ||
      chunk_sequence(bytes) != state->last_sequence) {
    state->invalid = true;
    return status;
  }
  if (state->oversized && state->selected && state->ack_count == 1 &&
      state->send_count == 1 && chunk_sequence(bytes) == 0 &&
      bytes[2] == SPDM_CHUNK_SEND_ACK_RESPONSE_ATTRIBUTE_EARLY_ERROR_DETECTED &&
      *size >= header_size + sizeof(spdm_error_response_t) &&
      bytes[header_size] == bytes[0] && bytes[header_size + 1] == SPDM_ERROR &&
      bytes[header_size + 2] == SPDM_ERROR_CODE_INVALID_REQUEST &&
      bytes[header_size + 3] == 0) {
    state->first_early_invalid_request = true;
  }
  if (state->oversized && state->selected && state->ack_count == 1 &&
      !state->first_early_invalid_request) {
    teeio_fault_record_actual("first_chunk_not_early_invalid_request");
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR,
      "Oversized chunk: first ACK is not early InvalidRequest; stopping before further chunks\n"));
    return LIBSPDM_STATUS_ERROR_PEER;
  }
  /* DOE exposes up to three zero alignment bytes (notably 1.2's six-byte ACK). */
  terminal_error_size = *size >= header_size + sizeof(spdm_error_response_t) &&
    *size <= ((header_size + sizeof(spdm_error_response_t) + 3) & ~(size_t)3);
  if (terminal_error_size) {
    size_t i;
    for (i = header_size + sizeof(spdm_error_response_t); i < *size; i++)
      terminal_error_size &= bytes[i] == 0;
  }
  if (state->last_send_final && bytes[2] == 0 && terminal_error_size &&
      bytes[header_size] == state->version && bytes[header_size + 3] == 0 &&
      bytes[header_size + 1] == SPDM_ERROR &&
      bytes[header_size + 2] == SPDM_ERROR_CODE_OPERATION_FAILED) {
    state->backend_rejection = true;
  }
  if (!state->oversized && state->selected && state->last_send_final &&
      bytes[2] == 0 && terminal_error_size &&
      bytes[header_size] == state->version && bytes[header_size + 3] == 0 &&
      bytes[header_size + 1] == SPDM_ERROR &&
      bytes[header_size + 2] == SPDM_ERROR_CODE_UNSPECIFIED) {
    state->terminal_certificate_rejection = true;
  }
  return status;
}

static bool chunk_install(teeio_spdm_test_context_t *context)
{
  libspdm_context_t *spdm = context->spdm_context;
  teeio_fault_test_buffer_t *state = (void *)context->test_scratch_buffer;

  if (sizeof(*state) > sizeof(context->test_scratch_buffer) ||
      spdm->transport_encode_message != teeio_fault_transport_encode_message ||
      spdm->transport_decode_message != teeio_fault_transport_decode_message) {
    return false;
  }
  memset(state, 0, sizeof(*state));
  context->test_scratch_buffer_size = sizeof(*state);
  state->frame = malloc(TEEIO_FAULT_MAX_MESSAGE_SIZE);
  if (state->frame == NULL) {
    return false;
  }
  state->saved_encode = spdm->transport_encode_message;
  state->saved_decode = spdm->transport_decode_message;
  state->saved_app = spdm->app_context_data_ptr;
  state->abandon = teeio_fault_driver() == TEEIO_FAULT_DRIVER_CHUNK_ABANDON;
  state->missing = teeio_fault_driver() == TEEIO_FAULT_DRIVER_CHUNK_LAST_WITHHELD;
  state->oversized = teeio_fault_driver() == TEEIO_FAULT_DRIVER_CHUNK_OVERSIZED;
  state->installed = true;
  spdm->app_context_data_ptr = state;
  spdm->transport_encode_message = chunk_encode;
  spdm->transport_decode_message = chunk_decode;
  return true;
}

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
  if (is_key_exchange_scenario()) {
    return spdm_test_case_fault_key_exchange_setup(test_context);
  }
  if (teeio_fault_driver() == TEEIO_FAULT_DRIVER_FINISH_SIGNATURE) {
    return spdm_test_case_fault_mut_auth_setup(test_context);
  }

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

  if (!is_set_certificate_scenario() ||
      teeio_fault_same_session_recovery_configured() ||
      !chunk_install(context) ||
      !teeio_spdm_apply_version_override(context->spdm_context)) {
    return false;
  }
  if (teeio_spdm_pqc_profile() && teeio_fault_driver() != TEEIO_FAULT_DRIVER_CHUNK_ABANDON &&
      !is_key_exchange_chunk_target()) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR,
      "SET_CERTIFICATE signature driver supports only the classical profile (ML-DSA chain exceeds MaxSPDMmsgSize)\n"));
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
                      &data8, sizeof(data8)) ||
      !teeio_spdm_apply_algorithm_profile(context->spdm_context)) {
    return false;
  }

  status = libspdm_init_connection(context->spdm_context, false);
  if (LIBSPDM_STATUS_IS_ERROR(status) ||
      !teeio_spdm_negotiated_profile_matches(context->spdm_context)) {
    return false;
  }
  teeio_spdm_log_negotiated_version(context->spdm_context);

  capabilities = 0;
  if (!get_connection_data(context->spdm_context,
                           LIBSPDM_DATA_CAPABILITY_FLAGS,
                           &capabilities, sizeof(capabilities)) ||
      (teeio_fault_driver() != TEEIO_FAULT_DRIVER_CHUNK_ABANDON &&
       !is_key_exchange_chunk_target() &&
       (capabilities & SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_SET_CERT_CAP) == 0) ||
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
  /* The KEY_EXCHANGE target itself establishes the session in run. */
  status = is_key_exchange_chunk_target() ? LIBSPDM_STATUS_SUCCESS : libspdm_start_session(
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
  /* A raw-DOE/mis-scoped config must not corrupt setup unnoticed. */
  return !teeio_fault_scenario_fired();
}

static bool chunk_recover(teeio_spdm_test_context_t *context)
{
  teeio_fault_test_buffer_t *state = (void *)context->test_scratch_buffer;
  libspdm_context_t *spdm = context->spdm_context;
  libspdm_return_t status;
  uint8_t *chain;
  size_t chain_size;
  size_t index;
  bool old_sessions_gone = true;

  state->armed = false;
  /* No END_SESSION/HEARTBEAT on the old session while a transfer is active.
   * GET_VERSION is the sole legal interrupt; init_connection uses the SAME
   * allocated context and resets its sessions before negotiating anew. */
  state->session_id = 0;
  status = libspdm_init_connection(spdm, false);
  chunk_assert(10, !LIBSPDM_STATUS_IS_ERROR(status),
               "GET_VERSION abort and fresh connection on the same context");
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    return false;
  }
  for (index = 0; index < LIBSPDM_MAX_SESSION_COUNT; index++) {
    old_sessions_gone &= spdm->session_info[index].session_id == INVALID_SESSION_ID;
  }
  chunk_assert(11, old_sessions_gone,
               "no old local session survives GET_VERSION reset");
  if (!old_sessions_gone) {
    return false;
  }
  chain = malloc(LIBSPDM_MAX_CERT_CHAIN_SIZE);
  if (chain == NULL) {
    chunk_assert(12, false, "recovery certificate allocation");
    return false;
  }
  chain_size = LIBSPDM_MAX_CERT_CHAIN_SIZE;
  status = libspdm_get_certificate(spdm, NULL, 0, &chain_size, chain);
  free(chain);
  chunk_assert(12, !LIBSPDM_STATUS_IS_ERROR(status),
               "complete certificate refetch after abandoned transfer");
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    return false;
  }
  status = libspdm_start_session(
    spdm, false, NULL, 0, SPDM_KEY_EXCHANGE_REQUEST_NO_MEASUREMENT_SUMMARY_HASH, 0,
    SPDM_KEY_EXCHANGE_REQUEST_SESSION_POLICY_TERMINATION_POLICY_RUNTIME_UPDATE,
    &state->session_id, &state->heartbeat_period, NULL);
  chunk_assert(13, !LIBSPDM_STATUS_IS_ERROR(status),
               "fresh authenticated session after connection reset");
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    state->session_id = 0;
    return false;
  }
  status = libspdm_heartbeat(spdm, state->session_id);
  chunk_assert(14, !LIBSPDM_STATUS_IS_ERROR(status), "fresh-session HEARTBEAT");
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    return false;
  }
  status = libspdm_stop_session(spdm, state->session_id, 0);
  chunk_assert(15, !LIBSPDM_STATUS_IS_ERROR(status), "fresh-session END_SESSION");
  state->session_id = 0;
  return !LIBSPDM_STATUS_IS_ERROR(status);
}

void spdm_test_case_fault_run(void *test_context)
{
  if (is_key_exchange_scenario()) {
    spdm_test_case_fault_key_exchange_run(test_context);
    return;
  }
  if (teeio_fault_driver() == TEEIO_FAULT_DRIVER_FINISH_SIGNATURE) {
    spdm_test_case_fault_mut_auth_run(test_context);
    return;
  }

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
      (test_buffer->session_id == 0 && !is_key_exchange_chunk_target())) {
    teeio_record_assertion_result(
      SPDM_TEST_CASE_FAULT, teeio_fault_driver(),
      TEEIO_FAULT_SET_CERTIFICATE_ASSERTION_ID,
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
      SPDM_TEST_CASE_FAULT, teeio_fault_driver(),
      TEEIO_FAULT_SET_CERTIFICATE_ASSERTION_ID,
      IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED,
      "failed to read negotiated SPDM data");
    return;
  }

  /* Setup already fetched the chain; a second fetch overflows message_b with ML-DSA chains. */
  if (is_key_exchange_chunk_target()) {
    test_buffer->data_transfer_size = data_transfer_size;
    test_buffer->armed = true;
    status = libspdm_start_session(
      context->spdm_context, false, NULL, 0,
      SPDM_KEY_EXCHANGE_REQUEST_NO_MEASUREMENT_SUMMARY_HASH, 0,
      SPDM_KEY_EXCHANGE_REQUEST_SESSION_POLICY_TERMINATION_POLICY_RUNTIME_UPDATE,
      &test_buffer->session_id, &test_buffer->heartbeat_period, NULL);
    if (!LIBSPDM_STATUS_IS_ERROR(status)) {
      libspdm_stop_session(context->spdm_context, test_buffer->session_id, 0);
      test_buffer->session_id = 0;
    }
    if (test_buffer->oversized) {
      chunk_assert(3, test_buffer->first_early_invalid_request &&
                     !test_buffer->invalid && test_buffer->send_count == 1 &&
                     test_buffer->ack_count <= 1,
        "FIRST response to oversized KEY_EXCHANGE CHUNK_SEND: InvalidRequest 0x01, bare or in EARLY_ERROR_DETECTED ACK");
    } else {
      chunk_assert(3, test_buffer->selected && test_buffer->last_send_final &&
                     test_buffer->ack_count > 0 && !test_buffer->invalid,
        "LAST_CHUNK withheld only after an acknowledged partial KEY_EXCHANGE transfer");
    }
    teeio_record_assertion_result(
      SPDM_TEST_CASE_FAULT, teeio_fault_driver(),
      TEEIO_FAULT_SET_CERTIFICATE_ASSERTION_ID,
      IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
      test_buffer->selected && !test_buffer->invalid &&
      teeio_fault_fire_count() == 1 && LIBSPDM_STATUS_IS_ERROR(status) &&
      (test_buffer->oversized ? test_buffer->first_early_invalid_request :
       test_buffer->ack_count > 0) ?
        TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
      "corrupted chunked KEY_EXCHANGE status - 0x%x", (uint32_t)status);
    teeio_fault_record_actual("chunk_driver_no_expected_response");
    chunk_recover(context);
    return;
  }

  cert_chain = malloc(LIBSPDM_MAX_CERT_CHAIN_SIZE);
  if (cert_chain == NULL) {
    teeio_record_assertion_result(
      SPDM_TEST_CASE_FAULT, teeio_fault_driver(),
      TEEIO_FAULT_SET_CERTIFICATE_ASSERTION_ID,
      IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED,
      "failed to allocate certificate chain");
    return;
  }
  cert_chain_size = LIBSPDM_MAX_CERT_CHAIN_SIZE;
  status = libspdm_get_certificate(context->spdm_context, NULL, 0,
                                   &cert_chain_size, cert_chain);
  if (test_buffer->abandon) {
    /* Setup and this prefetch are FI-free. Now start an actual new transfer,
     * abandon only after its first non-final CERTIFICATE chunk, and recover. */
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
      free(cert_chain);
      chunk_assert(1, false, "GET_CERTIFICATE positive control failed");
      return;
    }
    test_buffer->armed = true;
    cert_chain_size = LIBSPDM_MAX_CERT_CHAIN_SIZE;
    status = libspdm_get_certificate(context->spdm_context, NULL, 0,
                                     &cert_chain_size, cert_chain);
    free(cert_chain);
    chunk_assert(1, test_buffer->partial_response && test_buffer->selected &&
      !test_buffer->invalid && teeio_fault_fire_count() == 1 &&
      LIBSPDM_STATUS_IS_ERROR(status),
      "partial GET_CERTIFICATE received, next CHUNK_GET deliberately abandoned");
    /* Freeze even an unexpected/no-response outcome before recovery. */
    teeio_fault_record_actual("abandon_driver_failed");
    chunk_recover(context);
    return;
  }
  if (LIBSPDM_STATUS_IS_ERROR(status) ||
      ((capabilities & SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_ALIAS_CERT_CAP) != 0 &&
       !select_alias_immutable_chain(cert_chain, &cert_chain_size,
                                     base_hash_algo)) ||
      cert_chain_size + sizeof(spdm_set_certificate_request_t) <=
        data_transfer_size) {
    free(cert_chain);
    teeio_record_assertion_result(
      SPDM_TEST_CASE_FAULT, teeio_fault_driver(),
      TEEIO_FAULT_SET_CERTIFICATE_ASSERTION_ID,
      IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED,
      "certificate chain does not trigger CHUNK_SEND");
    return;
  }

  test_buffer->data_transfer_size = data_transfer_size;
  if (!test_buffer->missing && !test_buffer->oversized) {
    /* UNSPECIFIED (0x05) alone is ambiguous when the responder's update
     * callback is a stub. Require the identical unmodified chain to succeed.
     * Do not fabricate a positive or accept UNSUPPORTED_REQUEST here. */
    status = libspdm_set_certificate(context->spdm_context, NULL, 0,
                                     cert_chain, cert_chain_size);
    chunk_assert(2, status == LIBSPDM_STATUS_SUCCESS,
      "Signature provenance: unmodified SET_CERTIFICATE must succeed; a rejecting/stub callback leaves the test unresolved");
    if (status != LIBSPDM_STATUS_SUCCESS) {
      free(cert_chain);
      chunk_recover(context);
      return;
    }
    /* The parser does not check RootHash against DER. A hash-only mismatch
     * therefore exercises the identity backend, without invalidating X.509.
     * Require distinct wire provenance before trusting a later 0x05. */
    test_buffer->backend_probe = true;
    ((uint8_t *)cert_chain)[sizeof(spdm_cert_chain_t)] ^= 1;
    status = libspdm_set_certificate(context->spdm_context, NULL, 0,
                                     cert_chain, cert_chain_size);
    ((uint8_t *)cert_chain)[sizeof(spdm_cert_chain_t)] ^= 1;
    test_buffer->backend_probe = false;
    chunk_assert(4, LIBSPDM_STATUS_IS_ERROR(status) && test_buffer->backend_rejection,
      "Signature provenance: backend-only rejection must be OperationFailed 0x44, not ambiguous 0x05");
    if (!LIBSPDM_STATUS_IS_ERROR(status) || !test_buffer->backend_rejection ||
        !certificate_signature_target(test_buffer, cert_chain, cert_chain_size, base_hash_algo)) {
      chunk_assert(5, false, "Signature flip requires backend provenance and a parsed non-root ECDSA signature");
      free(cert_chain);
      chunk_recover(context);
      return;
    }
    test_buffer->send_count = test_buffer->ack_count = 0;
    test_buffer->backend_rejection = false;
  }
  test_buffer->armed = true;
  status = libspdm_set_certificate(
    context->spdm_context, NULL, 0,
    cert_chain, cert_chain_size);
  free(cert_chain);

  if (test_buffer->oversized) {
    chunk_assert(3, test_buffer->first_early_invalid_request &&
                   !test_buffer->invalid && test_buffer->send_count == 1 &&
                   test_buffer->ack_count <= 1,
      "FIRST response to oversized CHUNK_SEND: InvalidRequest 0x01, bare or in EARLY_ERROR_DETECTED ACK (not terminal SET_CERTIFICATE 0x05)");
  } else if (test_buffer->missing) {
    chunk_assert(3, test_buffer->selected && test_buffer->last_send_final &&
                   test_buffer->ack_count > 0 && !test_buffer->invalid,
      "LAST_CHUNK withheld only after an acknowledged partial SET_CERTIFICATE transfer");
  }
  teeio_record_assertion_result(
    SPDM_TEST_CASE_FAULT, teeio_fault_driver(),
    TEEIO_FAULT_SET_CERTIFICATE_ASSERTION_ID,
    IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
    test_buffer->selected && !test_buffer->invalid &&
    teeio_fault_fire_count() == 1 && LIBSPDM_STATUS_IS_ERROR(status) &&
    (test_buffer->oversized ? test_buffer->first_early_invalid_request :
     test_buffer->missing ? test_buffer->ack_count > 0 :
    (test_buffer->terminal_certificate_rejection && !test_buffer->backend_rejection)) ?
      TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
    "corrupted SET_CERTIFICATE status - 0x%x", (uint32_t)status);
  teeio_fault_record_actual("chunk_driver_no_expected_response");
  chunk_recover(context);
}

void spdm_test_case_fault_teardown(void *test_context)
{
  if (is_key_exchange_scenario()) {
    spdm_test_case_fault_key_exchange_teardown(test_context);
    return;
  }
  if (teeio_fault_driver() == TEEIO_FAULT_DRIVER_FINISH_SIGNATURE) {
    /* The mut-auth driver restores its callbacks within each operation.
     * Its scratch is NOT a chunk driver state, including on setup failure. */
    spdm_test_case_common_teardown(test_context);
    return;
  }

  teeio_spdm_test_context_t *context = test_context;
  teeio_fault_test_buffer_t *test_buffer;

  test_buffer = (void *)context->test_scratch_buffer;
  if (context->test_scratch_buffer_size == sizeof(*test_buffer) &&
      test_buffer->installed) {
    libspdm_context_t *spdm = context->spdm_context;
    test_buffer->armed = false;
    if (test_buffer->session_id != 0) {
      libspdm_stop_session(spdm, test_buffer->session_id, 0);
    }
    spdm->transport_encode_message = test_buffer->saved_encode;
    spdm->transport_decode_message = test_buffer->saved_decode;
    spdm->app_context_data_ptr = test_buffer->saved_app;
    free(test_buffer->frame);
    test_buffer->frame = NULL;
    test_buffer->installed = false;
  }
  spdm_test_case_common_teardown(test_context);
}
