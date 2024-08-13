/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __CXL_IDE_INTERNAL_H__
#define __CXL_IDE_INTERNAL_H__

#include "ide_test.h"

uint8_t* cxl_map_bar_addr(int cfg_space_fd, uint32_t bar, uint64_t offset_in_bar, int* mapped_fd);
void cxl_unmap_memcache_reg_block(int mapped_fd, uint8_t* mapped_addr);
bool cxl_init_memcache_reg_block(int cfg_space_fd, CXL_PRIV_DATA_MEMCACHE_REG_DATA* memcache_regs, IDE_TEST_CXL_PCIE_DVSEC* dvsec, int count);
bool cxl_populate_dev_caps_in_ecap(int fd, CXL_PRIV_DATA_ECAP* ecap);
#endif
