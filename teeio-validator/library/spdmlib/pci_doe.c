/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include "teeio_validator.h"
#include "teeio_spdmlib.h"
#include "teeio_fault_injection.h"
#include "pcap.h"

/* PCI Express - begin */
#define PCI_EXPRESS_EXTENDED_CAPABILITY_DOE_ID 0x002E
#define PCI_EXPRESS_EXTENDED_CAPABILITY_DOE_VER1 0x1

#define PCI_EXPRESS_REG_DOE_CAPABILITIES_OFFSET 0x04
#define   PCI_EXPRESS_REG_DOE_CAPABILITIES_DOE_INT_SUPPORT 0x00000001
#define PCI_EXPRESS_REG_DOE_CONTROL_OFFSET 0x08
#define   PCI_EXPRESS_REG_DOE_CONTROL_DOE_ABORT 0x00000001
#define   PCI_EXPRESS_REG_DOE_CONTROL_DOE_INT_EN 0x00000002
#define   PCI_EXPRESS_REG_DOE_CONTROL_DOE_GO 0x80000000
#define PCI_EXPRESS_REG_DOE_STATUS_OFFSET 0x0C
#define   PCI_EXPRESS_REG_DOE_STATUS_DOE_BUSY 0x00000001
#define   PCI_EXPRESS_REG_DOE_STATUS_DOE_INT_STS 0x00000002
#define   PCI_EXPRESS_REG_DOE_STATUS_DOE_ERROR 0x00000004
#define   PCI_EXPRESS_REG_DOE_STATUS_DOE_READY 0x80000000
#define PCI_EXPRESS_REG_DOE_WRITE_DATA_MAILBOX_OFFSET 0x10
#define PCI_EXPRESS_REG_DOE_READ_DATA_MAILBOX_OFFSET 0x14

#define PCI_EXPRESS_DOE_MAILBOX_TIMEOUT 300000000   // 30 second, enough for debug device to respond
/* PCI Express - end */

extern int m_dev_fp;
extern uint32_t g_doe_extended_offset;
extern bool g_doe_log;
void *m_pci_doe_context;

// The must supported pci_doe_data_object_type for TEEIO-Validator
uint8_t m_pci_doe_data_object_type[] = {
    PCI_DOE_DATA_OBJECT_TYPE_DOE_DISCOVERY,
    PCI_DOE_DATA_OBJECT_TYPE_SPDM,
    PCI_DOE_DATA_OBJECT_TYPE_SECURED_SPDM
};


bool m_send_receive_buffer_acquired = false;
uint8_t m_send_receive_buffer[LIBSPDM_RECEIVER_BUFFER_SIZE];
size_t m_send_receive_buffer_size;
static bool m_fault_skip_receive;
static uint8_t m_fault_deferred_send[TEEIO_FAULT_MAX_MESSAGE_SIZE];
static size_t m_fault_deferred_send_size;
static uint8_t m_fault_plain_send[TEEIO_FAULT_MAX_MESSAGE_SIZE];
static uint8_t m_fault_duplicate_response[TEEIO_FAULT_MAX_MESSAGE_SIZE];
static bool m_fault_draining_duplicate;
static void *m_fault_secured_message_context;
static uint8_t m_fault_session_keys[TEEIO_FAULT_MAX_MESSAGE_SIZE];
static size_t m_fault_session_keys_size;

static void snapshot_fault_session(void *spdm_context,
                                   const uint32_t *session_id)
{
    m_fault_secured_message_context = NULL;
    m_fault_session_keys_size = 0;
    if (session_id == NULL ||
        !teeio_fault_same_session_recovery_configured()) {
        return;
    }
    m_fault_secured_message_context =
        libspdm_get_secured_message_context_via_session_id(spdm_context,
                                                            *session_id);
    if (m_fault_secured_message_context == NULL) {
        return;
    }
    libspdm_secured_message_export_session_keys(
        m_fault_secured_message_context, NULL, &m_fault_session_keys_size);
    if (m_fault_session_keys_size > sizeof(m_fault_session_keys) ||
        !libspdm_secured_message_export_session_keys(
            m_fault_secured_message_context, m_fault_session_keys,
            &m_fault_session_keys_size)) {
        m_fault_secured_message_context = NULL;
        m_fault_session_keys_size = 0;
    }
}

static bool restore_fault_session(void)
{
    if (m_fault_secured_message_context == NULL ||
        m_fault_session_keys_size == 0) {
        return false;
    }
    return libspdm_secured_message_import_session_keys(
        m_fault_secured_message_context, m_fault_session_keys,
        m_fault_session_keys_size);
}

static void record_spdm_actual(const uint8_t *message, size_t message_size)
{
    char actual[64];
    size_t chunk_header_size;

    if (message == NULL || message_size < sizeof(spdm_message_header_t)) {
        return;
    }
    if (message[1] == SPDM_CHUNK_SEND_ACK) {
        chunk_header_size = message[0] >= SPDM_MESSAGE_VERSION_14 ?
            sizeof(spdm_chunk_send_ack_response_14_t) :
            sizeof(spdm_chunk_send_ack_response_t);
        if (message_size <= chunk_header_size) {
            return;
        }
        message += chunk_header_size;
        message_size -= chunk_header_size;
        if (message_size < sizeof(spdm_message_header_t)) {
            return;
        }
    }
    if (message[1] == SPDM_ERROR) {
        snprintf(actual, sizeof(actual), "spdm_error_0x%02x", message[2]);
    } else {
        snprintf(actual, sizeof(actual), "spdm_response_0x%02x", message[1]);
    }
    teeio_fault_record_actual(actual);
}

static void set_fault_plain_header(uint8_t *message, size_t message_size,
                                   bool secured)
{
    uint32_t dword_length = (uint32_t)((message_size + 3) / 4);

    memset(message, 0, sizeof(pci_doe_data_object_header_t));
    message[2] = secured ? TEEIO_FAULT_DOE_TYPE_PLAIN_SECURED_SPDM :
                           TEEIO_FAULT_DOE_TYPE_PLAIN_SPDM;
    message[4] = (uint8_t)dword_length;
    message[5] = (uint8_t)(dword_length >> 8);
    message[6] = (uint8_t)(dword_length >> 16);
    message[7] = (uint8_t)(dword_length >> 24);
}

static bool apply_plain_send_fault(bool secured, void **message,
                                   size_t *message_size, uint8_t *plain_buffer)
{
    teeio_fault_result_t result;
    size_t framed_size;

    framed_size = *message_size + sizeof(pci_doe_data_object_header_t);
    if (framed_size > TEEIO_FAULT_MAX_MESSAGE_SIZE) {
        return false;
    }
    set_fault_plain_header(plain_buffer, framed_size, secured);
    memcpy(plain_buffer + sizeof(pci_doe_data_object_header_t),
           *message, *message_size);
    result = teeio_fault_apply(plain_buffer, framed_size,
                               TEEIO_FAULT_MAX_MESSAGE_SIZE);
    if (result.disposition == TEEIO_FAULT_DISPOSITION_PASS) {
        return true;
    }
    if ((result.disposition != TEEIO_FAULT_DISPOSITION_MUTATE &&
         result.disposition != TEEIO_FAULT_DISPOSITION_REPLAY) ||
        result.message_size < sizeof(pci_doe_data_object_header_t)) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR,
                     "Unsupported plaintext fault disposition: %s\n",
                     teeio_fault_audit_record()));
        return false;
    }

    TEEIO_DEBUG((TEEIO_DEBUG_WARN, "Plaintext fault injection: %s\n",
                 teeio_fault_audit_record()));
    memcpy(plain_buffer, result.message, result.message_size);
    *message = plain_buffer + sizeof(pci_doe_data_object_header_t);
    *message_size = result.message_size - sizeof(pci_doe_data_object_header_t);
    return true;
}

libspdm_return_t teeio_fault_transport_encode_message(
    void *spdm_context, const uint32_t *session_id, bool is_app_message,
    bool is_request_message, size_t message_size, void *message,
    size_t *transport_message_size, void **transport_message)
{
    if (!is_app_message &&
        !apply_plain_send_fault(session_id != NULL, &message, &message_size,
                                m_fault_plain_send)) {
        return LIBSPDM_STATUS_SEND_FAIL;
    }
    snapshot_fault_session(spdm_context, session_id);
    return libspdm_transport_pci_doe_encode_message(
        spdm_context, session_id, is_app_message, is_request_message,
        message_size, message, transport_message_size, transport_message);
}

libspdm_return_t teeio_fault_transport_decode_message(
    void *spdm_context, uint32_t **session_id, bool *is_app_message,
    bool is_request_message, size_t transport_message_size,
    void *transport_message, size_t *message_size, void **message)
{
    libspdm_return_t status;

    status = libspdm_transport_pci_doe_decode_message(
        spdm_context, session_id, is_app_message, is_request_message,
        transport_message_size, transport_message, message_size, message);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        teeio_fault_record_actual("decode_failed");
        return status;
    }
    if (!*is_app_message && teeio_fault_should_record_response()) {
        record_spdm_actual(*message, *message_size);
    }
    return LIBSPDM_STATUS_SUCCESS;
}

void teeio_fault_transport_reset(void)
{
    teeio_fault_reset();
    m_fault_skip_receive = false;
    m_fault_deferred_send_size = 0;
    memset(m_fault_deferred_send, 0, sizeof(m_fault_deferred_send));
    memset(m_fault_plain_send, 0, sizeof(m_fault_plain_send));
    memset(m_fault_duplicate_response, 0, sizeof(m_fault_duplicate_response));
    m_fault_draining_duplicate = false;
    m_fault_secured_message_context = NULL;
    m_fault_session_keys_size = 0;
    memset(m_fault_session_keys, 0, sizeof(m_fault_session_keys));
}

#define TEEIO_DOE_DEBUG(expression) \
    do {                            \
        if(g_doe_log) {             \
          TEEIO_DEBUG(expression);  \
        }                           \
    } while (false)

// more info please check file - new_cambria_core_regs_RWF_FM85.doc.xml
void check_pcie_advance_error()
{
    return;
}

uint32_t device_pci_doe_control_read_32 ()
{
    TEEIO_ASSERT(g_doe_extended_offset != 0);
    return device_pci_read_32 (g_doe_extended_offset + PCI_EXPRESS_REG_DOE_CONTROL_OFFSET, m_dev_fp);
}

void device_pci_doe_control_write_32 (uint32_t data)
{
    TEEIO_ASSERT(g_doe_extended_offset != 0);
    device_pci_write_32 (g_doe_extended_offset + PCI_EXPRESS_REG_DOE_CONTROL_OFFSET, data, m_dev_fp);
}

uint32_t device_pci_doe_status_read_32 ()
{
    TEEIO_ASSERT(g_doe_extended_offset != 0);
    return device_pci_read_32 (g_doe_extended_offset + PCI_EXPRESS_REG_DOE_STATUS_OFFSET, m_dev_fp);
}

void device_pci_doe_write_mailbox_write_32 (uint32_t data)
{
    TEEIO_ASSERT(g_doe_extended_offset != 0);
    device_pci_write_32 (g_doe_extended_offset + PCI_EXPRESS_REG_DOE_WRITE_DATA_MAILBOX_OFFSET, data, m_dev_fp);
}

uint32_t device_pci_doe_read_mailbox_read_32 ()
{
    TEEIO_ASSERT(g_doe_extended_offset != 0);
    return device_pci_read_32 (g_doe_extended_offset + PCI_EXPRESS_REG_DOE_READ_DATA_MAILBOX_OFFSET, m_dev_fp);
}

void device_pci_doe_read_mailbox_write_32 (uint32_t data)
{
    TEEIO_ASSERT(g_doe_extended_offset != 0);
    device_pci_write_32 (g_doe_extended_offset + PCI_EXPRESS_REG_DOE_READ_DATA_MAILBOX_OFFSET, data, m_dev_fp);
}

bool is_doe_error_asserted(){
    uint32_t doe_status = device_pci_doe_status_read_32 ();
    if ((doe_status & PCI_EXPRESS_REG_DOE_STATUS_DOE_ERROR) != 0){
        return true;
    }else{
        return false;
    }
}

bool is_doe_busy_asserted(){
    uint32_t doe_status = device_pci_doe_status_read_32 ();
    if ((doe_status & PCI_EXPRESS_REG_DOE_STATUS_DOE_BUSY) != 0){
        return true;
    }else{
        return false;
    }
}

bool is_doe_data_object_ready_asserted(){
    uint32_t doe_status = device_pci_doe_status_read_32 ();
    if ((doe_status & PCI_EXPRESS_REG_DOE_STATUS_DOE_READY) != 0){
        return true;
    }else{
        return false;
    }
}

void trigger_doe_abort(){
    uint32_t doe_control = device_pci_doe_control_read_32 ();
    doe_control |= PCI_EXPRESS_REG_DOE_CONTROL_DOE_ABORT;
    doe_control &= ~PCI_EXPRESS_REG_DOE_CONTROL_DOE_GO;
    device_pci_doe_control_write_32 (doe_control);
}

void trigger_doe_go(){
    uint32_t doe_control = device_pci_doe_control_read_32 ();
    doe_control &= ~PCI_EXPRESS_REG_DOE_CONTROL_DOE_ABORT;
    doe_control |= PCI_EXPRESS_REG_DOE_CONTROL_DOE_GO;
    device_pci_doe_control_write_32 (doe_control);
}

libspdm_return_t device_doe_send_message(
    void *spdm_context,
    size_t request_size,
    const void *request,
    uint64_t timeout)
{
    libspdm_return_t status;
    uint32_t index;
    uint64_t delay;
    uint32_t data_object_count;
    const uint32_t *data_object_buffer;
    const uint8_t *data_object_bytes;
    teeio_fault_result_t fault_result;
    bool raw_secured_fault;

    check_pcie_advance_error();

    TEEIO_DOE_DEBUG ((TEEIO_DEBUG_INFO, "[device_doe_send_message] Start ... \n"));
    TEEIO_DOE_DEBUG ((TEEIO_DEBUG_INFO, "[device_doe_send_message] RequestSize = 0x%x \n", request_size));

    if (request == NULL) {
        return LIBSPDM_STATUS_INVALID_PARAMETER;
    }

    if (request_size == 0) {
        return LIBSPDM_STATUS_INVALID_PARAMETER;
    }

    if (m_fault_deferred_send_size != 0) {
        request = m_fault_deferred_send;
        request_size = m_fault_deferred_send_size;
        m_fault_deferred_send_size = 0;
    } else {
        raw_secured_fault = request_size >= sizeof(pci_doe_data_object_header_t) &&
                            ((const uint8_t *)request)[2] ==
                                PCI_DOE_DATA_OBJECT_TYPE_SECURED_SPDM;
        fault_result = teeio_fault_apply(request, request_size,
                                         TEEIO_FAULT_MAX_MESSAGE_SIZE);
        if (fault_result.disposition == TEEIO_FAULT_DISPOSITION_ERROR) {
            teeio_fault_record_actual("injection_rejected");
            TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Fault injection rejected: %s\n",
                         teeio_fault_audit_record()));
            return LIBSPDM_STATUS_SEND_FAIL;
        }
        if (fault_result.rule_id >= 0) {
            TEEIO_DEBUG((TEEIO_DEBUG_WARN, "Fault injection: %s\n",
                         teeio_fault_audit_record()));
            if (raw_secured_fault &&
                teeio_fault_same_session_recovery_configured() &&
                !restore_fault_session()) {
                teeio_fault_record_actual("session_state_restore_failed");
                return LIBSPDM_STATUS_SEND_FAIL;
            }
        }
        if (fault_result.disposition == TEEIO_FAULT_DISPOSITION_DROP ||
            fault_result.disposition == TEEIO_FAULT_DISPOSITION_ABANDON ||
            fault_result.disposition == TEEIO_FAULT_DISPOSITION_HOLD) {
            m_fault_skip_receive = true;
            return LIBSPDM_STATUS_SUCCESS;
        }
        request = fault_result.message;
        request_size = fault_result.message_size;
        if (fault_result.secondary_message_size != 0) {
            memcpy(m_fault_deferred_send, fault_result.secondary_message,
                   fault_result.secondary_message_size);
            m_fault_deferred_send_size = fault_result.secondary_message_size;
        }
    }

    TEEIO_ASSERT((request_size & 3) == 0);
    data_object_count = (uint32_t)(request_size / sizeof(uint32_t));
    data_object_buffer = (uint32_t *)(uintptr_t)request; // cast discards ‘const’ qualifier from pointer target type

    TEEIO_DOE_DEBUG ((TEEIO_DEBUG_INFO, "[device_doe_send_message] Start ... \n"));

    if (timeout == 0) {
        timeout = PCI_EXPRESS_DOE_MAILBOX_TIMEOUT;
    }

        delay = timeout / 30 + 1;

    if (is_doe_error_asserted()) {
        TEEIO_DEBUG ((TEEIO_DEBUG_ERROR, "[device_doe_send_message] 'DOE Error' bit is set before sending message. Clear error bit and wait 1 second.\n"));
        /* Write 1b to the DOE Abort bit and wait 1 second. */
        trigger_doe_abort();
        libspdm_sleep(1000*1000);
    }

    do {
        /* Check the DOE Busy bit is Clear to ensure that the DOE instance is ready to receive a DOE request. */
        if (!is_doe_busy_asserted()) {
            /* Write the entire data object a DWORD at a time via the DOE Write Data Mailbox register. */
            TEEIO_DOE_DEBUG ((TEEIO_DEBUG_INFO, "[device_doe_send_message] 'DOE Busy' bit is cleared. Start writing Mailbox ...\n"));
            TEEIO_DOE_DEBUG ((TEEIO_DEBUG_VERBOSE, "Requester: \n"));
            for (index = 0; index < data_object_count; index++) { 
                device_pci_doe_write_mailbox_write_32 (data_object_buffer[index]);
                data_object_bytes = (const uint8_t *)(data_object_buffer + index);
                TEEIO_DOE_DEBUG((TEEIO_DEBUG_VERBOSE, "mailbox: 0x%08x\n", data_object_buffer[index]));
                TEEIO_DOE_DEBUG ((TEEIO_DEBUG_VERBOSE,"%02x %02x %02x %02x \n", data_object_bytes[0],
                                                            data_object_bytes[1],
                                                            data_object_bytes[2],
                                                            data_object_bytes[3]));
            }
            TEEIO_DOE_DEBUG ((TEEIO_DEBUG_VERBOSE,"\n"));

            /* Write 1b to the DOE Go bit. */
            TEEIO_DOE_DEBUG ((TEEIO_DEBUG_INFO, "[device_doe_send_message] Set 'DOE Go' bit, the instance start consuming the data object.\n"));
            trigger_doe_go();

            break;
        } else {
            /* Stall for 30 microseconds. */
            TEEIO_DOE_DEBUG ((TEEIO_DEBUG_INFO, "[device_doe_send_message] 'DOE Busy' bit is not cleared! Waiting ...\n"));
            if(is_doe_error_asserted()){
                TEEIO_DEBUG ((TEEIO_DEBUG_ERROR, "[device_doe_send_message] DOE error is found. Exiting!\n"));
                break;
            }
            libspdm_sleep (30 * 1000);
            delay--;
        }
    } while (delay != 0);

    if (delay == 0) {
        teeio_fault_record_actual("send_timeout");
        status = LIBSPDM_STATUS_SEND_FAIL;
    } else {
        /* check ERROR bit again */
        if (is_doe_error_asserted()) {
            teeio_fault_record_actual("doe_error");
            status = LIBSPDM_STATUS_SEND_FAIL;
            TEEIO_DEBUG ((TEEIO_DEBUG_ERROR, "[device_doe_send_message] 'DOE Error' bit is set. Send failed. Clear error bit and wait 1 second.\n"));
            /* Write 1b to the DOE Abort bit and wait 1 second. */
            trigger_doe_abort();
            libspdm_sleep(1000*1000);
        } else {
            append_pcap_packet_data(NULL, 0, (const void *)request, request_size);
            status = LIBSPDM_STATUS_SUCCESS;
        }
    }

    check_pcie_advance_error();

    return status;
}


libspdm_return_t device_doe_receive_message(
    void *spdm_context,
    size_t *response_size,
    void **response,
    uint64_t timeout)
{
    libspdm_return_t status;
    uint32_t data_object_count;
    uint32_t *data_object_buffer;
    uint32_t index;
    pci_doe_data_object_header_t *data_object_header;
    uint64_t delay;

    check_pcie_advance_error();

    if (response_size == NULL || response == NULL || *response == NULL) {
        return LIBSPDM_STATUS_INVALID_PARAMETER;
    }

    TEEIO_DOE_DEBUG ((TEEIO_DEBUG_INFO, "[device_doe_receive_message] Start ... \n"));
    TEEIO_DOE_DEBUG ((TEEIO_DEBUG_INFO, "[device_doe_receive_message] Response_size = 0x%x \n", *response_size));

    if (m_fault_skip_receive) {
        m_fault_skip_receive = false;
        teeio_fault_record_actual("controlled_drop");
        return LIBSPDM_STATUS_RECEIVE_FAIL;
    }
    if (timeout == 0) {
        timeout = PCI_EXPRESS_DOE_MAILBOX_TIMEOUT;
    }

    //delay = timeout / 30 + 1;
    delay = 0x1000000 / 30 + 1; //make sure debug device have enough time to write to mailbox

    data_object_buffer = (uint32_t *)*response;
    data_object_header = (pci_doe_data_object_header_t *)*response;
    if (*response_size < sizeof (pci_doe_data_object_header_t)) {
        *response_size = sizeof (pci_doe_data_object_header_t);
        return LIBSPDM_STATUS_BUFFER_TOO_SMALL;
    }

    /* check error bit */
    if (is_doe_error_asserted()) {
        TEEIO_DEBUG ((TEEIO_DEBUG_ERROR, "[device_doe_receive_message] 'DOE Error' bit is set before receiving. Clear error bit and wait 1 second.\n"));
        /* Write 1b to the DOE Abort bit and wait 1 second. */
        trigger_doe_abort();
        libspdm_sleep(1000*1000);
    }

    do {
        /* Poll the Data Object Ready bit. */
        if (is_doe_data_object_ready_asserted()) {
            TEEIO_DOE_DEBUG ((TEEIO_DEBUG_INFO, "[device_doe_receive_message] 'Data Object Ready' bit is set. Start reading Mailbox ...\n"));
            TEEIO_DOE_DEBUG ((TEEIO_DEBUG_INFO,"Responder: \n"));
            /* Get DataObjectHeader1. */
            data_object_buffer[0] = device_pci_doe_read_mailbox_read_32 ();
            /* Write to the DOE Read Data Mailbox to indicate a successful read. */
            device_pci_doe_read_mailbox_write_32 (data_object_buffer[0]);
            /* Get DataObjectHeader2. */
            data_object_buffer[1] = device_pci_doe_read_mailbox_read_32 ();
            /* Write to the DOE Read Data Mailbox to indicate a successful read. */
            device_pci_doe_read_mailbox_write_32 (data_object_buffer[1]);
            data_object_count = data_object_header->length;
            if (data_object_count == 0) {
                data_object_count = 0x40000;
            }
            TEEIO_DOE_DEBUG ((TEEIO_DEBUG_INFO, "[device_doe_receive_message] data_object_count = 0x%x\n", data_object_count));

            TEEIO_DOE_DEBUG ((TEEIO_DEBUG_INFO,"%02x %02x %02x %02x \n", *((uint8_t*)(data_object_buffer + 0) + 0),
                                                            *((uint8_t*)(data_object_buffer + 0) + 1),
                                                            *((uint8_t*)(data_object_buffer + 0) + 2),
                                                            *((uint8_t*)(data_object_buffer + 0) + 3)));
            TEEIO_DOE_DEBUG ((TEEIO_DEBUG_INFO,"%02x %02x %02x %02x \n", *((uint8_t*)(data_object_buffer + 1) + 0),
                                                            *((uint8_t*)(data_object_buffer + 1) + 1),
                                                            *((uint8_t*)(data_object_buffer + 1) + 2),
                                                            *((uint8_t*)(data_object_buffer + 1) + 3)));

            if (data_object_count * sizeof(uint32_t) > *response_size) {
                *response_size = data_object_count * sizeof(uint32_t);  
                return LIBSPDM_STATUS_BUFFER_TOO_SMALL;
            }
            *response_size = data_object_count * sizeof(uint32_t);

            for (index = sizeof (pci_doe_data_object_header_t) / sizeof(uint32_t); index < data_object_count; index++) {
                /* Read data from the DOE Read Data Mailbox and save it. */
                data_object_buffer[index] = device_pci_doe_read_mailbox_read_32 ();
                /* Write to the DOE Read Data Mailbox to indicate a successful read. */
                device_pci_doe_read_mailbox_write_32 (data_object_buffer[index]);
                TEEIO_DOE_DEBUG ((TEEIO_DEBUG_INFO,"%02x %02x %02x %02x \n", *((uint8_t*)(data_object_buffer + index) + 0),
                                                            *((uint8_t*)(data_object_buffer + index) + 1),
                                                            *((uint8_t*)(data_object_buffer + index) + 2),
                                                            *((uint8_t*)(data_object_buffer + index) + 3)));
            }
            TEEIO_DOE_DEBUG ((TEEIO_DEBUG_INFO,"\n"));

            break;
        } else if (m_fault_draining_duplicate) {
            status = LIBSPDM_STATUS_SUCCESS;
        } else {
            /* Stall for 30 microseconds.. */
            TEEIO_DOE_DEBUG ((TEEIO_DEBUG_INFO, "[device_doe_receive_message] 'Data Object Ready' bit is not set! Waiting ...\n"));
            if(is_doe_error_asserted()){
                TEEIO_DEBUG ((TEEIO_DEBUG_ERROR, "[device_doe_receive_message] 'DOE Error' bit is set. Quit the reading loop\n"));
                break;
            }
            libspdm_sleep (30 * 1000);
            delay--;
        }
    } while (delay != 0);

    if (delay == 0) {
        teeio_fault_record_actual("transport_receive_timeout");
        status = LIBSPDM_STATUS_RECEIVE_FAIL;
    } else {
        /* check ERROR bit again */
        if (is_doe_error_asserted()) {
            teeio_fault_record_actual("transport_receive_failed");
            status = LIBSPDM_STATUS_RECEIVE_FAIL;
            TEEIO_DEBUG ((TEEIO_DEBUG_ERROR, "[device_doe_receive_message] 'DOE Error' bit is set. Receive failed. Clear error bit and wait 1 second.\n"));
            /* Write 1b to the DOE Abort bit and wait 1 second. */
            trigger_doe_abort();
            libspdm_sleep(1000*1000);
        } else {
            append_pcap_packet_data(NULL, 0, (const void *)*response, *response_size);
            status = LIBSPDM_STATUS_SUCCESS;
        }
    }

    if (!LIBSPDM_STATUS_IS_ERROR(status) && !m_fault_draining_duplicate &&
        m_fault_deferred_send_size != 0) {
        void *duplicate_response = m_fault_duplicate_response;
        size_t duplicate_response_size = sizeof(m_fault_duplicate_response);
        libspdm_return_t duplicate_status;

        duplicate_status = device_doe_send_message(
            spdm_context, m_fault_deferred_send_size,
            m_fault_deferred_send, timeout);
        if (!LIBSPDM_STATUS_IS_ERROR(duplicate_status)) {
            m_fault_draining_duplicate = true;
            duplicate_status = device_doe_receive_message(
                spdm_context, &duplicate_response_size,
                &duplicate_response, timeout);
            m_fault_draining_duplicate = false;
        }
        if (LIBSPDM_STATUS_IS_ERROR(duplicate_status)) {
            teeio_fault_record_actual("duplicate_exchange_failed");
        } else if (duplicate_response_size >=
                   sizeof(pci_doe_data_object_header_t) +
                   sizeof(spdm_message_header_t)) {
            record_spdm_actual(
                m_fault_duplicate_response +
                sizeof(pci_doe_data_object_header_t),
                duplicate_response_size -
                sizeof(pci_doe_data_object_header_t));
        } else {
            teeio_fault_record_actual("duplicate_exchange_complete");
        }
    }

    check_pcie_advance_error();

    return status;
}

libspdm_return_t spdm_device_acquire_sender_buffer (
    void *context, void **msg_buf_ptr)
{
    TEEIO_ASSERT (!m_send_receive_buffer_acquired);
    *msg_buf_ptr = m_send_receive_buffer;
    libspdm_zero_mem (m_send_receive_buffer, sizeof(m_send_receive_buffer));
    m_send_receive_buffer_acquired = true;
    return LIBSPDM_STATUS_SUCCESS;
}

void spdm_device_release_sender_buffer (
    void *context, const void *msg_buf_ptr)
{
    TEEIO_ASSERT (m_send_receive_buffer_acquired);
    TEEIO_ASSERT (msg_buf_ptr == m_send_receive_buffer);
    m_send_receive_buffer_acquired = false;
    return;
}

libspdm_return_t spdm_device_acquire_receiver_buffer (
    void *context, void **msg_buf_ptr)
{
    TEEIO_ASSERT (!m_send_receive_buffer_acquired);
    *msg_buf_ptr = m_send_receive_buffer;
    libspdm_zero_mem (m_send_receive_buffer, sizeof(m_send_receive_buffer));
    m_send_receive_buffer_acquired = true;
    return LIBSPDM_STATUS_SUCCESS;
}

void spdm_device_release_receiver_buffer (
    void *context, const void *msg_buf_ptr)
{
    TEEIO_ASSERT (m_send_receive_buffer_acquired);
    TEEIO_ASSERT (msg_buf_ptr == m_send_receive_buffer);
    m_send_receive_buffer_acquired = false;
    return;
}

/**
 * Send and receive an DOE message
 *
 * @param request                       the PCI DOE request message, start from pci_doe_data_object_header_t.
 * @param request_size                  size in bytes of request.
 * @param response                      the PCI DOE response message, start from pci_doe_data_object_header_t.
 * @param response_size                 size in bytes of response.
 *
 * @retval LIBSPDM_STATUS_SUCCESS               The request is sent and response is received.
 * @return ERROR                        The response is not received correctly.
 **/
libspdm_return_t pci_doe_send_receive_data(const void *pci_doe_context,
                                        size_t request_size, const void *request,
                                        size_t *response_size, void *response)
{
    libspdm_return_t status;

    status = device_doe_send_message (NULL, request_size, request, 0);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        return status;
    }
    status = device_doe_receive_message (NULL, response_size, &response, 0);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        return status;
    }
    return LIBSPDM_STATUS_SUCCESS;
}

bool pcie_doe_init_request(uint8_t doe_discovery_version)
{
    pci_doe_data_object_protocol_t data_object_protocol[6];
    size_t data_object_protocol_size;
    libspdm_return_t status;
    uint32_t index;

    data_object_protocol_size = sizeof(data_object_protocol);
    status =
        pci_doe_discovery (m_pci_doe_context, data_object_protocol, &data_object_protocol_size, doe_discovery_version);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        return false;
    }

    for (index = 0; index < data_object_protocol_size/sizeof(pci_doe_data_object_protocol_t); index++) {
        TEEIO_DOE_DEBUG((TEEIO_DEBUG_INFO, "DOE(0x%x) VendorId-0x%04x, DataObjectType-0x%02x\n",
                        index, data_object_protocol[index].vendor_id,
                        data_object_protocol[index].data_object_type));
    }

    bool found = false;
    for(int i = 0; i < sizeof(m_pci_doe_data_object_type); i++) {
        found = false;
        for (index = 0; index < data_object_protocol_size/sizeof(pci_doe_data_object_protocol_t); index++) {
            if(data_object_protocol[index].data_object_type == m_pci_doe_data_object_type[i]) {
                found = true; break;
            }
        }
        if(!found) {
            TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "PCI DOE DataObjectType(0x%02x) is not found.\n", m_pci_doe_data_object_type[i]));
            break;
        }
    }

    return found;
}
