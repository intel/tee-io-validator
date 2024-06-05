/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __PCIE_IDE_TEST_INTERNAL_H__
#define __PCIE_IDE_TEST_INTERNAL_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/param.h>

#include "library/pci_ide_km_requester_lib.h"

#pragma pack(1)
typedef struct {
    pci_ide_km_header_t header;
    uint8_t reserved[2];
    uint8_t stream_id;
    uint8_t reserved2;
    uint8_t key_sub_stream;
    uint8_t port_index;
    pci_ide_km_aes_256_gcm_key_buffer_t key_buffer;
} test_pci_ide_km_key_prog_t;
#pragma pack()

#endif