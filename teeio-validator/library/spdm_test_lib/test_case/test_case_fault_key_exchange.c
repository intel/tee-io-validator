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
  bool ready, duplicate, target, pending, sent_twice, distinct, dirty, chunked;
  unsigned kex_count, cert_count, finish_count;
  uint16_t req_id;
  uint32_t first_id, second_id;
  uint8_t *chain, *wire, *reply, *io;
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

/* Exact unpadded KEY_EXCHANGE_RSP size for the negotiated algorithms: no
 * measurement summary or mutual authentication. 0 if the shape is invalid. */
static size_t kex_response_size(const libspdm_context_t *spdm, uint8_t version,
                                const uint8_t *b, size_t size)
{
  const libspdm_device_algorithm_t *a = &spdm->connection_info.algorithm;
  size_t exchange = a->kem_alg != 0 ? libspdm_get_kem_cipher_text_size(a->kem_alg) :
    libspdm_get_dhe_pub_key_size(a->dhe_named_group);
  size_t signature = a->pqc_asym_algo != 0 ?
    libspdm_get_pqc_asym_signature_size(a->pqc_asym_algo) :
    libspdm_get_asym_signature_size(a->base_asym_algo);
  size_t tail = signature + libspdm_get_hash_size(a->base_hash_algo);
  size_t opaque_offset = sizeof(spdm_key_exchange_response_t) + exchange;
  uint16_t opaque_size;
  if (exchange == 0 || signature == 0 || size < opaque_offset + sizeof(uint16_t) + tail ||
      b[0] != version || b[1] != SPDM_KEY_EXCHANGE_RSP ||
      b[3] != 0 || b[6] != 0 || b[7] != 0) {
    return 0;
  }
  opaque_size = libspdm_read_uint16(b + opaque_offset);
  if (opaque_size > SPDM_MAX_OPAQUE_DATA_SIZE) {
    return 0;
  }
  return opaque_offset + sizeof(uint16_t) + opaque_size + tail;
}

static bool kex_composed_id(const kex_state_t *s, const uint8_t *b, uint32_t *id)
{
  *id = libspdm_generate_session_id(s->req_id, libspdm_read_uint16(b + 4));
  return *id != INVALID_SESSION_ID && (*id >> 16) != 0;
}

static size_t kex_doe_frame(uint8_t *frame, size_t size)
{
  size_t framed = KEX_DOE + ((size + 3) & ~(size_t)3);
  libspdm_zero_mem(frame, KEX_DOE);
  libspdm_zero_mem(frame + KEX_DOE + size, framed - KEX_DOE - size);
  libspdm_write_uint16(frame, PCI_DOE_VENDOR_ID_PCISIG);
  frame[2] = PCI_DOE_DATA_OBJECT_TYPE_SPDM;
  libspdm_write_uint32(frame + 4, (uint32_t)(framed / 4));
  return framed;
}

/* One raw plaintext SPDM 1.4 request/response over the original device IO. */
static bool kex_raw_round_trip(libspdm_context_t *spdm, kex_state_t *s, size_t size,
                               const uint8_t **reply, size_t *reply_size)
{
  uint8_t version = s->io[KEX_DOE];
  void *in = s->io;
  size_t in_size = TEEIO_FAULT_MAX_MESSAGE_SIZE;
  if (kex_send_base(spdm, s, kex_doe_frame(s->io, size), s->io, 0) != LIBSPDM_STATUS_SUCCESS ||
      kex_receive_base(spdm, s, &in_size, &in, 0) != LIBSPDM_STATUS_SUCCESS ||
      in_size < KEX_DOE + sizeof(spdm_message_header_t) + sizeof(uint32_t) ||
      ((const uint8_t *)in)[2] != PCI_DOE_DATA_OBJECT_TYPE_SPDM ||
      (libspdm_read_uint32((const uint8_t *)in + 4) & 0x3ffff) * 4 != in_size ||
      ((const uint8_t *)in)[KEX_DOE] != version) {
    return false;
  }
  *reply = (const uint8_t *)in + KEX_DOE;
  *reply_size = in_size - KEX_DOE;
  return true;
}

/* Send the identical large KEY_EXCHANGE as an independent CHUNK_SEND transfer
 * and fetch its RSP, before libspdm's own transfer of the same bytes. The RSP
 * is the orphan: shape-checked here; libspdm authenticates the second one. */
static bool kex_chunked_orphan(libspdm_context_t *spdm, kex_state_t *s,
                               const uint8_t *large, size_t size)
{
  uint32_t dts = LIBSPDM_MIN(spdm->connection_info.capability.data_transfer_size,
                             spdm->local_context.capability.sender_data_transfer_size);
  uint8_t version = large[0], handle = (uint8_t)(spdm->chunk_context.send.chunk_handle + 0x80);
  uint8_t *req = s->io + KEX_DOE;
  const uint8_t *rsp = NULL;
  size_t sent = 0, copy, header, rsp_size = 0, total = 0, assembled = 0, expected;
  uint32_t seq;

  if (version < SPDM_MESSAGE_VERSION_14 || dts <= sizeof(spdm_chunk_send_request_14_t) + 4 ||
      dts > TEEIO_FAULT_MAX_MESSAGE_SIZE - KEX_DOE - 3) {
    return false;
  }
  for (seq = 0; sent < size; seq++) {
    header = sizeof(spdm_chunk_send_request_14_t) + (seq == 0 ? sizeof(uint32_t) : 0);
    copy = LIBSPDM_MIN(dts - header, size - sent);
    req[0] = version;
    req[1] = SPDM_CHUNK_SEND;
    req[2] = sent + copy == size ? SPDM_CHUNK_SEND_REQUEST_ATTRIBUTE_LAST_CHUNK : 0;
    req[3] = handle;
    libspdm_write_uint32(req + 4, seq);
    libspdm_write_uint32(req + 8, (uint32_t)copy);
    if (seq == 0) {
      libspdm_write_uint32(req + 12, (uint32_t)size);
    }
    libspdm_copy_mem(req + header, dts - header, large + sent, copy);
    sent += copy;
    if (!kex_raw_round_trip(spdm, s, header + copy, &rsp, &rsp_size)) {
      return false;
    }
    /* When ACK plus a large response exceeds DataTransferSize, the final
     * chunk gets the bare ERROR(LargeResponse) without the ACK portion. */
    if (sent == size && rsp[1] == SPDM_ERROR) {
      break;
    }
    if (rsp_size < sizeof(spdm_chunk_send_ack_response_14_t) ||
        rsp[1] != SPDM_CHUNK_SEND_ACK || rsp[2] != 0 || rsp[3] != handle ||
        libspdm_read_uint32(rsp + 4) != seq ||
        (sent < size && rsp_size != sizeof(spdm_chunk_send_ack_response_14_t))) {
      return false;
    }
    if (sent == size) {
      rsp += sizeof(spdm_chunk_send_ack_response_14_t);
      rsp_size -= sizeof(spdm_chunk_send_ack_response_14_t);
    }
  }
  if (rsp_size >= sizeof(spdm_error_response_t) + sizeof(spdm_error_data_large_response_t) &&
      rsp[0] == version && rsp[1] == SPDM_ERROR && rsp[2] == SPDM_ERROR_CODE_LARGE_RESPONSE) {
    handle = rsp[sizeof(spdm_error_response_t)];
    for (seq = 0; total == 0 || assembled < total; seq++) {
      req[0] = version;
      req[1] = SPDM_CHUNK_GET;
      req[2] = 0;
      req[3] = handle;
      libspdm_write_uint32(req + 4, seq);
      header = sizeof(spdm_chunk_response_response_14_t) + (seq == 0 ? sizeof(uint32_t) : 0);
      if (!kex_raw_round_trip(spdm, s, sizeof(spdm_chunk_get_request_14_t), &rsp, &rsp_size) ||
          rsp_size < header || rsp[1] != SPDM_CHUNK_RESPONSE || rsp[3] != handle ||
          libspdm_read_uint32(rsp + 4) != seq) {
        return false;
      }
      if (seq == 0) {
        total = libspdm_read_uint32(rsp + 12);
      }
      copy = libspdm_read_uint32(rsp + 8);
      if (total == 0 || total > TEEIO_FAULT_MAX_MESSAGE_SIZE || copy == 0 ||
          copy > rsp_size - header || copy > total - assembled ||
          ((rsp[2] & SPDM_CHUNK_GET_RESPONSE_ATTRIBUTE_LAST_CHUNK) != 0) !=
            (assembled + copy == total)) {
        return false;
      }
      libspdm_copy_mem(s->reply + assembled, TEEIO_FAULT_MAX_MESSAGE_SIZE - assembled,
                       rsp + header, copy);
      assembled += copy;
    }
    rsp = s->reply;
    rsp_size = total;
  }
  /* An embedded RSP may carry up to three DOE alignment bytes. */
  expected = kex_response_size(spdm, version, rsp, rsp_size);
  if (expected == 0 || expected > rsp_size || rsp_size - expected > 3 ||
      !kex_composed_id(s, rsp, &s->first_id)) {
    return false;
  }
  for (; expected < rsp_size; expected++) {
    if (rsp[expected] != 0) {
      return false;
    }
  }
  return true;
}

/* PQC KEY_EXCHANGE reaches the transport only as CHUNK_SEND. At its first
 * chunk, apply the duplicate rule to the whole request and run the orphan. */
static bool kex_chunked_duplicate(libspdm_context_t *spdm, kex_state_t *s)
{
  const libspdm_chunk_info_t *send = &spdm->chunk_context.send;
  teeio_fault_result_t fault;
  size_t size = send->large_message_size;

  if (s->pending || s->sent_twice || size < sizeof(spdm_key_exchange_request_t) ||
      KEX_DOE + size > TEEIO_FAULT_MAX_MESSAGE_SIZE) {
    return false;
  }
  libspdm_zero_mem(s->wire, KEX_DOE);
  s->wire[2] = TEEIO_FAULT_DOE_TYPE_PLAIN_SPDM;
  libspdm_write_uint32(s->wire + 4, (uint32_t)((KEX_DOE + size + 3) / 4));
  libspdm_copy_mem(s->wire + KEX_DOE, TEEIO_FAULT_MAX_MESSAGE_SIZE - KEX_DOE,
                   send->large_message, size);
  fault = teeio_fault_apply(s->wire, KEX_DOE + size, TEEIO_FAULT_MAX_MESSAGE_SIZE);
  if (fault.disposition != TEEIO_FAULT_DISPOSITION_DUPLICATE ||
      fault.message == NULL || fault.secondary_message == NULL ||
      fault.message_size != KEX_DOE + size || fault.secondary_message_size != KEX_DOE + size ||
      teeio_fault_fire_count() != 1 ||
      !libspdm_consttime_is_mem_equal(fault.message, s->wire, KEX_DOE + size) ||
      !libspdm_consttime_is_mem_equal(fault.secondary_message, s->wire, KEX_DOE + size)) {
    return false;
  }
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Duplicate chunked KEY_EXCHANGE: %s\n", teeio_fault_audit_record()));
  s->req_id = libspdm_read_uint16(s->wire + KEX_DOE + 4);
  s->dirty = true;
  if (!kex_chunked_orphan(spdm, s, s->wire + KEX_DOE, size)) {
    return false;
  }
  s->sent_twice = true;
  return true;
}

static libspdm_return_t kex_encode(void *ctx, const uint32_t *sid, bool app,
  bool request, size_t size, void *msg, size_t *out_size, void **out)
{
  libspdm_context_t *spdm = ctx;
  kex_state_t *s = spdm->app_context_data_ptr;
  const uint8_t *bytes = msg;
  uint8_t code;
  if (s->target && request && !app && size >= sizeof(spdm_message_header_t)) {
    code = bytes[1];
    if (code == SPDM_CHUNK_SEND) {
      /* Count a chunked request once, at its first chunk. */
      code = size >= sizeof(spdm_chunk_send_request_14_t) && bytes[0] >= SPDM_MESSAGE_VERSION_14 &&
        libspdm_read_uint32(bytes + 4) == 0 && spdm->chunk_context.send.chunk_in_use &&
        spdm->chunk_context.send.large_message_size >= sizeof(spdm_message_header_t) ?
        ((const uint8_t *)spdm->chunk_context.send.large_message)[1] : 0;
      if (code == SPDM_KEY_EXCHANGE && s->duplicate && sid == NULL) {
        s->chunked = true;
        if (!kex_chunked_duplicate(spdm, s)) {
          return LIBSPDM_STATUS_SEND_FAIL;
        }
      }
    }
    s->cert_count += code == SPDM_GET_CERTIFICATE;
    s->finish_count += code == SPDM_FINISH;
    s->kex_count += code == SPDM_KEY_EXCHANGE;
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

static bool kex_response_id(const libspdm_context_t *spdm, const kex_state_t *s,
                            const void *msg, size_t size, uint32_t *id)
{
  const uint8_t *b = msg;
  size_t expected;
  /* Validate the first (otherwise discarded) RSP's complete wire shape too.
   * The second RSP additionally undergoes libspdm signature/HMAC checking. */
  if (size < KEX_DOE || b[2] != PCI_DOE_DATA_OBJECT_TYPE_SPDM ||
      (libspdm_read_uint32(b + 4) & 0x3ffff) * 4 != size) {
    return false;
  }
  expected = kex_response_size(spdm, s->wire[KEX_DOE], b + KEX_DOE, size - KEX_DOE);
  if (expected == 0 || KEX_DOE + ((expected + 3) & ~(size_t)3) != size) {
    return false;
  }
  return kex_composed_id(s, b + KEX_DOE, id);
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
      !kex_response_id(spdm, s, *msg, *size, &s->first_id)) {
    return LIBSPDM_STATUS_RECEIVE_FAIL;
  }
  status = kex_send_base(spdm, s, s->wire_size, s->wire, timeout);
  if (status != LIBSPDM_STATUS_SUCCESS) {
    return status;
  }
  s->sent_twice = true;
  status = kex_receive_base(spdm, s, &second_size, &second, timeout);
  if (status != LIBSPDM_STATUS_SUCCESS || second_size > capacity ||
      !kex_response_id(spdm, s, second, second_size, &s->second_id)) {
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
   * This setter stores the SPDM-formatted chain (transcript recording keeps
   * whole chains, including ML-DSA); it does NOT verify authority. Only
   * setup's trusted chain is allowed. */
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

static bool kex_session(void *spdm, uint32_t *session_id)
{
  uint32_t sid = 0;
  uint8_t heartbeat = 0;
  libspdm_return_t status = libspdm_start_session(spdm, false, NULL, 0,
    SPDM_KEY_EXCHANGE_REQUEST_NO_MEASUREMENT_SUMMARY_HASH, 0, 0,
    &sid, &heartbeat, NULL);
  if (session_id != NULL) {
    *session_id = sid;
  }
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
  s->io = malloc(TEEIO_FAULT_MAX_MESSAGE_SIZE);
  if (!s->chain || !s->wire || !s->reply || !s->io) {
    free(s->chain); free(s->wire); free(s->reply); free(s->io);
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
  if (!teeio_spdm_apply_algorithm_profile(spdm)) {
    return false;
  }
  s->chain_size = LIBSPDM_MAX_CERT_CHAIN_SIZE;
  if (libspdm_init_connection(spdm, false) != LIBSPDM_STATUS_SUCCESS ||
      !teeio_spdm_negotiated_profile_matches(spdm) ||
      libspdm_get_certificate_ex(spdm, NULL, 0, 0, &s->chain_size, s->chain,
        &anchor, &anchor_size) != LIBSPDM_STATUS_SUCCESS ||
      anchor == NULL || anchor_size == 0 ||
      s->chain_size <= sizeof(spdm_cert_chain_t) + 48 ||
      !kex_session(spdm, NULL) || teeio_fault_fire_count() != 0) {
    return false;
  }
  teeio_spdm_log_negotiated_version(spdm);
  /* New connection, no GET_CERTIFICATE on it. The profile pins the same
   * algorithms, so the imported chain matches the renegotiated hash. */
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
  uint32_t sid = 0;
  if (test != NULL && test->spdm_context != NULL && test->test_scratch_buffer_size == sizeof(*s)) {
    s = (void *)test->test_scratch_buffer;
    if (s->magic == KEX_MAGIC && s->ready && teeio_fault_fire_count() == 0 &&
        teeio_fault_driver() == (s->duplicate ?
          TEEIO_FAULT_DRIVER_KEY_EXCHANGE_DUPLICATE :
          TEEIO_FAULT_DRIVER_KEY_EXCHANGE_CACHED_CERT)) {
      s->ready = false;
      s->target = true;
      passed = kex_session(test->spdm_context, &sid);
      s->target = false;
      if (s->chunked && s->sent_twice) {
        /* The orphan went first; libspdm authenticated the second RSP. */
        s->second_id = sid;
        s->distinct = sid != INVALID_SESSION_ID && s->first_id != sid;
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Duplicate chunked KEY_EXCHANGE req=0x%04x first=0x%08x second=0x%08x\n",
                     s->req_id, s->first_id, s->second_id));
        if (passed && !s->distinct) {
          teeio_fault_record_actual("duplicate_composed_session_id");
        }
      }
      passed = passed && s->kex_count == 1 && s->finish_count == 1 && s->cert_count == 0 &&
        (s->duplicate ? (s->sent_twice && s->distinct && teeio_fault_fire_count() == 1) :
                        teeio_fault_fire_count() == 0);
      /* Recover even when the duplicate fails. Never turn a failed target
       * exchange into a pass merely because a subsequent reconnect works. */
      recovery = kex_version_reset(test->spdm_context, s) &&
        kex_restore_cache(test->spdm_context, s) && kex_session(test->spdm_context, NULL);
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
      free(s->chain); free(s->wire); free(s->reply); free(s->io);
      libspdm_zero_mem(s, sizeof(*s));
    }
  }
  spdm_test_case_common_teardown(test_context);
}