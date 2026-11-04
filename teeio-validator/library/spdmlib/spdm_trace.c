/**
 *  Copyright Notice:
 *  Copyright 2025 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include "teeio_validator.h"
#include "teeio_spdmlib.h"

/*
 * Tracks the unsecured SPDM traffic that crosses the DOE layer so a failed test
 * case setup can be explained afterwards. The state describes only the most
 * recent exchange; it is reset by the test runner before each case.
 */

static teeio_spdm_trace_t m_trace;
static uint8_t m_last_request_code;

/* A DOE data object starts with an 8 byte header, followed by the SPDM message. */
static const uint8_t *get_spdm_message(const void *doe_message, size_t size, size_t *spdm_size)
{
    const pci_doe_data_object_header_t *header = doe_message;

    if (doe_message == NULL || size < sizeof(pci_doe_data_object_header_t) + sizeof(spdm_message_header_t)) {
        return NULL;
    }
    if (header->data_object_type != PCI_DOE_DATA_OBJECT_TYPE_SPDM) {
        return NULL;
    }

    *spdm_size = size - sizeof(pci_doe_data_object_header_t);
    return (const uint8_t *)doe_message + sizeof(pci_doe_data_object_header_t);
}

void teeio_spdm_trace_reset(void)
{
    libspdm_zero_mem(&m_trace, sizeof(m_trace));
    m_last_request_code = 0;
}

void teeio_spdm_trace_get(teeio_spdm_trace_t *trace)
{
    TEEIO_ASSERT(trace != NULL);
    *trace = m_trace;
}

void teeio_spdm_trace_on_send(const void *doe_message, size_t size)
{
    size_t spdm_size;
    const uint8_t *spdm = get_spdm_message(doe_message, size, &spdm_size);

    if (spdm != NULL) {
        m_last_request_code = ((const spdm_message_header_t *)spdm)->request_response_code;
    }
}

void teeio_spdm_trace_on_receive(const void *doe_message, size_t size)
{
    size_t spdm_size;
    const uint8_t *spdm = get_spdm_message(doe_message, size, &spdm_size);
    const spdm_message_header_t *header;

    if (spdm == NULL) {
        return;
    }
    header = (const spdm_message_header_t *)spdm;

    /* Only the latest response matters: an ERROR that was followed by a normal
     * response (for example a retried request) did not end the setup. */
    m_trace.last_response_is_error = false;

    if (header->request_response_code == SPDM_ERROR) {
        m_trace.last_response_is_error = true;
        m_trace.error_request_code = m_last_request_code;
        m_trace.error_code = header->param1;
        m_trace.error_data = header->param2;
        m_trace.error_spdm_version = header->spdm_version;
    } else if (header->request_response_code == SPDM_VERSION) {
        const spdm_version_response_t *version_response = (const spdm_version_response_t *)spdm;
        const uint8_t *entries = spdm + sizeof(spdm_version_response_t);
        size_t available;
        uint8_t count;
        uint8_t index;

        if (spdm_size < sizeof(spdm_version_response_t)) {
            return;
        }
        available = (spdm_size - sizeof(spdm_version_response_t)) / sizeof(spdm_version_number_t);
        count = version_response->version_number_entry_count;
        if (count > available) {
            count = (uint8_t)available;
        }
        if (count > TEEIO_SPDM_TRACE_MAX_VERSIONS) {
            count = TEEIO_SPDM_TRACE_MAX_VERSIONS;
        }

        m_trace.has_versions = true;
        m_trace.version_count = count;
        for (index = 0; index < count; index++) {
            spdm_version_number_t entry;
            libspdm_copy_mem(&entry, sizeof(entry), entries + index * sizeof(entry), sizeof(entry));
            m_trace.versions[index] = (uint8_t)(entry >> SPDM_VERSION_NUMBER_SHIFT_BIT);
        }
    }
}

const char *teeio_spdm_request_code_name(uint8_t request_code)
{
    switch (request_code) {
    case SPDM_GET_VERSION: return "GET_VERSION";
    case SPDM_GET_CAPABILITIES: return "GET_CAPABILITIES";
    case SPDM_NEGOTIATE_ALGORITHMS: return "NEGOTIATE_ALGORITHMS";
    case SPDM_GET_DIGESTS: return "GET_DIGESTS";
    case SPDM_GET_CERTIFICATE: return "GET_CERTIFICATE";
    case SPDM_CHALLENGE: return "CHALLENGE";
    case SPDM_GET_MEASUREMENTS: return "GET_MEASUREMENTS";
    case SPDM_KEY_EXCHANGE: return "KEY_EXCHANGE";
    case SPDM_FINISH: return "FINISH";
    case SPDM_HEARTBEAT: return "HEARTBEAT";
    case SPDM_KEY_UPDATE: return "KEY_UPDATE";
    case SPDM_END_SESSION: return "END_SESSION";
    default: return "an SPDM request";
    }
}

const char *teeio_spdm_error_code_name(uint8_t error_code)
{
    switch (error_code) {
    case SPDM_ERROR_CODE_INVALID_REQUEST: return "InvalidRequest";
    case SPDM_ERROR_CODE_BUSY: return "Busy";
    case SPDM_ERROR_CODE_UNEXPECTED_REQUEST: return "UnexpectedRequest";
    case SPDM_ERROR_CODE_UNSPECIFIED: return "Unspecified";
    case SPDM_ERROR_CODE_DECRYPT_ERROR: return "DecryptError";
    case SPDM_ERROR_CODE_UNSUPPORTED_REQUEST: return "UnsupportedRequest";
    case SPDM_ERROR_CODE_VERSION_MISMATCH: return "VersionMismatch";
    case SPDM_ERROR_CODE_RESPONSE_NOT_READY: return "ResponseNotReady";
    case SPDM_ERROR_CODE_REQUEST_RESYNCH: return "RequestResynch";
    default: return "other error";
    }
}
