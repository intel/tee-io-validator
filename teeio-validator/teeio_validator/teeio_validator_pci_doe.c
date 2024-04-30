/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include "teeio_validator.h"

void *m_pci_doe_context;

// The must supported pci_doe_data_object_type for TEEIO-Validator
uint8_t m_pci_doe_data_object_type[] = {
    PCI_DOE_DATA_OBJECT_TYPE_DOE_DISCOVERY,
    PCI_DOE_DATA_OBJECT_TYPE_SPDM,
    PCI_DOE_DATA_OBJECT_TYPE_SECURED_SPDM
};

bool pci_doe_init_request()
{
    pci_doe_data_object_protocol_t data_object_protocol[6];
    size_t data_object_protocol_size;
    libspdm_return_t status;
    uint32_t index;

    data_object_protocol_size = sizeof(data_object_protocol);
    status =
        pci_doe_discovery (m_pci_doe_context, data_object_protocol, &data_object_protocol_size);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        return false;
    }

    for (index = 0; index < data_object_protocol_size/sizeof(pci_doe_data_object_protocol_t); index++) {
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "DOE(0x%x) VendorId-0x%04x, DataObjectType-0x%02x\n",
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
