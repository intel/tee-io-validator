/* Copyright 2026 Intel. SPDX-License-Identifier: BSD-3-Clause */
/* Test the production static driver directly. Only recovery's high-level
 * operations are mocked; FI, DOE framing, and CHUNK_SEND responder are real. */
#define libspdm_init_connection host_init_connection
#define libspdm_get_certificate host_get_certificate
#define libspdm_start_session host_start_session
#define libspdm_heartbeat host_heartbeat
#define libspdm_stop_session host_stop_session
#define libspdm_set_certificate host_set_certificate
#include "../../library/spdm_test_lib/test_case/test_case_fault.c"
#include "internal/libspdm_responder_lib.h"
#include <assert.h>

static unsigned assertions_failed;
static unsigned recovery_step;
static unsigned fail_step;
static void *expected_context;
static IDE_TEST_FAULT_CONFIG config;
static bool b5_control;
static unsigned b5_control_calls;
static uint8_t b5_chain[LIBSPDM_MAX_CERT_CHAIN_SIZE];
static size_t b5_chain_size;
static const char *b5_directory;
static bool b5_wire_mode, b5_bypass, b5_legacy;
static unsigned b5_transfer;
static libspdm_return_t b5_exchange(void *spdm, void *chain, size_t size);
/* Existing requester debuglib references these application-owned settings. */
bool g_libspdm_log = false;
FILE *m_logfile;
int g_debug_level = TEEIO_DEBUG_ERROR;

bool teeio_record_assertion_result(int cls, int id, int assertion,
  ide_common_test_case_assertion_type_t type, teeio_test_result_t result,
  const char *format, ...)
{
  assertions_failed += result == TEEIO_TEST_RESULT_FAILED;
  return true;
}

static unsigned dispatch_kind, dispatch_setup, dispatch_run, dispatch_teardown;
static unsigned common_teardown;
static bool dispatch_setup_result;
void spdm_test_case_common_teardown(void *context) { common_teardown++; }
bool teeio_spdm_apply_version_override(void *context)
{
  assert(!"dedicated dispatch fell through to chunk setup"); return false;
}
void teeio_spdm_log_negotiated_version(void *context)
{
  assert(!"dedicated dispatch fell through to chunk setup");
}

/* Separate drivers are tested with real libspdm in their own harnesses.
 * These routing probes fail loudly if reached by an ordinary chunk case. */
bool spdm_test_case_fault_key_exchange_setup(void *context)
{
  assert(dispatch_kind == 4); dispatch_setup++; return dispatch_setup_result;
}
void spdm_test_case_fault_key_exchange_run(void *context)
{
  assert(dispatch_kind == 4); dispatch_run++;
}
void spdm_test_case_fault_key_exchange_teardown(void *context)
{
  assert(dispatch_kind == 4); dispatch_teardown++;
}
bool spdm_test_case_fault_mut_auth_setup(void *context)
{
  assert(dispatch_kind == 5); dispatch_setup++; return dispatch_setup_result;
}
void spdm_test_case_fault_mut_auth_run(void *context)
{
  assert(dispatch_kind == 5); dispatch_run++;
}

static void dedicated_dispatch(void)
{
  const char *names[] = {"B4_duplicate_key_exchange",
    "B4_key_exchange_before_certificate", "B5_invalid_finish_signature"};
  unsigned i, success;
  for (i = 0; i < 3; i++) {
    for (success = 0; success < 2; success++) {
      teeio_spdm_test_context_t test = {0};
      memset(&config, 0, sizeof(config)); config.rule_count = 1;
      strcpy(config.rules[0].scenario, names[i]); teeio_fault_init(&config);
      dispatch_kind = i == 2 ? 5 : 4;
      dispatch_setup = dispatch_run = dispatch_teardown = common_teardown = 0;
      dispatch_setup_result = success != 0;
      /* Deliberately looks like a populated chunk scratch. Mut-auth teardown
       * must not read its bogus installed/callback/session fields. */
      memset(test.test_scratch_buffer, 0xa5, sizeof(test.test_scratch_buffer));
      test.test_scratch_buffer_size = sizeof(teeio_fault_test_buffer_t);
      assert(spdm_test_case_fault_setup(&test) == dispatch_setup_result);
      if (success) spdm_test_case_fault_run(&test);
      spdm_test_case_fault_teardown(&test);
      assert(dispatch_setup == 1 && dispatch_run == success);
      assert(dispatch_teardown == (i != 2) && common_teardown == (i == 2));
      assert(test.test_scratch_buffer[0] == 0xa5);
    }
  }
  strcpy(config.rules[0].scenario, "B4_duplicate_key_exchange_suffix");
  teeio_fault_init(&config); assert(!is_key_exchange_scenario());
  strcpy(config.rules[0].scenario, "b4_duplicate_key_exchange");
  teeio_fault_init(&config); assert(!is_key_exchange_scenario());
  dispatch_kind = 0;
  puts("PASS: exact B4/B5 dispatch, setup failures, scratch-safe common mut-auth teardown");
}

/* Unexpected dispatch is a test failure: an oversized first chunk must never
 * reach SET_CERTIFICATE or its update callback. */
libspdm_get_spdm_response_func libspdm_get_response_func_via_request_code(uint8_t code)
{
  assert(!"oversized first chunk reached terminal dispatch");
  return NULL;
}

libspdm_return_t libspdm_responder_handle_response_state(libspdm_context_t *spdm,
  uint8_t code, size_t *size, void *response)
{
  assert(!"unexpected responder state");
  return LIBSPDM_STATUS_INVALID_STATE_LOCAL;
}

/* Stand-in for the common observer, using the REAL transport decoder. */
libspdm_return_t teeio_fault_transport_decode_message(void *spdm,
  uint32_t **session, bool *app, bool request, size_t transport_size,
  void *transport, size_t *size, void **message)
{
  libspdm_return_t status = libspdm_transport_pci_doe_decode_message(
    spdm, session, app, request, transport_size, transport, size, message);
  if (!LIBSPDM_STATUS_IS_ERROR(status) && *size >= 4) {
    const uint8_t *bytes = *message;
    size_t offset = bytes[0] >= SPDM_MESSAGE_VERSION_14 ? 8 : 6;
    if (bytes[1] == SPDM_CHUNK_SEND_ACK && *size >= offset + 4 &&
        bytes[offset + 1] == SPDM_ERROR) {
      char actual[32];
      snprintf(actual, sizeof(actual), "spdm_error_0x%02x", bytes[offset + 2]);
      teeio_fault_record_actual(actual);
    }
  }
  return status;
}

libspdm_return_t teeio_fault_transport_encode_message(void *spdm,
  const uint32_t *session, bool app, bool request, size_t size, void *message,
  size_t *transport_size, void **transport)
{
  assert(!"unscoped plaintext injector called");
  return LIBSPDM_STATUS_SEND_FAIL;
}

static libspdm_return_t recovery_call(void *spdm, unsigned step)
{
  teeio_fault_test_buffer_t *state = ((libspdm_context_t *)spdm)->app_context_data_ptr;
  assert(spdm == expected_context);
  assert(!state->armed);
  assert(++recovery_step == step);
  return fail_step == step ? LIBSPDM_STATUS_SEND_FAIL : LIBSPDM_STATUS_SUCCESS;
}

libspdm_return_t host_init_connection(void *spdm, bool version_only)
{
  libspdm_return_t status = recovery_call(spdm, 1);
  unsigned i;
  for (i = 0; i < LIBSPDM_MAX_SESSION_COUNT; i++) {
    ((libspdm_context_t *)spdm)->session_info[i].session_id = INVALID_SESSION_ID;
  }
  return status;
}

libspdm_return_t host_get_certificate(void *spdm, const uint32_t *session,
  uint8_t slot, size_t *size, void *chain)
{
  assert(session == NULL);
  if (b5_wire_mode && recovery_step == 0) {
    memcpy(chain, b5_chain, b5_chain_size);
    *size = b5_chain_size;
    return LIBSPDM_STATUS_SUCCESS;
  }
  if (b5_control) {
    memset(chain, 0, 1024);
    *size = 1024;
    return LIBSPDM_STATUS_SUCCESS;
  }
  return recovery_call(spdm, 2);
}

libspdm_return_t host_set_certificate(void *spdm, const uint32_t *session,
  uint8_t slot, void *chain, size_t size)
{
  if (b5_wire_mode) return b5_exchange(spdm, chain, size);
  assert(b5_control);
  b5_control = false;
  b5_control_calls++;
  /* Same ambiguous callback rejection as the real unresolved B5 baseline. */
  return LIBSPDM_STATUS_ERROR_PEER;
}

libspdm_return_t host_start_session(void *spdm, bool psk, const void *hint,
  uint16_t hint_size, uint8_t hash, uint8_t slot, uint8_t policy,
  uint32_t *session, uint8_t *period, void *measurement)
{
  *session = 0x22223333;
  return recovery_call(spdm, 3);
}

libspdm_return_t host_heartbeat(void *spdm, uint32_t session)
{
  assert(session == 0x22223333);
  return recovery_call(spdm, 4);
}

libspdm_return_t host_stop_session(void *spdm, uint32_t session, uint8_t attributes)
{
  assert(session == 0x22223333);
  return recovery_call(spdm, 5);
}

static teeio_fault_test_buffer_t *fixture(teeio_spdm_test_context_t *test,
  const char *scenario, uint8_t code, teeio_fault_action_t action)
{
  libspdm_context_t *spdm = calloc(1, sizeof(*spdm));
  assert(spdm != NULL);
  memset(test, 0, sizeof(*test));
  test->spdm_context = spdm;
  spdm->version = LIBSPDM_CONTEXT_STRUCT_VERSION;
  spdm->transport_encode_message = teeio_fault_transport_encode_message;
  spdm->transport_decode_message = teeio_fault_transport_decode_message;
  spdm->app_context_data_ptr = test; /* restoration sentinel */
  memset(&config, 0, sizeof(config));
  config.rule_count = 1;
  config.rules[0].id = 1;
  snprintf(config.rules[0].scenario, sizeof(config.rules[0].scenario), "%s", scenario);
  config.rules[0].doe_type = TEEIO_FAULT_DOE_TYPE_PLAIN_SPDM;
  config.rules[0].spdm_code = code;
  config.rules[0].occurrence = 1;
  config.rules[0].action = action;
  config.rules[0].size = 4;
  config.rules[0].pattern_size = 4;
  teeio_fault_init(&config);
  assert(chunk_install(test));
  ((teeio_fault_test_buffer_t *)(void *)test->test_scratch_buffer)->version =
    SPDM_MESSAGE_VERSION_14;
  return (void *)test->test_scratch_buffer;
}

static void cleanup(teeio_spdm_test_context_t *test)
{
  libspdm_context_t *spdm = test->spdm_context;
  teeio_fault_test_buffer_t *state = (void *)test->test_scratch_buffer;
  state->session_id = 0;
  spdm_test_case_fault_teardown(test);
  assert(spdm->transport_encode_message == teeio_fault_transport_encode_message);
  assert(spdm->transport_decode_message == teeio_fault_transport_decode_message);
  assert(spdm->app_context_data_ptr == test);
  free(spdm);
}

static void make_send(uint8_t *message, uint8_t version, uint32_t sequence,
  bool last, size_t size)
{
  memset(message, 0, size);
  message[0] = version;
  message[1] = SPDM_CHUNK_SEND;
  message[2] = last ? SPDM_CHUNK_SEND_REQUEST_ATTRIBUTE_LAST_CHUNK : 0;
  message[3] = 7;
  libspdm_write_uint32(message + 4, sequence);
  libspdm_write_uint32(message + 8, (uint32_t)(size - (sequence == 0 ? 16 : 12)));
  if (sequence == 0) {
    libspdm_write_uint32(message + 12, 4096);
    message[16] = version;
    message[17] = SPDM_SET_CERTIFICATE;
  }
}

static void observe(teeio_spdm_test_context_t *test, const uint8_t *message, size_t size)
{
  uint8_t storage[512];
  void *transport = storage;
  void *decoded;
  size_t transport_size = sizeof(storage), decoded_size = sizeof(storage);
  uint32_t *session = NULL;
  bool app = false;
  memcpy(storage + 8, message, size);
  assert(!LIBSPDM_STATUS_IS_ERROR(libspdm_transport_pci_doe_encode_message(
    test->spdm_context, NULL, false, false, size, storage + 8,
    &transport_size, &transport)));
  assert(!LIBSPDM_STATUS_IS_ERROR(chunk_decode(test->spdm_context, &session,
    &app, false, transport_size, transport, &decoded_size, &decoded)));
}

static void oversized(uint8_t version)
{
  teeio_spdm_test_context_t test;
  teeio_fault_test_buffer_t *state = fixture(&test,
    TEEIO_FAULT_OVERSIZED_CHUNK_SCENARIO, SPDM_CHUNK_SEND, TEEIO_FAULT_ACTION_EXTEND);
  uint8_t storage[640], response[64];
  void *transport = storage, *plain;
  size_t transport_size = sizeof(storage), plain_size = sizeof(storage);
  size_t response_size = sizeof(response);
  uint32_t *session = NULL;
  bool app = false;
  libspdm_context_t *responder = calloc(1, sizeof(*responder));
  state->version = version;
  assert(responder != NULL);
  responder->version = LIBSPDM_CONTEXT_STRUCT_VERSION;
  responder->connection_info.version = (spdm_version_number_t)version << SPDM_VERSION_NUMBER_SHIFT_BIT;
  responder->connection_info.connection_state = LIBSPDM_CONNECTION_STATE_NEGOTIATED;
  responder->local_context.capability.flags = SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_CHUNK_CAP;
  responder->connection_info.capability.flags = SPDM_GET_CAPABILITIES_REQUEST_FLAGS_CHUNK_CAP;
  responder->local_context.capability.data_transfer_size = 256;
  responder->local_context.capability.max_spdm_msg_size = 8192;
  responder->scratch_buffer = calloc(1, 65536);
  responder->scratch_buffer_size = 65536;

  make_send(storage + 8, version, 0, false, 256);
  /* Positive control against real responder: same first chunk is valid. */
  assert(!LIBSPDM_STATUS_IS_ERROR(libspdm_get_response_chunk_send(responder,
    256, storage + 8, &response_size, response)));
  assert(response[1] == SPDM_CHUNK_SEND_ACK && response[2] == 0);
  assert(responder->chunk_context.send.chunk_in_use);
  memset(&responder->chunk_context, 0, sizeof(responder->chunk_context));

  /* Setup/control traffic never consumes FI occurrence. */
  assert(!LIBSPDM_STATUS_IS_ERROR(chunk_encode(test.spdm_context, NULL, false,
    true, 256, storage + 8, &transport_size, &transport)));
  assert(teeio_fault_fire_count() == 0 && config.rules[0].match_count == 0);
  state->armed = true;
  state->data_transfer_size = 256;
  transport_size = sizeof(storage);
  assert(!LIBSPDM_STATUS_IS_ERROR(chunk_encode(test.spdm_context, NULL, false,
    true, 256, storage + 8, &transport_size, &transport)));
  assert(transport_size == 268);
  assert(libspdm_read_uint32((uint8_t *)transport + 4) == 67);
  assert(!LIBSPDM_STATUS_IS_ERROR(libspdm_transport_pci_doe_decode_message(
    test.spdm_context, &session, &app, true, transport_size, transport, &plain_size, &plain)));
  assert(plain_size == 260); /* appended bytes are actually visible to SPDM */
  response_size = sizeof(response);
  assert(!LIBSPDM_STATUS_IS_ERROR(libspdm_get_response_chunk_send(responder,
    plain_size, plain, &response_size, response)));
  observe(&test, response, response_size);
  assert(state->first_early_invalid_request && !state->invalid);
  assert(teeio_fault_fire_count() == 1 && !responder->chunk_context.send.chunk_in_use);
  assert(strstr(teeio_fault_result_record(), "spdm_error_0x01"));

  /* Regression: terminal 0x05, late 0x01, missing EARLY_ERROR, wrong sequence,
   * and truncated embedded errors must not satisfy the first-ACK predicate. */
  state->first_early_invalid_request = false;
  state->ack_count = 0;
  response[2] = 0;
  response[(version >= SPDM_MESSAGE_VERSION_14 ? 8 : 6) + 2] = 5;
  observe(&test, response, response_size);
  assert(!state->first_early_invalid_request);
  response[2] = 1;
  response[(version >= SPDM_MESSAGE_VERSION_14 ? 8 : 6) + 2] = 1;
  observe(&test, response, response_size);
  assert(!state->first_early_invalid_request);
  state->ack_count = 0;
  observe(&test, response, version >= SPDM_MESSAGE_VERSION_14 ? 8 : 6);
  assert(!state->first_early_invalid_request);
  state->ack_count = 0;
  response[4] = 1;
  observe(&test, response, response_size);
  assert(!state->first_early_invalid_request && state->invalid);
  free(responder->scratch_buffer);
  free(responder);
  cleanup(&test);
}

static void missing(unsigned chunks, uint8_t version)
{
  teeio_spdm_test_context_t test;
  teeio_fault_test_buffer_t *state = fixture(&test,
    TEEIO_FAULT_MISSING_SCENARIO, SPDM_CHUNK_SEND, TEEIO_FAULT_ACTION_DROP);
  unsigned i;
  uint8_t storage[512], ack[8] = {0};
  state->version = version;
  state->armed = true;
  for (i = 0; i < chunks; i++) {
    void *transport = storage;
    size_t transport_size = sizeof(storage);
    libspdm_return_t status;
    make_send(storage + 8, version, i, i + 1 == chunks, 256);
    status = chunk_encode(test.spdm_context, NULL, false, true, 256,
                           storage + 8, &transport_size, &transport);
    if (i + 1 == chunks) {
      assert(status == LIBSPDM_STATUS_SEND_FAIL);
      assert(state->selected && teeio_fault_fire_count() == 1);
    } else {
      assert(!LIBSPDM_STATUS_IS_ERROR(status));
      assert(config.rules[0].match_count == 0);
      ack[0] = version;
      ack[1] = SPDM_CHUNK_SEND_ACK;
      ack[3] = 7;
      libspdm_write_uint32(ack + 4, i);
      observe(&test, ack, version >= SPDM_MESSAGE_VERSION_14 ? 8 : 6);
    }
  }
  assert(!state->invalid && state->ack_count == chunks - 1);
  assert(strstr(teeio_fault_result_record(), "final_chunk_withheld"));
  cleanup(&test);
}

static void abandon_and_recovery(unsigned failure)
{
  teeio_spdm_test_context_t test;
  teeio_fault_test_buffer_t *state = fixture(&test,
    TEEIO_FAULT_ABANDON_SCENARIO, SPDM_CHUNK_GET, TEEIO_FAULT_ACTION_ABANDON);
  uint8_t storage[512] = {0}, response[32] = {0};
  void *transport = storage;
  size_t transport_size = sizeof(storage);
  char result[768];
  state->armed = true;
  storage[8] = SPDM_MESSAGE_VERSION_14;
  storage[9] = SPDM_CHUNK_GET;
  assert(!LIBSPDM_STATUS_IS_ERROR(chunk_encode(test.spdm_context, NULL, false,
    true, 8, storage + 8, &transport_size, &transport)));
  assert(teeio_fault_fire_count() == 0);
  response[0] = SPDM_MESSAGE_VERSION_14;
  response[1] = SPDM_CHUNK_RESPONSE;
  libspdm_write_uint32(response + 8, 16);
  libspdm_write_uint32(response + 12, 2048);
  response[16] = SPDM_MESSAGE_VERSION_14;
  response[17] = SPDM_DIGESTS;
  observe(&test, response, sizeof(response));
  assert(!state->partial_response);
  response[17] = SPDM_CERTIFICATE;
  observe(&test, response, sizeof(response));
  assert(state->partial_response);
  libspdm_write_uint32(storage + 12, state->next_get_sequence);
  transport_size = sizeof(storage);
  assert(chunk_encode(test.spdm_context, NULL, false, true, 8, storage + 8,
    &transport_size, &transport) == LIBSPDM_STATUS_SEND_FAIL);
  assert(teeio_fault_fire_count() == 1 && !state->invalid);
  snprintf(result, sizeof(result), "%s", teeio_fault_result_record());
  state->session_id = 0x11112222;
  ((libspdm_context_t *)test.spdm_context)->session_info[0].session_id = state->session_id;
  expected_context = test.spdm_context;
  recovery_step = 0;
  fail_step = failure;
  assertions_failed = 0;
  assert(chunk_recover(&test) == (failure == 0));
  assert(recovery_step == (failure == 0 ? 5 : failure));
  assert(assertions_failed == (failure == 0 ? 0 : 1));
  assert(strcmp(result, teeio_fault_result_record()) == 0);
  cleanup(&test);
}

static void b5_der_bounds(void)
{
  uint8_t inputs[][12] = {{0x30, 0x80}, {0x30, 0x81, 0x01, 0},
    {0x30, 0x82, 0, 0x80}, {0x30, 0x89}, {0x30, 0x88, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, {0x31, 0}};
  unsigned i;
  size_t n, length;
  const uint8_t *p, *value;
  for (i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
    for (n = 0; n <= sizeof(inputs[0]); n++) {
      p = inputs[i];
      assert(!certificate_der(&p, p + n, 0x30, &value, &length));
    }
  }
  puts("PASS: bounded DER rejects indefinite, nonminimal, truncated, overflow and wrong tags");
}

static void b5_config(void)
{
  config.rules[0].offset = 12;
  config.rules[0].pattern_size = 1;
  config.rules[0].pattern[0] = 1;
}

static void b5_fail_closed(void)
{
  teeio_fault_test_buffer_t parsed = {0};
  const uint32_t hash = SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_384;
  assert(certificate_signature_target(&parsed, b5_chain, b5_chain_size, hash));
  size_t target = parsed.certificate_target;
  for (size_t n = 0; n <= target - sizeof(spdm_set_certificate_request_t); n++)
    assert(!certificate_signature_target(&parsed, b5_chain, n, hash));
  for (unsigned failure = 0; failure < 7; failure++) {
    teeio_spdm_test_context_t test;
    teeio_fault_test_buffer_t *state = fixture(&test,
      TEEIO_FAULT_SET_CERTIFICATE_SCENARIO, SPDM_CHUNK_SEND, TEEIO_FAULT_ACTION_XOR);
    uint8_t storage[4096] = {0}, saved[4096];
    b5_config();
    assert(certificate_signature_target(state, b5_chain, b5_chain_size, hash));
    /* Put the computed target in the first chunk; still leave data pending. */
    size_t size = 16 + state->certificate_target + 1;
    assert(size < sizeof(storage) - 8 && size - 16 < b5_chain_size + 4);
    make_send(storage + 8, state->version, 0, false, size);
    libspdm_write_uint32(storage + 20, (uint32_t)(b5_chain_size + 4));
    memcpy(storage + 28, b5_chain, size - 20);
    state->armed = true;
    switch (failure) {
      case 0: config.rules[0].pattern[0] = 3; break;
      case 1: config.rules[0].offset = 24; break;
      case 2: config.rules[0].occurrence = 2; break;
      case 3: state->certificate_target_valid = false; break;
      case 4: storage[8 + 16 + state->certificate_target] ^= 2; break;
      case 5: storage[12] = 1; break;
      case 6: libspdm_write_uint32(storage + 16, (uint32_t)size); break;
    }
    memcpy(saved, storage, sizeof(storage));
    void *transport = storage;
    size_t transport_size = sizeof(storage);
    assert(chunk_encode(test.spdm_context, NULL, false, true, size, storage + 8,
      &transport_size, &transport) == LIBSPDM_STATUS_SEND_FAIL);
    assert(state->invalid && !state->selected && memcmp(saved, storage, sizeof(storage)) == 0);
    cleanup(&test);
  }
  puts("PASS: B5 truncations, multibit/wrong-offset/occurrence rules, missing target, byte mismatch and malformed chunks fail closed");
}

/* Generate actual driver-mutated plaintext chunks, preserving every other
 * byte. The separate responder bridge reassembles these with real libspdm/mbedTLS. */
static void b5_stream(teeio_spdm_test_context_t *test, const uint8_t *chain,
                     size_t chain_size, FILE *output, FILE *comparison,
                     const uint8_t *final_ack, size_t final_ack_size)
{
  teeio_fault_test_buffer_t *state = (void *)test->test_scratch_buffer;
  uint8_t logical[LIBSPDM_MAX_CERT_CHAIN_SIZE + 4] = {0};
  uint8_t storage[4096], ack[8] = {0}, expected[4096];
  size_t total = chain_size + 4, sent = 0;
  unsigned sequence = 0, differences = 0;
  logical[0] = state->version;
  logical[1] = SPDM_SET_CERTIFICATE;
  memcpy(logical + 4, chain, chain_size);
  while (sent < total) {
    size_t header = sequence == 0 ? 16 : 12;
    size_t payload = b5_transfer - header;
    if (payload > total - sent) payload = total - sent;
    size_t size = header + payload;
    make_send(storage + 8, state->version, sequence, sent + payload == total, size);
    if (sequence == 0) libspdm_write_uint32(storage + 20, (uint32_t)total);
    memcpy(storage + 8 + header, logical + sent, payload);
    memcpy(expected, storage + 8, size);
    void *transport = storage;
    size_t transport_size = sizeof(storage);
    assert(chunk_encode(test->spdm_context, NULL, false, true, size,
      storage + 8, &transport_size, &transport) == LIBSPDM_STATUS_SUCCESS);
    const uint8_t *actual = (uint8_t *)transport + 8;
    for (size_t i = 0; i < size; i++) {
      if (actual[i] != expected[i]) {
        assert(i >= header && sent + i - header == state->certificate_target);
        assert((actual[i] ^ expected[i]) == 1);
        differences++;
        if (output) printf("B5 one-bit wire proof: version=%02x transfer=%u logical=%zu sequence=%u DOE_offset=%zu\n",
          state->version, b5_transfer, sent + i - header, sequence, i + 8);
      }
    }
    uint32_t size32 = (uint32_t)size;
    if (output) {
      assert(fwrite(&size32, 4, 1, output) == 1);
      assert(fwrite(actual, 1, size, output) == size);
    }
    if (comparison) {
      uint32_t recorded;
      assert(fread(&recorded, 4, 1, comparison) == 1 && recorded == size);
      assert(fread(expected, 1, size, comparison) == size && memcmp(expected, actual, size) == 0);
    }
    if (final_ack) {
      if (sent + payload == total) {
        observe(test, final_ack, final_ack_size);
      } else {
        ack[0] = state->version; ack[1] = SPDM_CHUNK_SEND_ACK; ack[3] = 7;
        libspdm_write_uint32(ack + 4, sequence);
        observe(test, ack, state->version >= SPDM_MESSAGE_VERSION_14 ? 8 : 6);
      }
    }
    sent += payload;
    sequence++;
  }
  assert(differences == (state->armed ? 1 : 0));
  if (comparison) assert(fgetc(comparison) == EOF);
}

static void b5_path(char *path, size_t capacity, uint8_t version, const char *kind,
                    const char *extension)
{
  snprintf(path, capacity, "%s/%02x-%u-%s.%s", b5_directory, version,
           b5_transfer, kind, extension);
}

static libspdm_return_t b5_exchange(void *spdm, void *chain, size_t size)
{
  teeio_fault_test_buffer_t *state = ((libspdm_context_t *)spdm)->app_context_data_ptr;
  teeio_spdm_test_context_t *test = state->saved_app;
  const char *kind = state->armed ? "mutated" : state->backend_probe ? "probe" : "positive";
  char path[1024];
  uint8_t ack[32];
  b5_path(path, sizeof(path), state->version, kind, "chunks");
  FILE *comparison = fopen(path, "rb");
  assert(comparison);
  b5_path(path, sizeof(path), state->version,
    state->backend_probe && b5_legacy ? "legacy" :
    state->armed && b5_bypass ? "bypass" : kind, "ack");
  FILE *response = fopen(path, "rb");
  assert(response);
  size_t ack_size = fread(ack, 1, sizeof(ack), response);
  assert(ack_size >= 10 && fclose(response) == 0);
  b5_stream(test, chain, size, NULL, comparison, ack, ack_size);
  assert(fclose(comparison) == 0);
  return ack[ack_size - 3] == SPDM_ERROR ? LIBSPDM_STATUS_ERROR_PEER : LIBSPDM_STATUS_SUCCESS;
}

static void b5_real_fixture(bool generate)
{
  unsigned version, variant, control;
  const unsigned transfers[] = {128, 256, 512, 1024};
  for (version = SPDM_MESSAGE_VERSION_12; version <= SPDM_MESSAGE_VERSION_14; version += 2) {
    for (variant = 0; variant < 4; variant++) {
      b5_transfer = transfers[variant];
      for (control = 0; control < 3; control++) {
        teeio_spdm_test_context_t test;
        teeio_fault_test_buffer_t *state = fixture(&test,
          TEEIO_FAULT_SET_CERTIFICATE_SCENARIO, SPDM_CHUNK_SEND, TEEIO_FAULT_ACTION_XOR);
        state->version = version;
        b5_config();
        assert(certificate_signature_target(state, b5_chain, b5_chain_size,
          SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_384));
        if (generate) {
          char path[1024];
          const char *kind = control == 0 ? "positive" : control == 1 ? "probe" : "mutated";
          b5_path(path, sizeof(path), version, kind, "chunks");
          FILE *output = fopen(path, "wb");
          assert(output);
          state->armed = control == 2;
          if (control == 1) b5_chain[sizeof(spdm_cert_chain_t)] ^= 1;
          b5_stream(&test, b5_chain, b5_chain_size, output, NULL, NULL, 0);
          if (control == 1) b5_chain[sizeof(spdm_cert_chain_t)] ^= 1;
          assert(fclose(output) == 0 && !state->invalid);
          assert(teeio_fault_fire_count() == (control == 2));
          if (control == 2) {
            assert(state->selected && state->certificate_sent == state->certificate_request_size);
            printf("B5 %02x transfer=%u target=%zu FI=%s\n", version, b5_transfer,
              state->certificate_target, teeio_fault_audit_record());
          }
        } else {
          libspdm_context_t *spdm = test.spdm_context;
          spdm->connection_info.algorithm.base_hash_algo = SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_384;
          spdm->connection_info.capability.flags = SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_SET_CERT_CAP;
          spdm->connection_info.capability.data_transfer_size = b5_transfer;
          state->session_id = 0x11112222;
          b5_wire_mode = true;
          b5_bypass = control == 1;
          b5_legacy = control == 2;
          recovery_step = fail_step = assertions_failed = 0;
          expected_context = spdm;
          spdm_test_case_fault_run(&test);
          assert(!state->invalid && state->selected == !b5_legacy &&
            teeio_fault_fire_count() == (b5_legacy ? 0 : 1));
          assert(assertions_failed == (b5_legacy ? 2 : b5_bypass ? 1 : 0) && recovery_step == 5);
          if (!b5_legacy) assert(strstr(teeio_fault_result_record(),
            b5_bypass ? "spdm_error_0x44" : "spdm_error_0x05"));
          printf("PASS B5 full driver %02x transfer=%u bypass=%u legacy=%u failed_assertions=%u\n",
            version, b5_transfer, b5_bypass, b5_legacy, assertions_failed);
          b5_wire_mode = false;
        }
        cleanup(&test);
      }
    }
  }
}

static void b5_rejecting_positive_control(void)
{
  teeio_spdm_test_context_t test;
  teeio_fault_test_buffer_t *state = fixture(&test,
    TEEIO_FAULT_SET_CERTIFICATE_SCENARIO, SPDM_CHUNK_SEND, TEEIO_FAULT_ACTION_XOR);
  libspdm_context_t *spdm = test.spdm_context;
  spdm->connection_info.algorithm.base_hash_algo = SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_384;
  spdm->connection_info.capability.flags = SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_SET_CERT_CAP;
  spdm->connection_info.capability.data_transfer_size = 256;
  state->session_id = 0x11112222;
  b5_control = true;
  b5_control_calls = 0;
  recovery_step = fail_step = assertions_failed = 0;
  expected_context = spdm;
  spdm_test_case_fault_run(&test);
  assert(!b5_control && b5_control_calls == 1);
  assert(teeio_fault_fire_count() == 0 && !state->selected);
  assert(assertions_failed == 1 && recovery_step == 5);
  cleanup(&test);
}

int main(int argc, char **argv)
{
  unsigned n;
  if (argc == 4) {
    FILE *f = fopen(argv[2], "rb");
    assert(f);
    b5_chain_size = fread(b5_chain, 1, sizeof(b5_chain), f);
    assert(b5_chain_size > 1024 && fclose(f) == 0);
    b5_directory = argv[3];
    b5_fail_closed();
    b5_real_fixture(strcmp(argv[1], "generate") == 0);
    return 0;
  }
  dedicated_dispatch();
  oversized(SPDM_MESSAGE_VERSION_12);
  oversized(SPDM_MESSAGE_VERSION_14);
  for (n = 2; n <= 9; n++) {
    missing(n, SPDM_MESSAGE_VERSION_12);
    missing(n, SPDM_MESSAGE_VERSION_14);
  }
  for (n = 0; n <= 5; n++) {
    abandon_and_recovery(n);
  }
  b5_der_bounds();
  b5_rejecting_positive_control();
  puts("PASS: real DOE resize + real responder first EARLY_ERROR 0x01 (1.2/1.4)");
  puts("PASS: dynamic LAST_CHUNK (2..9 chunks), partial CERTIFICATE phase gating");
  puts("PASS: mocked recovery ordering/failure propagation and original FI outcome preservation");
  puts("PASS: B5 payload selection, FI scope, and callback/app-context restoration");
  puts("PASS: B5 rejecting positive control fails without injecting or claiming provenance");
  return 0;
}
