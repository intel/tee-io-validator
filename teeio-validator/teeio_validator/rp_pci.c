/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include "teeio_validator.h"
#include "hal/library/platform_lib.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>

int m_rp_fp = 0;

uint32_t get_ide_reg_block_offset(int fd, TEST_IDE_TYPE ide_type, uint8_t ide_id, uint32_t ide_ecap_offset);

INTEL_KEYP_STREAM_CONFIG_REG_BLOCK *get_stream_cfg_reg_block(
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *const kcbar_ptr,
    const uint16_t rp_stream_index)
{
    return (&kcbar_ptr->stream_config_reg_block) + rp_stream_index;
}

void kcbar_set_key_slot(
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *const kcbar_ptr,
    const uint8_t direction,
    const uint8_t slot_id,
    INTEL_KEYP_KEY_SLOT *const key_val_ptr,
    INTEL_KEYP_IV_SLOT *const iv_val_ptr)
{
    // INTEL_KEYP_IV_SLOT default_iv_val = {0};
    INTEL_KEYP_KEY_SLOT *key_slot_ptr = NULL;
    INTEL_KEYP_IV_SLOT *iv_slot_ptr = NULL;
    INTEL_KEYP_PCIE_STREAM_CAP kcbar_capabilities = {.raw = mmio_read_reg32(&kcbar_ptr->capabilities)};
    INTEL_KEYP_STREAM_CONFIG_REG_BLOCK *stream_config_reg_block = &kcbar_ptr->stream_config_reg_block;

    // Jump to Tx key_slots[0]
    key_slot_ptr = (INTEL_KEYP_KEY_SLOT *)(stream_config_reg_block + kcbar_capabilities.num_stream_supported + 1);
    // Jump to Tx iv_slots[0]
    iv_slot_ptr = (INTEL_KEYP_IV_SLOT *)(key_slot_ptr + kcbar_capabilities.num_tx_key_slots + 1);

    if (direction == PCIE_IDE_STREAM_TX)
    {
        // Jump to rx key_slots[0]
        key_slot_ptr = (INTEL_KEYP_KEY_SLOT *)(iv_slot_ptr + kcbar_capabilities.num_tx_key_slots + 1);
        // Jump to rx iv_slots[0]
        iv_slot_ptr = (INTEL_KEYP_IV_SLOT *)(key_slot_ptr + kcbar_capabilities.num_rx_key_slots + 1);
    }

    // Jump to correct key slot
    key_slot_ptr += slot_id;
    // Jump to correct iv slot
    iv_slot_ptr += slot_id;

    reg_memcpy_dw(key_slot_ptr, sizeof(INTEL_KEYP_KEY_SLOT), key_val_ptr, sizeof(INTEL_KEYP_KEY_SLOT));
    reg_memcpy_dw(iv_slot_ptr, sizeof(INTEL_KEYP_IV_SLOT), iv_val_ptr, sizeof(INTEL_KEYP_IV_SLOT));
}

void cfg_rc_ide_keys(
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *kcbar_ptr,
    const uint8_t rp_stream_index,        // N
    const uint8_t direction,     // RX TX
    const uint8_t key_set_select,// KS0 KS1
    const uint8_t sub_stream,    // PR NPR CPL
    const uint8_t slot_id,       // n
    INTEL_KEYP_KEY_SLOT * key_val_ptr,      // key vals
    INTEL_KEYP_IV_SLOT * iv_ptr             // iv vals
    )
{
    kcbar_set_key_slot(kcbar_ptr, direction, slot_id, key_val_ptr, iv_ptr);

    INTEL_KEYP_STREAM_CONFIG_REG_BLOCK *stream_cfg_reg_block = get_stream_cfg_reg_block(
        kcbar_ptr,
        rp_stream_index);

    INTEL_KEYP_STREAM_KEYSET_SLOT_ID *keyset_slot_id_ptr = NULL;
    if (direction == PCIE_IDE_STREAM_TX)
    {
        keyset_slot_id_ptr = key_set_select == PCIE_IDE_STREAM_KS0 ? &stream_cfg_reg_block->rx_key_set_0 : &stream_cfg_reg_block->rx_key_set_1;
    }
    else if (direction == PCIE_IDE_STREAM_RX)
    {
        keyset_slot_id_ptr = key_set_select == PCIE_IDE_STREAM_KS0 ? &stream_cfg_reg_block->tx_key_set_0 : &stream_cfg_reg_block->tx_key_set_1;
    }
    else
    {
        TEEIO_ASSERT(false);
    }

    // Replace the SLOT_ID for the specified SUB_STREAM
    INTEL_KEYP_STREAM_KEYSET_SLOT_ID stream_keyset_slot_id = {.raw = mmio_read_reg32(keyset_slot_id_ptr)};
    if (sub_stream == PCIE_IDE_SUB_STREAM_PR)
    {
        stream_keyset_slot_id.pr = slot_id;
    }
    else if (sub_stream == PCIE_IDE_SUB_STREAM_NPR)
    {
        stream_keyset_slot_id.npr = slot_id;
    }
    else if (sub_stream == PCIE_IDE_SUB_STREAM_CPL)
    {
        stream_keyset_slot_id.cpl = slot_id;
    }
    else
    {
        TEEIO_ASSERT(false);
    }

    mmio_write_reg32(keyset_slot_id_ptr, stream_keyset_slot_id.raw);
    return;
}

bool is_rp_ide_stream_enabled_in_kcbar(
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *const kcbar_ptr,
    const uint8_t rp_stream_index
)
{
    INTEL_KEYP_STREAM_CONFIG_REG_BLOCK *stream_cfg_reg_block = get_stream_cfg_reg_block(kcbar_ptr, rp_stream_index);

    INTEL_KEYP_STREAM_CONTROL *stream_ctrl_ptr = &stream_cfg_reg_block->control;
    INTEL_KEYP_STREAM_CONTROL stream_ctrl = {.raw = mmio_read_reg32(stream_ctrl_ptr)};
    return stream_ctrl.en == 1;
}

void set_rc_tx_ide_key_set(
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *const kcbar_ptr,
    const uint8_t rp_stream_index,
    const uint8_t key_set_select)
{
    INTEL_KEYP_STREAM_CONFIG_REG_BLOCK *stream_cfg_reg_block = get_stream_cfg_reg_block(kcbar_ptr, rp_stream_index);
    INTEL_KEYP_STREAM_TXRX_CONTROL *ctrl_reg_ptr = &stream_cfg_reg_block->tx_ctrl;

    INTEL_KEYP_STREAM_TXRX_CONTROL stream_txrx_control = {.raw = mmio_read_reg32(ctrl_reg_ptr)};
    if (key_set_select == PCIE_IDE_STREAM_KS0)
    {
        stream_txrx_control.stream_tx_control.key_set_select = 0b01;
    }
    else if (key_set_select == PCIE_IDE_STREAM_KS1)
    {
        stream_txrx_control.stream_tx_control.key_set_select = 0b10;
    }
    else
    {
        TEEIO_ASSERT(false);
    }
    mmio_write_reg32(ctrl_reg_ptr, stream_txrx_control.raw);
}

void prime_rc_ide_keys(
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *const kcbar_ptr,
    const uint8_t rp_stream_index,
    const uint8_t direction,
    const uint8_t key_set_select)
{
    INTEL_KEYP_STREAM_CONFIG_REG_BLOCK *stream_cfg_reg_block = get_stream_cfg_reg_block(kcbar_ptr, rp_stream_index);
    INTEL_KEYP_STREAM_TXRX_CONTROL *ctrl_reg_ptr = (direction == PCIE_IDE_STREAM_RX) ? &stream_cfg_reg_block->tx_ctrl : &stream_cfg_reg_block->rx_ctrl;
    INTEL_KEYP_STREAM_TXRX_STATUS *status_reg_ptr = (direction == PCIE_IDE_STREAM_RX) ? &stream_cfg_reg_block->tx_status : &stream_cfg_reg_block->rx_status;

    TEEIO_ASSERT(key_set_select < PCIE_IDE_STREAM_KS_NUM);

    INTEL_KEYP_STREAM_TXRX_CONTROL stream_txrx_control = {.raw = mmio_read_reg32(ctrl_reg_ptr)};
    if (key_set_select == PCIE_IDE_STREAM_KS0)
    {
        stream_txrx_control.common.prime_key_set_0 = 1;
    }
    else if (key_set_select == PCIE_IDE_STREAM_KS1)
    {
        stream_txrx_control.common.prime_key_set_1 = 1;
    }
    mmio_write_reg32(ctrl_reg_ptr, stream_txrx_control.raw);

    // check if ready_key_set_x is 1 after prime
    uint32_t data32 = mmio_read_reg32(status_reg_ptr);
    INTEL_KEYP_STREAM_TXRX_STATUS txrx_status = {.raw = data32};
    if (key_set_select == PCIE_IDE_STREAM_KS0) {
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "ready_key_set_0 = %d\n", txrx_status.common.ready_key_set_0));
    } else if (key_set_select == PCIE_IDE_STREAM_KS1) {
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "ready_key_set_1 = %d\n", txrx_status.common.ready_key_set_1));
    }
}
