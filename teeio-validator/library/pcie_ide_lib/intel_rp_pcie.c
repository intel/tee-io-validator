/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include "teeio_validator.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include "pcie_ide_internal.h"
#include "pcie_ide_lib.h"

int m_rp_fp = 0;

#define KCBAR_MEMORY_SIZE 1024

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

void cfg_rootport_ide_keys(
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

uint8_t* map_kcbar_addr(uint64_t addr, int* mapped_fd)
{
    // we need to map the kcbar_addr to user space
    if(addr == 0) {
        return NULL;
    }

    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if(mem_fd == -1) {
        TEEIO_DEBUG ((TEEIO_DEBUG_ERROR, "Failed to open /dev/mem\n"));
        return NULL;
    }
    uint8_t * mem_ptr = (uint8_t *)mmap(NULL, KCBAR_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, addr);
    if (mem_ptr == MAP_FAILED) {
        TEEIO_DEBUG ((TEEIO_DEBUG_ERROR, "Failed to mmap kcbar\n"));
        close(mem_fd);
        return NULL;
    }

    *mapped_fd = mem_fd;

    return mem_ptr;
}

bool unmap_kcbar_addr(int kcbar_mem_fd, uint8_t* mapped_kcbar_addr)
{
    if(kcbar_mem_fd > 0 && mapped_kcbar_addr != NULL){
        munmap(mapped_kcbar_addr, KCBAR_MEMORY_SIZE);
        close(kcbar_mem_fd);
        return true;
    }

    return false;
}


// Parse the KEYP table and map the kcbar address
bool parse_keyp_table(ide_common_test_port_context_t *port_context)
{
  const char *keyp_table = "/sys/firmware/acpi/tables/KEYP";
  const char KEYP_SIGNATURE[] = {'K', 'E', 'Y', 'P'};
  uint8_t buffer[4096] = {0};
  uint32_t i = 0;
  bool found = false;
  INTEL_KEYP_KEY_CONFIGURATION_UNIT *kcu = 0;
  INTEL_KEYP_ROOT_PORT_INFORMATION *krpi = 0;
  uint32_t offset = 0;
  uint32_t size = 0;
  uint64_t kcbar_addr = 0;

  if (port_context == NULL || port_context->port->port_type != IDE_PORT_TYPE_ROOTPORT)
  {
    TEEIO_ASSERT(false);
    return false;
  }

  port_context->kcbar_fd = -1;
  port_context->mapped_kcbar_addr = 0;

  int fd = open(keyp_table, O_RDONLY);
  if (fd == -1)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Error opening KEYP. (%s)\n", keyp_table));
    return false;
  }

  size = lseek(fd, 0x0, SEEK_END);
  TEEIO_ASSERT(size <= 4096);

  lseek(fd, 0, SEEK_SET);
  size_t bytes_read = read(fd, buffer, size);
  TEEIO_ASSERT(bytes_read == size);

  INTEL_KEYP_ACPI *keyp = (INTEL_KEYP_ACPI *)buffer;
  if (memcmp(keyp->signature, KEYP_SIGNATURE, sizeof(keyp->signature)) != 0)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid KEYP signature.\n"));
    return false;
  }

  uint8_t sum = calculate_checksum(buffer, size);
  if (sum != 0)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid Checksum.\n"));
    return false;
  }

  offset = sizeof(INTEL_KEYP_ACPI);
  while (offset < size)
  {
    kcu = (INTEL_KEYP_KEY_CONFIGURATION_UNIT *)(buffer + offset);
    TEEIO_ASSERT(offset + kcu->Length <= size);
    if (kcu->ProtocolType != 1)
    {
      offset += kcu->Length;
      continue;
    }

    for (i = 0; i < kcu->RootPortCount; i++)
    {
      krpi = (INTEL_KEYP_ROOT_PORT_INFORMATION *)(buffer + offset + sizeof(INTEL_KEYP_KEY_CONFIGURATION_UNIT) + i * sizeof(INTEL_KEYP_ROOT_PORT_INFORMATION));
      if (krpi->Bus == port_context->port->bus && krpi->Bits.Device == port_context->port->device && krpi->Bits.Function == port_context->port->function)
      {
        found = true;
        break;
      }
    }

    if (found)
    {
      kcbar_addr = kcu->RegisterBaseAddr;
      break;
    }

    offset += kcu->Length;
  }

  if (!found || kcbar_addr == 0)
  {
    return false;
  }

  // we need to map the kcbar_addr to user space
  port_context->mapped_kcbar_addr = map_kcbar_addr(kcbar_addr, &port_context->kcbar_fd);
  if (port_context->mapped_kcbar_addr == NULL)
  {
    TEEIO_ASSERT(false);
    return false;
  }

  return true;
}

// Initialize key configuration registers block
bool initialize_kcbar_registers(
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *kcbar,
    const uint8_t stream_id,
    const uint8_t rp_stream_index)
{
  INTEL_KEYP_STREAM_CONFIG_REG_BLOCK *stream_config_reg_block = (&kcbar->stream_config_reg_block) + rp_stream_index;

  mmio_write_reg32(&stream_config_reg_block->tx_ctrl, 0);
  mmio_write_reg32(&stream_config_reg_block->rx_ctrl, 0);
  mmio_write_reg32(&stream_config_reg_block->tx_key_set_0, 0);
  mmio_write_reg32(&stream_config_reg_block->tx_key_set_1, 0);
  mmio_write_reg32(&stream_config_reg_block->rx_key_set_0, 0);
  mmio_write_reg32(&stream_config_reg_block->rx_key_set_1, 0);

  INTEL_KEYP_STREAM_CONTROL stream_control = {.raw = 0};
  stream_control.stream_id = stream_id;
  mmio_write_reg32(&stream_config_reg_block->control, stream_control.raw);

  return true;
}

void enable_ide_stream_in_kcbar(
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *const kcbar_ptr,
    const uint8_t rp_stream_index,
    bool enable
)
{
    INTEL_KEYP_STREAM_CONFIG_REG_BLOCK *stream_cfg_reg_block = get_stream_cfg_reg_block(kcbar_ptr, rp_stream_index);

    // enable ide stream in kcbar
    INTEL_KEYP_STREAM_CONTROL *stream_ctrl_ptr = &stream_cfg_reg_block->control;
    INTEL_KEYP_STREAM_CONTROL stream_ctrl = {.raw = mmio_read_reg32(stream_ctrl_ptr)};
    stream_ctrl.en = enable ? 1 : 0;

    mmio_write_reg32(stream_ctrl_ptr, stream_ctrl.raw);
}

void dump_kcbar(
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *const kcbar_ptr,
    const uint8_t rp_stream_index
)
{
    INTEL_KEYP_PCIE_STREAM_CAP *stream_cap_ptr = &kcbar_ptr->capabilities;
    INTEL_KEYP_PCIE_STREAM_CAP stream_cap = {.raw = mmio_read_reg32(stream_cap_ptr)};
    TEEIO_PRINT(("stream_cap: %08x\n", stream_cap.raw));
    TEEIO_PRINT(("    num_stream_supported=%d, num_tx_key_slots=%d, num_rx_key_slots=%d\n",
                                        stream_cap.num_stream_supported,
                                        stream_cap.num_tx_key_slots,
                                        stream_cap.num_rx_key_slots));

    TEEIO_PRINT(("stream_config_reg: (stream_%c)\n", 'a' + rp_stream_index));
    INTEL_KEYP_STREAM_CONFIG_REG_BLOCK *stream_cfg_reg_ptr = get_stream_cfg_reg_block(kcbar_ptr, rp_stream_index);

    INTEL_KEYP_STREAM_CONTROL stream_control = {.raw = mmio_read_reg32(&stream_cfg_reg_ptr->control)};
    TEEIO_PRINT(("    stream_control : %08x (enable=%x, stream_id=%x)\n", stream_control.raw, stream_control.en, stream_control.stream_id));

    INTEL_KEYP_STREAM_TXRX_CONTROL tx_control = {.raw = mmio_read_reg32(&stream_cfg_reg_ptr->tx_ctrl)};
    TEEIO_PRINT(("    tx_control     : %08x (key_set_select=%x, prime_key_set_0=%x, prime_key_set_1=%x)\n",
                                        tx_control.raw,
                                        tx_control.stream_tx_control.key_set_select,
                                        tx_control.stream_tx_control.prime_key_set_0,
                                        tx_control.stream_tx_control.prime_key_set_1));

    INTEL_KEYP_STREAM_TXRX_STATUS tx_status = {.raw = mmio_read_reg32(&stream_cfg_reg_ptr->tx_status)};
    TEEIO_PRINT(("    tx_status      : %08x (key_set_status=%x, ready_key_set_0=%x, ready_key_set_1=%x)\n",
                                        tx_status.raw,
                                        tx_status.stream_tx_status.key_set_status,
                                        tx_status.stream_tx_status.ready_key_set_0,
                                        tx_status.stream_tx_status.ready_key_set_1));

    INTEL_KEYP_STREAM_TXRX_CONTROL rx_control = {.raw = mmio_read_reg32(&stream_cfg_reg_ptr->rx_ctrl)};
    TEEIO_PRINT(("    rx_control     : %08x (prime_key_set_0=%x, prime_key_set_1=%x)\n",
                                        rx_control.raw,
                                        rx_control.stream_rx_control.prime_key_set_0,
                                        rx_control.stream_rx_control.prime_key_set_1));

    INTEL_KEYP_STREAM_TXRX_STATUS rx_status = {.raw = mmio_read_reg32(&stream_cfg_reg_ptr->rx_status)};
    TEEIO_PRINT(("    rx_status      : %08x (ready_key_set_0=%x, ready_key_set_1=%x)\n",
                                        rx_status.raw,
                                        rx_status.stream_rx_status.ready_key_set_0,
                                        rx_status.stream_rx_status.ready_key_set_1));

    INTEL_KEYP_STREAM_KEYSET_SLOT_ID tx_ks0 = {.raw = mmio_read_reg32(&stream_cfg_reg_ptr->tx_key_set_0)};
    TEEIO_PRINT(("    tx_keyset_0    : pr=%x, npr=%x, cpl=%x\n", tx_ks0.pr, tx_ks0.npr, tx_ks0.cpl));

    INTEL_KEYP_STREAM_KEYSET_SLOT_ID rx_ks0 = {.raw = mmio_read_reg32(&stream_cfg_reg_ptr->rx_key_set_0)};
    TEEIO_PRINT(("    rx_keyset_0    : pr=%x, npr=%x, cpl=%x\n", rx_ks0.pr, rx_ks0.npr, rx_ks0.cpl));

    INTEL_KEYP_STREAM_KEYSET_SLOT_ID tx_ks1 = {.raw = mmio_read_reg32(&stream_cfg_reg_ptr->tx_key_set_1)};
    TEEIO_PRINT(("    tx_keyset_1    : pr=%x, npr=%x, cpl=%x\n", tx_ks1.pr, tx_ks1.npr, tx_ks1.cpl));

    INTEL_KEYP_STREAM_KEYSET_SLOT_ID rx_ks1 = {.raw = mmio_read_reg32(&stream_cfg_reg_ptr->rx_key_set_1)};
    TEEIO_PRINT(("    rx_keyset_1    : pr=%x, npr=%x, cpl=%x\n", rx_ks1.pr, rx_ks1.npr, rx_ks1.cpl));
}
