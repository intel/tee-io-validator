/**
 *  Copyright Notice:
 *  Copyright 2026 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

/* Compiles the real DOE mailbox code as C with product flags and scripts the
 * device for gtest: the first request is answered, later ones are not. */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../library/spdmlib/pci_doe.c"

#define DOE_BASE 0x100
#define STATUS_READ_LIMIT 1000000

int m_dev_fp = 1;
uint32_t g_doe_extended_offset = DOE_BASE;
bool g_doe_log = false;

static const uint32_t m_response[] = {0x00010001, 4, 0x00000412, 0};
static bool m_response_pending;
static size_t m_read_index;
static unsigned m_go_count;
static unsigned m_abort_count;
static unsigned long m_status_reads;
static uint64_t m_slept_us;
static bool m_poll_limit_hit;

uint32_t device_pci_read_32(uint32_t offset, int fp)
{
    (void)fp;
    if (offset == DOE_BASE + PCI_EXPRESS_REG_DOE_STATUS_OFFSET) {
        /* Break a non-terminating poll loop so gtest can report it. */
        if (++m_status_reads > STATUS_READ_LIMIT && !m_poll_limit_hit) {
            m_poll_limit_hit = true;
            m_response_pending = true;
            m_read_index = 0;
        }
        return m_response_pending ? PCI_EXPRESS_REG_DOE_STATUS_DOE_READY : 0;
    }
    if (offset == DOE_BASE + PCI_EXPRESS_REG_DOE_READ_DATA_MAILBOX_OFFSET) {
        return m_response[m_read_index];
    }
    return 0;
}

void device_pci_write_32(uint32_t offset, uint32_t data, int fp)
{
    (void)fp;
    if (offset == DOE_BASE + PCI_EXPRESS_REG_DOE_CONTROL_OFFSET) {
        if ((data & PCI_EXPRESS_REG_DOE_CONTROL_DOE_ABORT) != 0) {
            m_abort_count++;
        } else if ((data & PCI_EXPRESS_REG_DOE_CONTROL_DOE_GO) != 0) {
            m_response_pending = ++m_go_count == 1;
            m_read_index = 0;
        }
    } else if (offset == DOE_BASE + PCI_EXPRESS_REG_DOE_READ_DATA_MAILBOX_OFFSET) {
        if (++m_read_index == LIBSPDM_ARRAY_SIZE(m_response)) {
            m_response_pending = false;
        }
    }
}

void libspdm_sleep(uint64_t duration)
{
    m_slept_us += duration;
}

void libspdm_zero_mem(void *buffer, size_t length)
{
    memset(buffer, 0, length);
}

uint16_t libspdm_read_uint16(const uint8_t *buffer)
{
    return (uint16_t)(buffer[0] | (buffer[1] << 8));
}

uint32_t libspdm_read_uint32(const uint8_t *buffer)
{
    return (uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8) |
           ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24);
}

void append_pcap_packet_data(const void *header, size_t header_size,
                             const void *data, size_t size)
{
    (void)header; (void)header_size; (void)data; (void)size;
}

void teeio_debug_print(int debug_level, const char *format, ...)
{
    (void)debug_level; (void)format;
}

void teeio_assert(const char *file_name, int line_number, const char *description)
{
    fprintf(stderr, "ASSERT %s:%d %s\n", file_name, line_number, description);
    exit(1);
}

void *libspdm_get_secured_message_context_via_session_id(void *spdm_context,
                                                         uint32_t session_id)
{
    (void)spdm_context; (void)session_id;
    return NULL;
}

bool libspdm_secured_message_export_session_keys(void *context, void *keys,
                                                 size_t *keys_size)
{
    (void)context; (void)keys; (void)keys_size;
    return false;
}

bool libspdm_secured_message_import_session_keys(void *context, const void *keys,
                                                 size_t keys_size)
{
    (void)context; (void)keys; (void)keys_size;
    return false;
}

libspdm_return_t libspdm_transport_pci_doe_encode_message(
    void *spdm_context, const uint32_t *session_id, bool is_app_message,
    bool is_request_message, size_t message_size, void *message,
    size_t *transport_message_size, void **transport_message)
{
    (void)spdm_context; (void)session_id; (void)is_app_message;
    (void)is_request_message; (void)message_size; (void)message;
    (void)transport_message_size; (void)transport_message;
    abort();
}

libspdm_return_t libspdm_transport_pci_doe_decode_message(
    void *spdm_context, uint32_t **session_id, bool *is_app_message,
    bool is_request_message, size_t transport_message_size,
    void *transport_message, size_t *message_size, void **message)
{
    (void)spdm_context; (void)session_id; (void)is_app_message;
    (void)is_request_message; (void)transport_message_size;
    (void)transport_message; (void)message_size; (void)message;
    abort();
}

libspdm_return_t pci_doe_discovery(const void *pci_doe_context,
                                   pci_doe_data_object_protocol_t *data_object_protocol,
                                   size_t *data_object_protocol_size,
                                   uint8_t version)
{
    (void)pci_doe_context; (void)data_object_protocol;
    (void)data_object_protocol_size; (void)version;
    abort();
}

void pci_doe_test_reset(void)
{
    teeio_fault_transport_reset();
    m_response_pending = false;
    m_read_index = 0;
    m_go_count = 0;
    m_abort_count = 0;
    m_status_reads = 0;
    m_slept_us = 0;
    m_poll_limit_hit = false;
}

bool pci_doe_test_send(const void *request, size_t request_size, uint64_t timeout_us)
{
    return !LIBSPDM_STATUS_IS_ERROR(
        device_doe_send_message(NULL, request_size, request, timeout_us));
}

bool pci_doe_test_receive(void *response, size_t *response_size, uint64_t timeout_us)
{
    return !LIBSPDM_STATUS_IS_ERROR(
        device_doe_receive_message(NULL, response_size, &response, timeout_us));
}

const uint32_t *pci_doe_test_scripted_response(size_t *size)
{
    *size = sizeof(m_response);
    return m_response;
}

unsigned pci_doe_test_go_count(void) { return m_go_count; }
unsigned pci_doe_test_abort_count(void) { return m_abort_count; }
uint64_t pci_doe_test_slept_us(void) { return m_slept_us; }
bool pci_doe_test_poll_limit_hit(void) { return m_poll_limit_hit; }
