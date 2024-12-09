/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __CXL_IDE_TEST_INTERNAL_H__
#define __CXL_IDE_TEST_INTERNAL_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/param.h>
#include "ide_test.h"
#include "library/cxl_ide_km_requester_lib.h"

#pragma pack(1)
typedef struct {
    cxl_ide_km_header_t header;
    uint8_t reserved;
    uint8_t port_index;
    uint8_t dev_func_num;
    uint8_t bus_num;
    uint8_t segment;
    uint8_t max_port_index;
    uint8_t caps;
    uint32_t ide_reg_block[CXL_IDE_KM_IDE_CAP_REG_BLOCK_MAX_COUNT];
} cxl_ide_km_query_resp_teeio_t;
#pragma pack()

// setup cxl ide stream
bool cxl_setup_ide_stream(void *doe_context, void *spdm_context,
                          uint32_t *session_id, uint8_t *kcbar_addr,
                          uint8_t stream_id, uint8_t port_index,
                          ide_common_test_port_context_t *upper_port,
                          ide_common_test_port_context_t *lower_port,
                          bool skip_ksetgo, uint32_t config_bitmap,
                          bool set_link_enc_enable, bool program_iv);
// stop cxl ide stream
bool cxl_stop_ide_stream(void *doe_context, void *spdm_context,
                         uint32_t *session_id, uint8_t *kcbar_addr,
                         uint8_t stream_id,
                         uint8_t port_index,
                         ide_common_test_port_context_t *upper_port,
                         ide_common_test_port_context_t *lower_port);

void test_cxl_ide_query(const void *pci_doe_context,
                        void *spdm_context, const uint32_t *session_id,
                        uint8_t port_index, uint8_t dev_func_num, uint8_t bus_num, uint8_t segment,
                        int case_class, int case_id);

#endif