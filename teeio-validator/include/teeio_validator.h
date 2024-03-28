/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __TEEIO_VALIDATOR_H__
#define __TEEIO_VALIDATOR_H__

#include "hal/base.h"
#include "hal/library/memlib.h"
#include "hal/library/debuglib.h"
#include "library/spdm_requester_lib.h"
#include "library/spdm_transport_pcidoe_lib.h"
#include "library/pci_doe_requester_lib.h"
#include "library/pci_ide_km_requester_lib.h"
#include "library/pci_tdisp_requester_lib.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "assert.h"
#include "time.h"
#include "stdarg.h"

#include "utils.h"
#include "teeio_debug.h"

#define TEEIO_VALIDATOR_NAME "teeio_validator"
#define TEEIO_VALIDATOR_VERSION "0.1.0"

typedef struct {
    size_t cert_chain_size;
    uint8_t cert_chain[LIBSPDM_MAX_CERT_CHAIN_SIZE];
} spdm_attester_cert_chain_struct_t;

libspdm_return_t spdm_send_receive_get_measurement(void *spdm_context,
                                                   const uint32_t *session_id,
                                                   uint8_t slot_id,
                                                   uint8_t *measurement_record,
                                                   uint32_t *measurement_record_length);

// void spdm_device_evidence_collection (void *spdm_context);

libspdm_return_t device_doe_send_message(
    void *spdm_context,
    size_t request_size,
    const void *request,
    uint64_t timeout);

libspdm_return_t device_doe_receive_message(
    void *spdm_context,
    size_t *response_size,
    void **response,
    uint64_t timeout);

libspdm_return_t spdm_device_acquire_sender_buffer (
    void *context, size_t *max_msg_size, void **msg_buf_ptr);

void spdm_device_release_sender_buffer (
    void *context, const void *msg_buf_ptr);

libspdm_return_t spdm_device_acquire_receiver_buffer (
    void *context, size_t *max_msg_size, void **msg_buf_ptr);

void spdm_device_release_receiver_buffer (
    void *context, const void *msg_buf_ptr);

libspdm_return_t pci_doe_process_session_test(void *spdm_context, uint32_t session_id);

bool libspdm_write_output_file(const char *file_name, const void *file_data,
                               size_t file_size);

#define LOGFILE "./spdm_log.txt"

#endif
