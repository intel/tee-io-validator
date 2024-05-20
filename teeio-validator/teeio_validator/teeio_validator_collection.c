/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include "teeio_validator.h"

void
spdm_device_evidence_collection (void *spdm_context)
{
    libspdm_return_t status;
    uint32_t measurement_record_length;
    uint8_t measurement_record[LIBSPDM_MAX_MEASUREMENT_RECORD_SIZE];
    spdm_attester_cert_chain_struct_t cert_chain;
    uint8_t slot_id;
    uint32_t session_id;
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
        return ;
    }
    cert_chain_name[18] = slot_id + '0';
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "write file - %s\n", cert_chain_name));
    libspdm_write_output_file (cert_chain_name,
                               cert_chain.cert_chain,
                               cert_chain.cert_chain_size);

    /* setup session based on slot 0 */
    status = libspdm_start_session(
                spdm_context, false,
                SPDM_CHALLENGE_REQUEST_NO_MEASUREMENT_SUMMARY_HASH,
                0,
                SPDM_KEY_EXCHANGE_REQUEST_SESSION_POLICY_TERMINATION_POLICY_RUNTIME_UPDATE,
                &session_id,
                NULL, NULL);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "libspdm_start_session - %x\n", (uint32_t)status));
        return ;
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
        status = spdm_send_receive_get_measurement (spdm_context, &session_id, 0,
                                                    measurement_record,
                                                    &measurement_record_length);
        if (LIBSPDM_STATUS_IS_ERROR(status)) {
            TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "spdm_send_receive_get_measurement - %x\n", (uint32_t)status));
            return ;
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
                    spdm_context, &session_id, slot_id,
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

    /* test IDE/TDISP in session */
    pci_doe_process_session_test(spdm_context, session_id);

    /* stop session */
    status = libspdm_stop_session(spdm_context, session_id, 0);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "libspdm_stop_session - %x\n", (uint32_t)status));
        return ;
    }

    return ;
}
