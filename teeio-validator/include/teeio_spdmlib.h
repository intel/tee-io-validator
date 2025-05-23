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
 * setup spdm connection
*/
bool spdm_connect (void *spdm_context, uint32_t *session_id);

/**
 * stop spdm connection
*/
bool spdm_stop(void *spdm_context, uint32_t session_id);

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
#define LIBSPDM_MAX_SPDM_MSG_SIZE 0x1200
#endif

#endif
