/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __PCIE_IDE_INTERNAL_H__
#define __PCIE_IDE_INTERNAL_H__

#include "ide_test.h"

uint32_t get_ide_reg_block_offset(int fd, TEST_IDE_TYPE ide_type, uint8_t ide_id, uint32_t ide_ecap_offset);

bool init_pci_doe(int fd);

void enable_ide_stream_in_kcbar(
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *const kcbar_ptr,
    const uint8_t rp_stream_index,
    bool enable
);

void dump_kcbar(
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *const kcbar_ptr,
    const uint8_t rp_stream_index
);

#endif
