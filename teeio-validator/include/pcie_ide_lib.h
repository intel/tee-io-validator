/**
 *  Copyright Notice:
 *  Copyright 2024-2025 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __PCIE_IDE_LIB_H__
#define __PCIE_IDE_LIB_H__

#include "ide_test.h"

// pcie_ide_lib header file

//
// PCIE APIs
//

/**
 * open PCIE's configuration space
*/
int open_configuration_space(char *bdf);

/**
 * get offset of cap in cap list
*/
uint32_t get_cap_offset(int fd, uint32_t cap_id);

/**
 * get offset of ext in ecap
*/
uint32_t get_extended_cap_offset(int fd, uint32_t ext_id);

/**
 * There may be multi DOE Extened Caps in ecap. This function walks thru the
 * extended caps and find out all the DOE Extended caps offset.
*/
bool get_doe_extended_cap_offset(int fd, uint32_t* doe_offsets, int* size);

/**
 * scan the devices from upper_port@bus
 */
bool scan_devices_at_bus(
    IDE_PORT* rp,
    IDE_PORT* ep,
    ide_common_test_switch_internal_conn_context_t* conn,
    uint8_t bus);

/**
 * Initialize rootcomplex port
 * root_port and upper_port is the same port
 */
bool init_root_port(pcie_ide_test_group_context_t *group_context);

/*
 * Open rootcomplex port
 */
bool open_root_port(ide_common_test_port_context_t *port_context);

/*
 * Close rootcomplex port
 */
bool close_root_port(pcie_ide_test_group_context_t *group_context);

/*
 * Initialize device port
 */
bool init_dev_port(pcie_ide_test_group_context_t *group_context);

/*
 * Open device port
 */
bool open_dev_port(ide_common_test_port_context_t *port_context);

/*
 * Close device port
 */
bool close_dev_port(ide_common_test_port_context_t *port_context, IDE_TEST_TOPOLOGY_TYPE top_type);

/*
 * set pcrc_en in ecap
 */
bool set_pcrc_in_ecap(
    int fd,
    TEST_IDE_TYPE ide_type,
    uint8_t ide_id,
    uint32_t ide_ecap_offset,
    bool enable
);

/**
 * Set Selective IDE for Configuration Req (bit9) in ide_stream ctrl
*/
bool set_sel_ide_for_cfg_req_in_ecap(
    int fd,
    TEST_IDE_TYPE ide_type,
    uint8_t ide_id,
    uint32_t ide_ecap_offset,
    bool enable
);

/**
 * read ide_stream_ctrl register in ecap
*/
uint32_t read_ide_stream_ctrl_in_ecap(
    int fd,
    TEST_IDE_TYPE ide_type,
    uint8_t ide_id,
    uint32_t ide_ecap_offset
);

/*
 * setup the ide ecap regs
 * IDE Extended Capability is defined in [PCI-SIG IDE] Sec 7.9.99
 */
bool setup_ide_ecap_regs (
    int fd, TEST_IDE_TYPE ide_type,
    uint8_t ide_id, uint32_t ide_ecap_offset,
    PCIE_SEL_IDE_STREAM_CTRL stream_ctrl_reg,
    PCIE_SEL_IDE_RID_ASSOC_1 rid_assoc_1,
    PCIE_SEL_IDE_RID_ASSOC_2 rid_assoc_2,
    PCIE_SEL_IDE_ADDR_ASSOC_1 addr_assoc_1,
    PCIE_SEL_IDE_ADDR_ASSOC_2 addr_assoc_2,
    PCIE_SEL_IDE_ADDR_ASSOC_3 addr_assoc_3);

/*
 * Reset ide stream related regisgers.
 * These registers include both PCIE ecap registers and Intel KCBAR registers 
*/
bool reset_ide_registers(
    ide_common_test_port_context_t *port_context,
    IDE_TEST_TOPOLOGY_TYPE top_type,
    uint8_t stream_id,
    uint8_t rp_stream_index,
    bool reset_kcbar);

/*
 * set prime_key_set_0/1 in stream_txrx_ctrl
*/
void prime_rp_ide_key_set(
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *const kcbar_ptr,
    const uint8_t rp_stream_index,
    const uint8_t direction,
    const uint8_t key_set_select);

/**
 * set stream_txrx_control.stream_tx_control.key_set_select
*/
void set_rp_ide_key_set_select(
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *const kcbar_ptr,
    const uint8_t rp_stream_index,
    const uint8_t key_set_select);

/**
 * set enable bit of PCIE_LNK_IDE_STREAM_CTRL / PCIE_SEL_IDE_STREAM_CTRL
*/
bool enable_ide_stream_in_ecap(
    int cfg_space_fd,
    uint32_t ecap_offset,
    TEST_IDE_TYPE ide_type,
    uint8_t ide_id,
    bool enable);

//
// Intel RootComplex KCBAR APIs
//

/**
 * get stream_cfg_blocks from Intel KCBAR
*/
INTEL_KEYP_STREAM_CONFIG_REG_BLOCK *
get_stream_cfg_reg_block(
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *const kcbar_ptr,
    const uint16_t rp_stream_index);

/**
 * parse KEYP table
*/
bool parse_keyp_table(ide_common_test_port_context_t *port_context, INTEL_KEYP_PROTOCOL_TYPE keyp_protocol);

/**
 * map KCBAR into memory
*/
uint8_t* map_kcbar_addr(uint64_t addr, int* mapped_fd);

/**
 * unmap KCBAR mapped memory
*/
bool unmap_kcbar_addr(int kcbar_mem_fd, uint8_t* mapped_kcbar_addr);

/**
 * Initialize key configuration registers block
*/
bool initialize_kcbar_registers(
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *kcbar,
    const uint8_t stream_id,
    const uint8_t rp_stream_index);

/**
 * This function is to find free key/iv slots for PCIE-IDE stream.
 *
 * There are 3 substreams (PR/NPR/CPL) in a PCIE-IDE stream. We assume
 * the key/iv slots allocated for these substreams are continuous. For
 * example, 0|1|2 or 3|4|5.
 */
bool pcie_ide_alloc_slot_ids(
    ide_common_test_port_context_t* port_context,
    uint8_t rp_stream_index,
    ide_key_set_t* k_set);

/**
 * enable rootport ide stream.
 * It will set registers in both PCIE ecap and Intel KCBAR.
*/
void enable_rootport_ide_stream(
    int cfg_space_fd,
    uint32_t ecap_offset,
    TEST_IDE_TYPE ide_type,
    uint8_t ide_id,
    uint8_t *kcbar_addr,
    uint8_t rp_stream_index,
    bool enable);

/**
 * read ide_stream status in rootport ecap
*/
uint32_t read_stream_status_in_rp_ecap(
    int cfg_space_fd,
    uint32_t ecap_offset,
    TEST_IDE_TYPE ide_type,
    uint8_t ide_id);

/**
 * check stream_ctrl.enabled (refer to PCIE_SEL_IDE_STREAM_CTRL & PCIE_LNK_IDE_STREAM_CTRL)
*/
bool is_ide_enabled(
    int cfg_space_fd,
    TEST_IDE_TYPE ide_type,
    uint8_t ide_id,
    uint32_t ecap_offset);

/**
 * dump rootport's registers, including both ecap and kcbar
*/
void dump_rootport_registers(
    uint8_t *kcbar_addr,
    uint8_t rp_stream_index,
    int cfg_space_fd,
    uint8_t ide_id,
    uint32_t ecap_offset,
    TEST_IDE_TYPE ide_type);

/**
 * dump dev's registers.
*/
void dump_dev_registers(
    int cfg_space_fd,
    uint8_t ide_id,
    uint32_t ecap_offset,
    TEST_IDE_TYPE ide_type);

void cfg_rootport_ide_keys(
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *kcbar_ptr,
    const uint8_t rp_stream_index,              // N
    const uint8_t direction,                    // RX TX
    const uint8_t key_set_select,               // KS0 KS1
    const uint8_t sub_stream,                   // PR NPR CPL
    const uint8_t slot_id,                      // n
    INTEL_KEYP_KEY_SLOT * key_val_ptr,          // key vals
    INTEL_KEYP_IV_SLOT * iv_ptr                 // iv vals
    );

#endif
