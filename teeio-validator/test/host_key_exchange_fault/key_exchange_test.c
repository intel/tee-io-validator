/* Copyright 2026 Intel. SPDX-License-Identifier: BSD-3-Clause */
/* In-memory device IO; real FI, DOE, requester, responder, crypto and FINISH.
 * Deliberate peer/IO defects are restricted to the selected target exchange. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "teeio_validator.h"
#include "teeio_spdmlib.h"
#include "teeio_fault_injection.h"
#include "spdm_test_lib.h"
#include "internal/libspdm_common_lib.h"
#include "internal/libspdm_device_secret_lib.h"
#include "internal/libspdm_responder_lib.h"

bool spdm_test_case_fault_key_exchange_setup(void *);
void spdm_test_case_fault_key_exchange_run(void *);
void spdm_test_case_fault_key_exchange_teardown(void *);

static void *rsp;
static uint8_t req_buffer[65536], rsp_buffer[65536];
static uint8_t request_wire[65536], response_wire[65536], first_kex[65536];
static size_t request_size, response_size, first_kex_size;
static uint8_t version;
static const char *mode;
static unsigned failures, assertions, teardown_calls, target_kex, target_cert;
static unsigned target_finish, versions, active_after_duplicate;
static bool target;
static uint16_t first_rsp_id;
static unsigned marker;

static bool is_mode(const char *name) { return strcmp(mode, name) == 0; }

bool libspdm_read_input_file(const char *name, void **data, size_t *size)
{
  FILE *f = fopen(name, "rb");
  long n;
  if (!f) return false;
  assert(fseek(f, 0, SEEK_END) == 0);
  n = ftell(f); assert(n > 0); rewind(f);
  *size = (size_t)n; *data = malloc(*size); assert(*data);
  assert(fread(*data, 1, *size, f) == *size); fclose(f);
  return true;
}
bool libspdm_write_output_file(const char *name, const void *data, size_t size)
{ (void)name; (void)data; (void)size; return false; }
void teeio_debug_print(int level, const char *fmt, ...)
{
  va_list args; (void)level; va_start(args, fmt); vprintf(fmt, args); va_end(args);
}
void teeio_assert(const char *file, int line, const char *desc)
{ fprintf(stderr, "%s:%d %s\n", file, line, desc); abort(); }
bool teeio_record_assertion_result(int cls, int id, int a,
  ide_common_test_case_assertion_type_t type, teeio_test_result_t result, const char *fmt, ...)
{
  va_list args; (void)cls; (void)id; (void)a; (void)type;
  assertions++; failures += result == TEEIO_TEST_RESULT_FAILED;
  va_start(args, fmt); vprintf(fmt, args); va_end(args); puts(""); return true;
}
void spdm_test_case_common_teardown(void *ctx) { (void)ctx; teardown_calls++; }
static void set_local(void *ctx, libspdm_data_type_t type, const void *data, size_t size)
{
  libspdm_data_parameter_t p = {0}; p.location = LIBSPDM_DATA_LOCATION_LOCAL;
  assert(libspdm_set_data(ctx, type, &p, data, size) == LIBSPDM_STATUS_SUCCESS);
}
bool teeio_spdm_apply_version_override(void *ctx)
{
  spdm_version_number_t v = (spdm_version_number_t)version << SPDM_VERSION_NUMBER_SHIFT_BIT;
  set_local(ctx, LIBSPDM_DATA_SPDM_VERSION, &v, sizeof(v)); return true;
}
void teeio_spdm_log_negotiated_version(void *ctx) { (void)ctx; }
static unsigned session_count(void *ctx)
{
  unsigned count = 0; size_t i;
  for (i = 0; i < LIBSPDM_MAX_SESSION_COUNT; i++)
    count += ((libspdm_context_t *)ctx)->session_info[i].session_id != INVALID_SESSION_ID;
  return count;
}
static libspdm_return_t acquire(void *ctx, void **buf)
{ *buf = ctx == rsp ? rsp_buffer : req_buffer; return LIBSPDM_STATUS_SUCCESS; }
static void release(void *ctx, const void *buf) { (void)ctx; (void)buf; }
static libspdm_return_t send_msg(void *ctx, size_t size, const void *msg, uint64_t timeout)
{
  const uint8_t *b = msg; (void)timeout;
  assert(size < sizeof(request_wire));
  if (ctx == rsp) { memcpy(response_wire, msg, size); response_size = size; }
  else {
    assert(((libspdm_context_t *)ctx)->app_context_data_ptr == &marker);
    if (b[2] == PCI_DOE_DATA_OBJECT_TYPE_SPDM && b[9] == SPDM_GET_VERSION) {
      versions++;
      if (target) target = false; /* Start of recovery, not target traffic. */
      if (is_mode("recovery-fail") && versions == 3) return LIBSPDM_STATUS_SEND_FAIL;
      if (is_mode("setup-fail") && versions == 1) return LIBSPDM_STATUS_SEND_FAIL;
    }
    if (target && b[2] == PCI_DOE_DATA_OBJECT_TYPE_SPDM && b[9] == SPDM_KEY_EXCHANGE) {
      target_kex++;
      if (target_kex == 1) { memcpy(first_kex, msg, size); first_kex_size = size; }
      else { assert(size == first_kex_size); assert(memcmp(first_kex, msg, size) == 0); }
    }
    memcpy(request_wire, msg, size); request_size = size;
  }
  return LIBSPDM_STATUS_SUCCESS;
}
static libspdm_return_t receive_msg(void *ctx, size_t *size, void **msg, uint64_t timeout)
{
  (void)timeout;
  if (ctx == rsp) { assert(*size >= request_size); memcpy(*msg, request_wire, request_size); *size = request_size; }
  else {
    libspdm_return_t status;
    assert(((libspdm_context_t *)ctx)->app_context_data_ptr == &marker);
    if (target && target_kex == 2 && is_mode("second-timeout")) return LIBSPDM_STATUS_RECEIVE_FAIL;
    response_size = 0;
    status = libspdm_responder_dispatch_message(rsp);
    if (status != LIBSPDM_STATUS_SUCCESS || response_size == 0) return LIBSPDM_STATUS_RECEIVE_FAIL;
    if (target && target_kex == 2 && response_wire[2] == PCI_DOE_DATA_OBJECT_TYPE_SPDM &&
      response_wire[9] == SPDM_KEY_EXCHANGE_RSP) active_after_duplicate = session_count(rsp);
    if (target && target_kex == 1 && is_mode("truncated-first")) {
      response_size = 48; libspdm_write_uint32(response_wire + 4, 12);
    }
    if (target && target_kex == 2 && is_mode("wrong-version")) response_wire[8] ^= 1;
    if (target && target_kex == 2 && is_mode("session-limit")) {
      response_size = 12; libspdm_write_uint32(response_wire + 4, 3);
      response_wire[9] = SPDM_ERROR; response_wire[10] = SPDM_ERROR_CODE_SESSION_LIMIT_EXCEEDED;
    }
    assert(*size >= response_size); memcpy(*msg, response_wire, response_size); *size = response_size;
  }
  return LIBSPDM_STATUS_SUCCESS;
}
static libspdm_return_t trace_encode(void *ctx, const uint32_t *sid, bool app, bool request,
  size_t size, void *msg, size_t *out_size, void **out)
{
  uint8_t *b = msg;
  if (!app && size >= 4 && ctx == rsp && target && b[1] == SPDM_KEY_EXCHANGE_RSP) {
    if (target_kex == 1) first_rsp_id = libspdm_read_uint16(b + 4);
    if (target_kex == 2 && is_mode("same-id")) libspdm_write_uint16(b + 4, first_rsp_id);
    /* P384 signature immediately precedes the 48-byte verify_data. */
    if (is_mode("bad-signature") || (target_kex == 2 && is_mode("duplicate-bad-signature"))) {
      assert(size > 49); b[size - 49] ^= 1;
    }
  }
  return libspdm_transport_pci_doe_encode_message(ctx, sid, app, request, size, msg, out_size, out);
}
/* Observe decrypted requests at the responder, including FINISH. */
static libspdm_return_t trace_decode(void *ctx, uint32_t **sid, bool *app, bool request,
  size_t in_size, void *in, size_t *size, void **msg)
{
  libspdm_return_t status = libspdm_transport_pci_doe_decode_message(ctx, sid, app, request, in_size, in, size, msg);
  if (status == LIBSPDM_STATUS_SUCCESS && target && !*app && *size >= 4) {
    const uint8_t *b = *msg;
    target_cert += b[1] == SPDM_GET_CERTIFICATE;
    target_finish += b[1] == SPDM_FINISH;
  }
  return status;
}
static void init_context(void *ctx)
{
  size_t size; void *scratch;
  spdm_version_number_t v = SPDM_MESSAGE_VERSION_11 << SPDM_VERSION_NUMBER_SHIFT_BIT;
  libspdm_init_context(ctx);
  libspdm_register_device_io_func(ctx, send_msg, receive_msg);
  libspdm_register_device_buffer_func(ctx, LIBSPDM_SENDER_BUFFER_SIZE, LIBSPDM_RECEIVER_BUFFER_SIZE,
    acquire, release, acquire, release);
  libspdm_register_transport_layer_func(ctx, LIBSPDM_MAX_SPDM_MSG_SIZE,
    LIBSPDM_TRANSPORT_HEADER_SIZE, LIBSPDM_TRANSPORT_TAIL_SIZE, trace_encode,
    ctx == rsp ? trace_decode : libspdm_transport_pci_doe_decode_message);
  size = libspdm_get_sizeof_required_scratch_buffer(ctx); scratch = calloc(1, size); assert(scratch);
  libspdm_set_scratch_buffer(ctx, scratch, size);
  teeio_spdm_apply_version_override(ctx);
  set_local(ctx, LIBSPDM_DATA_SECURED_MESSAGE_VERSION, &v, sizeof(v));
}
static void setup_responder(void)
{
  uint8_t n8 = 0; uint16_t n16; uint32_t n32; void *chain; size_t size;
  set_local(rsp, LIBSPDM_DATA_CAPABILITY_CT_EXPONENT, &n8, sizeof(n8));
  n32 = SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_CERT_CAP | SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_ENCRYPT_CAP |
    SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_MAC_CAP | SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_KEY_EX_CAP |
    SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_CHUNK_CAP;
  set_local(rsp, LIBSPDM_DATA_CAPABILITY_FLAGS, &n32, sizeof(n32));
  n32 = SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_384; set_local(rsp, LIBSPDM_DATA_BASE_HASH_ALGO, &n32, sizeof(n32));
  n32 = SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384; set_local(rsp, LIBSPDM_DATA_BASE_ASYM_ALGO, &n32, sizeof(n32));
  n16 = SPDM_ALGORITHMS_DHE_NAMED_GROUP_SECP_384_R1; set_local(rsp, LIBSPDM_DATA_DHE_NAME_GROUP, &n16, sizeof(n16));
  n16 = SPDM_ALGORITHMS_AEAD_CIPHER_SUITE_AES_256_GCM; set_local(rsp, LIBSPDM_DATA_AEAD_CIPHER_SUITE, &n16, sizeof(n16));
  n16 = SPDM_ALGORITHMS_KEY_SCHEDULE_SPDM; set_local(rsp, LIBSPDM_DATA_KEY_SCHEDULE, &n16, sizeof(n16));
  n8 = SPDM_ALGORITHMS_OPAQUE_DATA_FORMAT_1; set_local(rsp, LIBSPDM_DATA_OTHER_PARAMS_SUPPORT, &n8, sizeof(n8));
  n8 = 0; set_local(rsp, LIBSPDM_DATA_MEASUREMENT_SPEC, &n8, sizeof(n8));
  n8 = 1; set_local(rsp, LIBSPDM_DATA_LOCAL_SUPPORTED_SLOT_MASK, &n8, sizeof(n8));
  assert(libspdm_read_responder_public_certificate_chain(SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_384,
    SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384, &chain, &size, NULL, NULL));
  set_local(rsp, LIBSPDM_DATA_LOCAL_PUBLIC_CERT_CHAIN, chain, size);
}
int main(int argc, char **argv)
{
  IDE_TEST_FAULT_CONFIG config = {0}; teeio_spdm_test_context_t test = {0};
  teeio_fault_rule_t *rule = &config.rules[0]; bool setup, duplicate, expected_pass;
  void *root; size_t root_size;
  assert(argc == 3); version = (uint8_t)strtoul(argv[1], NULL, 16); mode = argv[2];
  duplicate = !(is_mode("cache") || is_mode("missing-trust") || is_mode("bad-signature") || is_mode("setup-fail"));
  expected_pass = is_mode("cache") || is_mode("duplicate");
  config.rule_count = 1; rule->id = 1;
  strcpy(rule->scenario, duplicate ? "B4_duplicate_key_exchange" : "B4_key_exchange_before_certificate");
  rule->doe_type = TEEIO_FAULT_DOE_TYPE_PLAIN_SPDM; rule->spdm_code = SPDM_KEY_EXCHANGE;
  rule->occurrence = is_mode("no-fire") ? 2 : 1;
  rule->action = is_mode("wrong-action") ? TEEIO_FAULT_ACTION_DROP : TEEIO_FAULT_ACTION_DUPLICATE;
  teeio_fault_init(&config);
  rsp = calloc(1, libspdm_get_context_size()); assert(rsp); init_context(rsp); setup_responder();
  test.spdm_context = calloc(1, libspdm_get_context_size()); assert(test.spdm_context); init_context(test.spdm_context);
  ((libspdm_context_t *)test.spdm_context)->app_context_data_ptr = &marker;
  if (!is_mode("missing-trust")) {
    assert(libspdm_read_input_file("ecp384/ca.cert.der", &root, &root_size));
    set_local(test.spdm_context, LIBSPDM_DATA_PEER_PUBLIC_ROOT_CERT, root, root_size);
  }
  setup = spdm_test_case_fault_key_exchange_setup(&test);
  assert(teeio_fault_fire_count() == 0);
  if (is_mode("missing-trust") || is_mode("setup-fail")) assert(!setup);
  else {
    assert(setup); assert(versions == 2); assert(session_count(rsp) == 0);
    target = true;
    spdm_test_case_fault_key_exchange_run(&test);
    assert(assertions == 1); assert((failures == 0) == expected_pass);
    assert(target_cert == 0);
    if (expected_pass) {
      assert(target_finish == 1);
      assert(target_kex == (duplicate ? 2u : 1u));
      assert(teeio_fault_fire_count() == (duplicate ? 1u : 0u));
      if (duplicate) assert(active_after_duplicate == 2);
      if (duplicate) assert(strstr(teeio_fault_result_record(),
        "\"actual\":\"duplicate_unique_session_ids_then_recover\"") != NULL);
      else assert(strstr(teeio_fault_result_record(), "\"fired\":false") != NULL);
    }
    assert(session_count(test.spdm_context) == 0);
    assert(session_count(rsp) == 0);
  }
  spdm_test_case_fault_key_exchange_teardown(&test);
  assert(teardown_calls == 1);
  assert(session_count(rsp) == 0 && session_count(test.spdm_context) == 0);
  assert(((libspdm_context_t *)test.spdm_context)->app_context_data_ptr == &marker);
  assert(((libspdm_context_t *)test.spdm_context)->send_message == send_msg);
  assert(((libspdm_context_t *)test.spdm_context)->receive_message == receive_msg);
  assert(((libspdm_context_t *)test.spdm_context)->transport_encode_message == trace_encode);
  assert(((libspdm_context_t *)test.spdm_context)->transport_decode_message == libspdm_transport_pci_doe_decode_message);
  puts("PASS: B4 expectation, actual wire sequence, session cleanup and callback restoration");
  return 0;
}