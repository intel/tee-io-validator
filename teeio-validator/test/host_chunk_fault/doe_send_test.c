/* Copyright 2026 Intel. SPDX-License-Identifier: BSD-3-Clause */
/* Exercise the real requester send path, not a direct encoder/handler call.
 * Only PCI register access and PCAP output are replaced by in-memory sinks. */
#define TEEIO_CHUNK_FAULT_REAL_DOE
#define main chunk_fault_tests_main
#include "chunk_fault_test.c"
#undef main
#include "internal/libspdm_requester_lib.h"

int m_dev_fp = 1;
uint32_t g_doe_extended_offset = 0x100;
bool g_doe_log = false;

static uint32_t mailbox[128];
static size_t mailbox_count;
static unsigned go_count, pcap_count;
static bool reply_early;

static libspdm_return_t scripted_ack(void *context, size_t *size,
                                     void **message, uint64_t timeout)
{
  const uint8_t *sent = (const uint8_t *)mailbox + 8;
  uint8_t *doe = *message;
  size_t header = sent[0] >= SPDM_MESSAGE_VERSION_14 ? 8 : 6;
  size_t total = (8 + header + (reply_early ? 4 : 0) + 3) & ~(size_t)3;
  assert(go_count == 1 && *size >= total);
  memset(doe, 0, total);
  doe[0] = 1;
  doe[2] = PCI_DOE_DATA_OBJECT_TYPE_SPDM;
  libspdm_write_uint32(doe + 4, (uint32_t)(total / 4));
  doe[8] = sent[0];
  doe[9] = SPDM_CHUNK_SEND_ACK;
  doe[10] = reply_early;
  doe[11] = sent[3];
  if (reply_early) {
    doe[8 + header] = sent[0];
    doe[9 + header] = SPDM_ERROR;
    doe[10 + header] = SPDM_ERROR_CODE_INVALID_REQUEST;
  }
  *size = total;
  return LIBSPDM_STATUS_SUCCESS;
}

uint32_t device_pci_read_32(uint32_t offset, int fd)
{
  assert(fd == m_dev_fp);
  assert(offset == g_doe_extended_offset + 0x08 ||
         offset == g_doe_extended_offset + 0x0c);
  return 0; /* No BUSY/ERROR and no pending response. */
}

void device_pci_write_32(uint32_t offset, uint32_t value, int fd)
{
  assert(fd == m_dev_fp);
  if (offset == g_doe_extended_offset + 0x10) {
    assert(go_count == 0 && mailbox_count < LIBSPDM_ARRAY_SIZE(mailbox));
    mailbox[mailbox_count++] = value;
  } else {
    assert(offset == g_doe_extended_offset + 0x08);
    assert(value == 0x80000000); /* GO, not an abort/retry. */
    go_count++;
  }
}

void append_pcap_packet_data(const void *header, size_t header_size,
                            const void *data, size_t size)
{
  assert(header == NULL && header_size == 0);
  assert(go_count == 1 && pcap_count++ == 0);
  assert(size == mailbox_count * sizeof(uint32_t));
  assert(memcmp(data, mailbox, size) == 0);
}

static void mailbox_boundary(uint8_t version, uint32_t limit, bool inject)
{
  teeio_spdm_test_context_t test;
  teeio_fault_test_buffer_t *state = fixture(&test,
    TEEIO_FAULT_OVERSIZED_CHUNK_SCENARIO, SPDM_CHUNK_SEND,
    TEEIO_FAULT_ACTION_EXTEND);
  libspdm_context_t *spdm = test.spdm_context;
  libspdm_context_t *responder;
  void *sender, *scratch, *plain;
  size_t sender_size, scratch_size, plain_size;
  uint8_t original[512], response[64], large[4096];
  size_t response_size = sizeof(response);
  uint32_t *session = NULL;
  bool app = false;
  size_t expected_size = limit + 8 + (inject ? 8 : 0);

  assert(limit <= sizeof(original) && expected_size <= sizeof(mailbox));
  libspdm_register_device_io_func(spdm, device_doe_send_message,
                                  device_doe_receive_message);
  libspdm_register_transport_layer_func(spdm, LIBSPDM_MAX_SPDM_MSG_SIZE,
    LIBSPDM_TRANSPORT_HEADER_SIZE, LIBSPDM_TRANSPORT_TAIL_SIZE,
    chunk_encode, chunk_decode);
  libspdm_register_device_buffer_func(spdm,
    LIBSPDM_SENDER_BUFFER_SIZE, LIBSPDM_RECEIVER_BUFFER_SIZE,
    spdm_device_acquire_sender_buffer, spdm_device_release_sender_buffer,
    spdm_device_acquire_receiver_buffer, spdm_device_release_receiver_buffer);
  scratch_size = libspdm_get_sizeof_required_scratch_buffer(spdm);
  scratch = calloc(1, scratch_size);
  assert(scratch != NULL);
  libspdm_set_scratch_buffer(spdm, scratch, scratch_size);
  spdm->connection_info.version = (spdm_version_number_t)version <<
    SPDM_VERSION_NUMBER_SHIFT_BIT;
  spdm->connection_info.capability.data_transfer_size = limit;
  state->version = version;
  state->data_transfer_size = limit;
  state->armed = inject;

  assert(libspdm_acquire_sender_buffer(spdm, &sender_size, &sender) ==
    LIBSPDM_STATUS_SUCCESS);
  make_send((uint8_t *)sender + LIBSPDM_TRANSPORT_HEADER_SIZE,
            version, 0, false, limit);
  memcpy(original, (uint8_t *)sender + LIBSPDM_TRANSPORT_HEADER_SIZE, limit);
  prepare_large_send(spdm, original, large);
  mailbox_count = go_count = pcap_count = 0;
  memset(mailbox, 0xa5, sizeof(mailbox));
  assert(libspdm_send_request(spdm, NULL, false, limit,
    (uint8_t *)sender + LIBSPDM_TRANSPORT_HEADER_SIZE) == LIBSPDM_STATUS_SUCCESS);
  assert(go_count == 1 && pcap_count == 1);
  assert(mailbox_count * sizeof(uint32_t) == expected_size);
  assert(mailbox[1] == expected_size / sizeof(uint32_t));
  assert(((uint8_t *)mailbox)[2] == PCI_DOE_DATA_OBJECT_TYPE_SPDM);
  assert(memcmp((uint8_t *)mailbox + 8, original, 8) == 0);
  assert(memcmp((uint8_t *)mailbox + 20, original + 12, limit - 12) == 0);
  assert(teeio_fault_fire_count() == (inject ? 1 : 0));
  /* The second FI sees real DOE type 1, not the plaintext selector 0xfd. */
  assert(config.rules[0].match_count == (inject ? 1 : 0));
  if (inject) {
    assert(libspdm_read_uint32((uint8_t *)mailbox + 16) == limit - 16 + 8);
    assert(memcmp((uint8_t *)mailbox + 24, large, limit - 16 + 8) == 0);
    assert(spdm->chunk_context.send.chunk_bytes_transferred == limit - 16 + 8);
  } else {
    assert(memcmp((uint8_t *)mailbox + 8, original, limit) == 0);
  }
  libspdm_release_sender_buffer(spdm);

  /* Decode the bytes written to the mailbox, not the encoder's source buffer. */
  plain_size = sizeof(mailbox);
  assert(libspdm_transport_pci_doe_decode_message(spdm, &session, &app,
    true, expected_size, mailbox, &plain_size, &plain) == LIBSPDM_STATUS_SUCCESS);
  assert(session == NULL && !app && plain_size == expected_size - 8);
  responder = calloc(1, sizeof(*responder));
  assert(responder != NULL);
  responder->version = LIBSPDM_CONTEXT_STRUCT_VERSION;
  responder->connection_info.version = spdm->connection_info.version;
  responder->connection_info.connection_state = LIBSPDM_CONNECTION_STATE_NEGOTIATED;
  responder->local_context.capability.flags = SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_CHUNK_CAP;
  responder->connection_info.capability.flags = SPDM_GET_CAPABILITIES_REQUEST_FLAGS_CHUNK_CAP;
  responder->local_context.capability.data_transfer_size = limit;
  responder->local_context.capability.max_spdm_msg_size = 8192;
  responder->scratch_buffer_size = 65536;
  responder->scratch_buffer = calloc(1, responder->scratch_buffer_size);
  assert(responder->scratch_buffer != NULL);
  assert(libspdm_get_response_chunk_send(responder, plain_size, plain,
    &response_size, response) == LIBSPDM_STATUS_SUCCESS);
  assert(response[1] == SPDM_CHUNK_SEND_ACK);
  if (inject) {
    observe(&test, response, response_size);
    assert(state->first_early_invalid_request && !state->invalid);
    assert(!responder->chunk_context.send.chunk_in_use);
  } else {
    assert(response[2] == 0 && responder->chunk_context.send.chunk_in_use);
  }
  free(responder->scratch_buffer);
  free(responder);
  cleanup(&test);
  free(scratch);
  printf("PASS: SPDM 1.%u limit=%u inject=%u mailbox_dw=%zu PCAP exact\n",
         version & 0xf, limit, inject, mailbox_count);
}

static void real_chunk_loop(uint8_t version, uint32_t limit, bool early)
{
  teeio_spdm_test_context_t test;
  teeio_fault_test_buffer_t *state = fixture(&test,
    TEEIO_FAULT_OVERSIZED_CHUNK_SCENARIO, SPDM_CHUNK_SEND, TEEIO_FAULT_ACTION_EXTEND);
  libspdm_context_t *spdm = test.spdm_context;
  uint8_t request[1607];
  void *scratch;
  size_t scratch_size;
  libspdm_return_t status;
  libspdm_register_device_io_func(spdm, device_doe_send_message, scripted_ack);
  libspdm_register_transport_layer_func(spdm, LIBSPDM_MAX_SPDM_MSG_SIZE,
    LIBSPDM_TRANSPORT_HEADER_SIZE, LIBSPDM_TRANSPORT_TAIL_SIZE, chunk_encode, chunk_decode);
  libspdm_register_device_buffer_func(spdm,
    LIBSPDM_SENDER_BUFFER_SIZE, LIBSPDM_RECEIVER_BUFFER_SIZE,
    spdm_device_acquire_sender_buffer, spdm_device_release_sender_buffer,
    spdm_device_acquire_receiver_buffer, spdm_device_release_receiver_buffer);
  scratch_size = libspdm_get_sizeof_required_scratch_buffer(spdm);
  scratch = calloc(1, scratch_size);
  assert(scratch != NULL);
  libspdm_set_scratch_buffer(spdm, scratch, scratch_size);
  spdm->connection_info.version = (spdm_version_number_t)version << SPDM_VERSION_NUMBER_SHIFT_BIT;
  spdm->connection_info.capability.data_transfer_size = limit;
  spdm->connection_info.capability.flags = SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_CHUNK_CAP;
  spdm->local_context.capability.flags = SPDM_GET_CAPABILITIES_REQUEST_FLAGS_CHUNK_CAP;
  state->version = version;
  state->data_transfer_size = limit;
  state->armed = true;
  for (size_t i = 0; i < sizeof(request); i++) request[i] = (uint8_t)(i * 19 + 5);
  request[0] = version;
  request[1] = SPDM_SET_CERTIFICATE;
  request[2] = request[3] = 0;
  mailbox_count = go_count = pcap_count = 0;
  reply_early = early;
  status = libspdm_send_spdm_request(spdm, NULL, sizeof(request), request);
  assert(go_count == 1 && pcap_count == 1 && state->send_count == 1 && state->ack_count == 1);
  assert(state->first_early_invalid_request == early && !state->invalid);
  assert(teeio_fault_fire_count() == 1);
  assert(mailbox_count * 4 == limit + 16);
  assert(libspdm_read_uint32((uint8_t *)mailbox + 16) == limit - 8);
  assert(libspdm_read_uint32((uint8_t *)mailbox + 20) == sizeof(request));
  assert(memcmp((uint8_t *)mailbox + 24, request, limit - 8) == 0);
  if (!early) {
    assert(LIBSPDM_STATUS_IS_ERROR(status));
    assert(!spdm->chunk_context.send.chunk_in_use);
    assert(strstr(teeio_fault_result_record(), "first_chunk_not_early_invalid_request"));
  }
  cleanup(&test);
  free(scratch);
  printf("PASS: real chunk loop 1.%u limit=%u early=%u sends=1 payload preserved\n",
         version & 0xf, limit, early);
}

int main(void)
{
  const uint8_t versions[] = {SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_14};
  const uint32_t limits[] = {256, 476};
  size_t version, limit;
  for (version = 0; version < LIBSPDM_ARRAY_SIZE(versions); version++) {
    for (limit = 0; limit < LIBSPDM_ARRAY_SIZE(limits); limit++) {
      mailbox_boundary(versions[version], limits[limit], false);
      mailbox_boundary(versions[version], limits[limit], true);
      real_chunk_loop(versions[version], limits[limit], false);
      real_chunk_loop(versions[version], limits[limit], true);
    }
  }
  return 0;
}