/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include "teeio_validator.h"

void *m_spdm_context;
void *m_scratch_buffer;

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

void *spdm_client_init(void)
{
    void *spdm_context;
    libspdm_return_t status;
    libspdm_data_parameter_t parameter;
    uint8_t data8;
    uint16_t data16;
    uint32_t data32;
    size_t scratch_buffer_size;

    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "context_size - 0x%x\n", (uint32_t)libspdm_get_context_size()));

    m_spdm_context = (void *)malloc(libspdm_get_context_size());
    if (m_spdm_context == NULL) {
        return NULL;
    }
    spdm_context = m_spdm_context;
    libspdm_init_context(spdm_context);

    libspdm_register_device_io_func(spdm_context, device_doe_send_message,
                                    device_doe_receive_message);
    libspdm_register_transport_layer_func(
            spdm_context,
            LIBSPDM_MAX_SPDM_MSG_SIZE,
            LIBSPDM_TRANSPORT_HEADER_SIZE,
            LIBSPDM_TRANSPORT_TAIL_SIZE,
            libspdm_transport_pci_doe_encode_message,
            libspdm_transport_pci_doe_decode_message);
    libspdm_register_device_buffer_func(spdm_context,
                                        LIBSPDM_SENDER_BUFFER_SIZE,
                                        LIBSPDM_RECEIVER_BUFFER_SIZE,
                                        spdm_device_acquire_sender_buffer,
                                        spdm_device_release_sender_buffer,
                                        spdm_device_acquire_receiver_buffer,
                                        spdm_device_release_receiver_buffer);
    scratch_buffer_size = libspdm_get_sizeof_required_scratch_buffer(m_spdm_context);
    m_scratch_buffer = (void *)malloc(scratch_buffer_size);
    if (m_scratch_buffer == NULL) {
        free(m_spdm_context);
        m_spdm_context = NULL;
        return NULL;
    }
    libspdm_set_scratch_buffer (spdm_context, m_scratch_buffer, scratch_buffer_size);


    libspdm_zero_mem(&parameter, sizeof(parameter));
    parameter.location = LIBSPDM_DATA_LOCATION_LOCAL;

    data8 = 0;
    libspdm_set_data(spdm_context, LIBSPDM_DATA_CAPABILITY_CT_EXPONENT,
                     &parameter, &data8, sizeof(data8));
    /* clear HBEAT_CAP now, since we dont have timer for send SPDM HBEAT */
    data32 =
        (0 |
        SPDM_GET_CAPABILITIES_REQUEST_FLAGS_CERT_CAP |
        /* SPDM_GET_CAPABILITIES_REQUEST_FLAGS_CHAL_CAP | */
        SPDM_GET_CAPABILITIES_REQUEST_FLAGS_ENCRYPT_CAP |
        SPDM_GET_CAPABILITIES_REQUEST_FLAGS_MAC_CAP |
        /* SPDM_GET_CAPABILITIES_REQUEST_FLAGS_MUT_AUTH_CAP | */
        SPDM_GET_CAPABILITIES_REQUEST_FLAGS_KEY_EX_CAP |
        /* SPDM_GET_CAPABILITIES_REQUEST_FLAGS_PSK_CAP_REQUESTER | */
        SPDM_GET_CAPABILITIES_REQUEST_FLAGS_ENCAP_CAP |
        /* SPDM_GET_CAPABILITIES_REQUEST_FLAGS_HBEAT_CAP | */
        SPDM_GET_CAPABILITIES_REQUEST_FLAGS_KEY_UPD_CAP |
        /* SPDM_GET_CAPABILITIES_REQUEST_FLAGS_HANDSHAKE_IN_THE_CLEAR_CAP | */
        /* SPDM_GET_CAPABILITIES_REQUEST_FLAGS_PUB_KEY_ID_CAP | */
        0);
    libspdm_set_data(spdm_context, LIBSPDM_DATA_CAPABILITY_FLAGS, &parameter,
                     &data32, sizeof(data32));

    data8 = SPDM_MEASUREMENT_SPECIFICATION_DMTF;
    libspdm_set_data(spdm_context, LIBSPDM_DATA_MEASUREMENT_SPEC, &parameter,
                     &data8, sizeof(data8));
    data32 = SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;
    data32 |= SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;
    data32 |= SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_RSASSA_3072;
    libspdm_set_data(spdm_context, LIBSPDM_DATA_BASE_ASYM_ALGO, &parameter,
                     &data32, sizeof(data32));
    data32 = SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_384;
    data32 |= SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_256;
    libspdm_set_data(spdm_context, LIBSPDM_DATA_BASE_HASH_ALGO, &parameter,
                     &data32, sizeof(data32));
    data16 = SPDM_ALGORITHMS_DHE_NAMED_GROUP_SECP_384_R1;
    data16 |= SPDM_ALGORITHMS_DHE_NAMED_GROUP_SECP_256_R1;
    libspdm_set_data(spdm_context, LIBSPDM_DATA_DHE_NAME_GROUP, &parameter,
                     &data16, sizeof(data16));
    data16 = SPDM_ALGORITHMS_AEAD_CIPHER_SUITE_AES_256_GCM;
    libspdm_set_data(spdm_context, LIBSPDM_DATA_AEAD_CIPHER_SUITE, &parameter,
                     &data16, sizeof(data16));
    data16 = 0;
    libspdm_set_data(spdm_context, LIBSPDM_DATA_REQ_BASE_ASYM_ALG, &parameter,
                     &data16, sizeof(data16));
    data16 = SPDM_ALGORITHMS_KEY_SCHEDULE_HMAC_HASH;
    libspdm_set_data(spdm_context, LIBSPDM_DATA_KEY_SCHEDULE, &parameter, &data16,
                     sizeof(data16));
    data8 = SPDM_ALGORITHMS_OPAQUE_DATA_FORMAT_1;
    libspdm_set_data(spdm_context, LIBSPDM_DATA_OTHER_PARAMS_SUPPORT, &parameter,
                     &data8, sizeof(data8));

    status = libspdm_init_connection(spdm_context, false);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "libspdm_init_connection - 0x%x\n", (uint32_t)status));
        free(m_spdm_context);
        m_spdm_context = NULL;
        return NULL;
    }

    return m_spdm_context;
}

bool spdm_connect (void *spdm_context, uint32_t *session_id)
{
    libspdm_return_t status;
    uint32_t measurement_record_length;
    uint8_t measurement_record[LIBSPDM_MAX_MEASUREMENT_RECORD_SIZE];
    spdm_attester_cert_chain_struct_t cert_chain;
    uint8_t slot_id;
    // uint32_t session_id;
    uint32_t data32;
    size_t data_size;
    libspdm_data_parameter_t parameter;
    char cert_chain_name[] = "device_cert_chain_0.bin";
    char measurement_name[] = "device_measurement.bin";

    /* get cert_chain 0 */
    slot_id = 0;
    cert_chain.cert_chain_size = sizeof(cert_chain.cert_chain);
    libspdm_zero_mem (cert_chain.cert_chain, sizeof(cert_chain.cert_chain));
    status = libspdm_get_certificate (spdm_context, NULL, slot_id,
                                      &cert_chain.cert_chain_size,
                                      cert_chain.cert_chain);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "libspdm_get_certificate (slot=%d) - %x\n", slot_id, (uint32_t)status));
        return false;
    }
    cert_chain_name[18] = slot_id + '0';
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "write file - %s\n", cert_chain_name));
    libspdm_write_output_file (cert_chain_name,
                               cert_chain.cert_chain,
                               cert_chain.cert_chain_size);

    /* setup session based on slot 0 */
    status = libspdm_start_session(
                spdm_context, false,
                NULL, 0,
                SPDM_CHALLENGE_REQUEST_NO_MEASUREMENT_SUMMARY_HASH,
                0,
                SPDM_KEY_EXCHANGE_REQUEST_SESSION_POLICY_TERMINATION_POLICY_RUNTIME_UPDATE,
                session_id,
                NULL, NULL);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "libspdm_start_session - %x\n", (uint32_t)status));
        return false;
    }

    /* get measurement */
    libspdm_zero_mem(&parameter, sizeof(parameter));
    parameter.location = LIBSPDM_DATA_LOCATION_CONNECTION;
    data_size = sizeof(data32);
    data32 = 0;
    status = libspdm_get_data(spdm_context, LIBSPDM_DATA_CAPABILITY_FLAGS, &parameter, &data32, &data_size);
    TEEIO_ASSERT(!LIBSPDM_STATUS_IS_ERROR(status));
    if(data32 & SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_MEAS_CAP) {
        measurement_record_length = sizeof(measurement_record);
        status = spdm_send_receive_get_measurement (spdm_context, session_id, 0,
                                                    measurement_record,
                                                    &measurement_record_length);
        if (LIBSPDM_STATUS_IS_ERROR(status)) {
            TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "spdm_send_receive_get_measurement - %x\n", (uint32_t)status));
            return false;
        }

        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "write file - %s\n", measurement_name));
        libspdm_write_output_file (measurement_name,
                                measurement_record, measurement_record_length);
    } else {
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "spdm measurement is not supported.\n"));
    }

    /* get cert_chain 1 ~ 7 */
    for (slot_id = 1; slot_id < SPDM_MAX_SLOT_COUNT; slot_id++) {
        cert_chain.cert_chain_size = sizeof(cert_chain.cert_chain);
        libspdm_zero_mem (cert_chain.cert_chain, sizeof(cert_chain.cert_chain));
        status = libspdm_get_certificate(
                    spdm_context, session_id, slot_id,
                    &cert_chain.cert_chain_size,
                    cert_chain.cert_chain);
        if (LIBSPDM_STATUS_IS_ERROR(status)) {
            TEEIO_DEBUG((TEEIO_DEBUG_INFO, "libspdm_get_certificate (slot=%d) - %x\n", slot_id, (uint32_t)status));
            cert_chain.cert_chain_size = 0;
        }
        cert_chain_name[18] = slot_id + '0';
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "write file - %s\n", cert_chain_name));
        libspdm_write_output_file (cert_chain_name,
                                   cert_chain.cert_chain,
                                   cert_chain.cert_chain_size);
    }

    return true;
}

bool spdm_stop(void *spdm_context, uint32_t session_id)
{
    /* stop session */
    libspdm_return_t status = libspdm_stop_session(spdm_context, session_id, 0);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "libspdm_stop_session - %x\n", (uint32_t)status));
        return false;
    }

    return true;
}

