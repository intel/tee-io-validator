/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __CXL_IDE_LIB_H__
#define __CXL_IDE_LIB_H__

#include "ide_test.h"

/**
 * Initialize rootcomplex port
 * root_port and upper_port is the same port
 */
bool cxl_init_root_port(cxl_ide_test_group_context_t *group_context);

/*
 * Open rootcomplex port
 */
bool cxl_open_root_port(ide_common_test_port_context_t *port_context);

/*
 * Close rootcomplex port
 */
bool cxl_close_root_port(cxl_ide_test_group_context_t *group_context);

/*
 * Initialize device port
 */
bool cxl_init_dev_port(cxl_ide_test_group_context_t *group_context);

/*
 * Open device port
 */
bool cxl_open_dev_port(ide_common_test_port_context_t *port_context);

/*
 * Close device port
 */
bool cxl_close_dev_port(ide_common_test_port_context_t *port_context, IDE_TEST_TOPOLOGY_TYPE top_type);

/*
 * Scan devices
 */
bool cxl_scan_devices(void *test_context);

/**
 * enable/disable cache_enable bit in CXL_DEV_CONTROL
 */
void cxl_cfg_cache_enable(int fd, uint32_t ecap_offset, bool enable);

/**
 * enable/disable mem_enable bit in CXL_DEV_CONTROL
 */
void cxl_cfg_mem_enable(int fd, uint32_t ecap_offset, bool enable);

void cxl_cfg_rp_link_enc_key_iv(
    INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr,
    CXL_IDE_STREAM_DIRECTION direction, // RX TX
    const uint8_t key_slot,             // key 0, 1, 2, 3
    uint8_t* key, uint32_t key_size,    // key
    uint8_t* iv, uint32_t iv_size       // iv vals
    );

void cxl_cfg_rp_txrx_key_valid(
    INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr,
    CXL_IDE_STREAM_DIRECTION direction,
    bool valid
    );

void cxl_cfg_rp_txrx_transto_insecure_state(
    INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr,
    CXL_IDE_STREAM_DIRECTION direction,
    bool insecure_state
    );

void cxl_cfg_rp_start_trigger(
    INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr,
    bool start
    );

void cxl_cfg_rp_linkenc_enable(
    INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr,
    bool enable
    );

void cxl_cfg_rp_mode(
    INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr,
    INTEL_CXL_IDE_MODE mode
    );

void cxl_dump_kcbar(INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr);

void cxl_dump_caps_in_ecap(CXL_PRIV_DATA_ECAP* ecap);

void cxl_dump_ide_capability(CXL_CAPABILITY_XXX_HEADER* cap_header, int cap_headers_cnt, uint8_t* mapped_memcache_reg_block);
void cxl_dump_ide_status(CXL_CAPABILITY_XXX_HEADER* cap_header, int cap_headers_cnt, uint8_t* mapped_memcache_reg_block);

bool cxl_ide_set_key_refresh_control_reg(ide_common_test_port_context_t* host_port, ide_common_test_port_context_t* dev_port);
bool cxl_ide_set_truncation_transmit_control_reg(ide_common_test_port_context_t* host_port, ide_common_test_port_context_t* dev_port);

#endif
