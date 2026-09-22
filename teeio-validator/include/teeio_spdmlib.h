/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __TEEIO_SPDMLIB_H__
#define __TEEIO_SPDMLIB_H__

// spdmlib header file

/**
 * initialize pcie doe
*/
bool pcie_doe_init_request(uint8_t doe_discovery_version);

/**
 * trigger doe abort
*/
void trigger_doe_abort();

/**
 * check if doe error is asserted
*/
bool is_doe_error_asserted();

/**
 * initialize spdm client
*/
void *spdm_client_init(void);

/**
 * configure the requested SPDM version
 */
void teeio_spdm_set_version(uint8_t spdm_version);

/**
 * apply the configured SPDM version to a client context
 */
bool teeio_spdm_apply_version_override(void *spdm_context);

/**
 * log the requested and negotiated SPDM versions
 */
void teeio_spdm_log_negotiated_version(void *spdm_context);

/**
 * setup spdm connection
*/
bool spdm_connect (void *spdm_context, uint32_t *session_id);

/**
 * stop spdm connection
*/
bool spdm_stop(void *spdm_context, uint32_t session_id);

/**
 * Start a new certificate-based SPDM session using slot 0 on an existing
 * connection with the responder certificate already available in spdm_context.
 * Unlike spdm_connect(), this only starts the session; it does not retrieve
 * certificates or measurements, or write them to files. It can be used to
 * open an additional session without ending existing sessions.
*/
bool spdm_start_session(void *spdm_context, uint32_t *session_id);

/**
 * Start a new PSK-based SPDM session on an existing connection using the
 * supplied PSK hint and a preconfigured PSK.
 * Unlike spdm_connect(), this uses PSK authentication rather than a certificate
 * and does not retrieve certificates or measurements, or write them to files.
 * It can be used to open an additional session without ending existing sessions.
*/
bool spdm_start_psk_session(void *spdm_context, const void *psk_hint,
                            uint16_t psk_hint_size, uint32_t *session_id);

/**
 * End only the specified SPDM session by delegating to spdm_stop().
 * The existing connection and context remain available for other sessions.
 * This does not perform the session setup or certificate and measurement
 * retrieval performed by spdm_connect().
*/
bool spdm_end_session(void *spdm_context, uint32_t session_id);

libspdm_return_t device_doe_receive_message(
    void *spdm_context,
    size_t *response_size,
    void **response,
    uint64_t timeout);

libspdm_return_t device_doe_send_message(
    void *spdm_context,
    size_t request_size,
    const void *request,
    uint64_t timeout);

libspdm_return_t spdm_device_acquire_sender_buffer (
    void *context, void **msg_buf_ptr);

void spdm_device_release_sender_buffer (
    void *context, const void *msg_buf_ptr);

libspdm_return_t spdm_device_acquire_receiver_buffer (
    void *context, void **msg_buf_ptr);

void spdm_device_release_receiver_buffer (
    void *context, const void *msg_buf_ptr);

#define LIBSPDM_TRANSPORT_HEADER_SIZE 64
#define LIBSPDM_TRANSPORT_TAIL_SIZE 64

/* define common LIBSPDM_TRANSPORT_ADDITIONAL_SIZE. It should be the biggest one. */
#define LIBSPDM_TRANSPORT_ADDITIONAL_SIZE \
    (LIBSPDM_TRANSPORT_HEADER_SIZE + LIBSPDM_TRANSPORT_TAIL_SIZE)

#if LIBSPDM_TRANSPORT_ADDITIONAL_SIZE < LIBSPDM_NONE_TRANSPORT_ADDITIONAL_SIZE
#error LIBSPDM_TRANSPORT_ADDITIONAL_SIZE is smaller than the required size in NONE
#endif
#if LIBSPDM_TRANSPORT_ADDITIONAL_SIZE < LIBSPDM_TCP_TRANSPORT_ADDITIONAL_SIZE
#error LIBSPDM_TRANSPORT_ADDITIONAL_SIZE is smaller than the required size in TCP
#endif
#if LIBSPDM_TRANSPORT_ADDITIONAL_SIZE < LIBSPDM_PCI_DOE_TRANSPORT_ADDITIONAL_SIZE
#error LIBSPDM_TRANSPORT_ADDITIONAL_SIZE is smaller than the required size in PCI_DOE
#endif
#if LIBSPDM_TRANSPORT_ADDITIONAL_SIZE < LIBSPDM_MCTP_TRANSPORT_ADDITIONAL_SIZE
#error LIBSPDM_TRANSPORT_ADDITIONAL_SIZE is smaller than the required size in MCTP
#endif

#ifndef LIBSPDM_SENDER_BUFFER_SIZE
#define LIBSPDM_SENDER_BUFFER_SIZE (0x1100 + \
                                    LIBSPDM_TRANSPORT_ADDITIONAL_SIZE)
#endif
#ifndef LIBSPDM_RECEIVER_BUFFER_SIZE
#define LIBSPDM_RECEIVER_BUFFER_SIZE (0x1200 + \
                                      LIBSPDM_TRANSPORT_ADDITIONAL_SIZE)
#endif

/* Maximum size of a large SPDM message.
 * If chunk is unsupported, it must be same as DATA_TRANSFER_SIZE.
 * If chunk is supported, it must be larger than DATA_TRANSFER_SIZE.
 * It matches MaxSPDMmsgSize in SPDM specification. */
#ifndef LIBSPDM_MAX_SPDM_MSG_SIZE
#define LIBSPDM_MAX_SPDM_MSG_SIZE 0x1900
#endif

#endif
