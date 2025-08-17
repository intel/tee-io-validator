/**
 *  Copyright Notice:
 *  Copyright 2023-2025 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>

#include "teeio_validator.h"
#include "hal/library/debuglib.h"
#include "teeio_debug.h"
#include "ide_test.h"

#include "teeio_spdmlib.h"
#include "pcie_ide_internal.h"
#include "pcie_ide_lib.h"


#define MAX_PCI_DOE_CNT 32

// PCIE_LINK_IDE_STREAM_CTRL:en PCIE_SEL_IDE_STREAM_CTRL:en
#define IDE_STREAM_CTRL_ENABLE 0x00000001

#define MEMORY_BASE_OFFSET 0x20
#define MEMORY_LIMIT_OFFSET 0x22
#define PREFETCH_MEMORY_BASE_OFFSET 0x24
#define PREFETCH_MEMORY_LIMIT_OFFSET 0x26
#define PREFETCH_MEMORY_BASE_UPPER_OFFSET 0x28
#define PREFETCH_MEMORY_LIMIT_UPPER_OFFSET 0x2c

#define LINK_IDE_REGISTER_BLOCK_SIZE (sizeof(PCIE_LNK_IDE_STREAM_CTRL) + sizeof(PCIE_LINK_IDE_STREAM_STATUS))

extern uint32_t g_doe_extended_offset;

uint32_t g_doe_extended_offset = 0;
uint32_t g_ide_extended_offset = 0;
uint32_t g_aer_extended_offset = 0;
int m_dev_fp = 0;

const char *m_ide_type_name[] = {
  "SelectiveIDE",
  "LinkIDE",
};

const char *ks_names[] = {
    "KS0", "KS1"};

const char *direct_names[] = {
    "RX", "TX"};
const char *ctrl_reg_names[] = {
    "tx_ctrl", "rx_ctrl"
};

PCIE_SEL_IDE_RID_ASSOC_REG_BLOCK m_rid_assoc_reg_block = {
    .rid_assoc1 = {.raw = 0xffff00},
    .rid_assoc2 = {.raw = 0x1}
};

PCIE_SEL_IDE_ADDR_ASSOC_REG_BLOCK m_addr_assoc_reg_block = {
    .addr_assoc1 = {.raw = 0xfff00001},
    .addr_assoc2 = {.raw = 0xffffffff},
    .addr_assoc3 = {.raw = 0},
};

int open_configuration_space(char *bdf)
{
  if (bdf == NULL)
  {
    TEEIO_ASSERT(false);
    return -1;
  }

  char buf[MAX_FILE_NAME];
  sprintf(buf, "/sys/bus/pci/devices/0000:%s/config", bdf);

  int fd = open(buf, O_RDWR);
  if (fd == -1)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "open %s error!\n", buf));
    return -1;
  }
  else
  {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "successful open device with fd: %d\n", fd));
  }
  uint32_t size = lseek(fd, 0x0, SEEK_END);
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cfg space size = %x\n", size));

  if (size > 0x1000)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cfg space size is not valid!(bdf: %s)\n", bdf));
    close(fd);
    return -1;
  }

  return fd;
}


uint32_t get_cap_offset(int fd, uint32_t cap_id)
{
  uint32_t walker = 0;
  uint32_t cap_header = 0;
  uint32_t offset = 0;

  // get capability start from Type 0/1 header
  uint16_t data16;
  data16 = device_pci_read_16(0x34, fd);
  walker = data16 & 0xFF; // CAPABILITY_POINTER

  if (cap_id != PCIE_CAPABILITY_ID)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Not supported cap id: 0x%x\n", cap_id));
    return 0;
  }

  while (walker < PCIE_EXT_CAP_START && walker != 0)
  {
    cap_header = device_pci_read_32(walker, fd);

    if (((PCIE_CAP_LIST *)&cap_header)->id == cap_id)
    {
      offset = walker;
      break;
    }

    walker = ((PCIE_CAP_LIST *)&cap_header)->next_cap_offset;
  }

  return offset;
}


uint32_t get_extended_cap_offset(int fd, uint32_t ext_id)
{
  uint32_t ext_cap_start = 0x100; // defined by PCIe Specification
  uint32_t walker = ext_cap_start;
  uint32_t cap_ext_header = 0;
  uint32_t offset = 0;

  if (ext_id != PCI_DOE_EXT_CAPABILITY_ID &&
      ext_id != PCI_IDE_EXT_CAPABILITY_ID &&
      ext_id != PCI_AER_EXT_CAPABILITY_ID)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Not supported extended cap id: 0x%x\n", ext_id));
    return 0;
  }

  while (walker < 0x1000 && walker != 0)
  {
    cap_ext_header = device_pci_read_32(walker, fd);

    if (((PCIE_CAP_ID *)&cap_ext_header)->id == ext_id)
    {
      offset = walker;
      break;
    }

    walker = ((PCIE_CAP_ID *)&cap_ext_header)->next_cap_offset;
  }

  return offset;
}

// There may be multi DOE Extened Caps in ecap. This function walks thru the
// extended caps and find out all the DOE Extended caps offset.
bool get_doe_extended_cap_offset(int fd, uint32_t* doe_offsets, int* size)
{
  uint32_t ext_cap_start = 0x100; // defined by PCIe Specification
  uint32_t walker = ext_cap_start;
  uint32_t cap_ext_header = 0;
  int cnt = 0;

  TEEIO_ASSERT(size != NULL);

  while (walker < 0x1000 && walker != 0)
  {
    cap_ext_header = device_pci_read_32(walker, fd);

    if (((PCIE_CAP_ID *)&cap_ext_header)->id == PCI_DOE_EXT_CAPABILITY_ID)
    {
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Find DOE Extended Cap at offset - 0x%04x\n", walker));

      if(doe_offsets != NULL) {
        TEEIO_ASSERT(cnt < *size);
        doe_offsets[cnt] = walker;
      }
      cnt += 1;
    }

    walker = ((PCIE_CAP_ID *)&cap_ext_header)->next_cap_offset;
  }

  *size = cnt;
  return cnt > 0;
}

bool close_root_port(pcie_ide_test_group_context_t *group_context)
{
  // clean Link/Selective IDE Stream Control Registers and KCBar corresponding registers
  ide_common_test_port_context_t* port_context = &group_context->common.upper_port;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "close_root_port %s(%s)\n", port_context->port->port_name, port_context->port->bdf));

  port_context->addr_assoc_reg_block.addr_assoc1.raw = 0;
  port_context->addr_assoc_reg_block.addr_assoc2.raw = 0;
  port_context->rid_assoc_reg_block.rid_assoc1.raw = 0;
  port_context->rid_assoc_reg_block.rid_assoc2.raw = 0;

  reset_ide_registers(port_context, group_context->common.top->type, 0, group_context->rp_stream_index, true);

  if(group_context->common.upper_port.kcbar_fd > 0) {
    unmap_kcbar_addr(group_context->common.upper_port.kcbar_fd, group_context->common.upper_port.mapped_kcbar_addr);
  }
  group_context->common.upper_port.kcbar_fd = 0;
  group_context->common.upper_port.mapped_kcbar_addr = 0;

  if(group_context->common.upper_port.cfg_space_fd > 0) {
    close(group_context->common.upper_port.cfg_space_fd);
    unset_device_info(group_context->common.upper_port.cfg_space_fd);
  }
  group_context->common.upper_port.cfg_space_fd = 0;
  
  return true;
}

void dump_rid_assoc_reg_block(PCIE_SEL_IDE_RID_ASSOC_REG_BLOCK * reg_block)
{
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "rid_assoc1.raw = 0x%08x (rid_limit = 0x%x)\n", reg_block->rid_assoc1.raw, reg_block->rid_assoc1.rid_limit));
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "rid_assoc2.raw = 0x%08x (rid_base = 0x%x, valid=%x)\n", reg_block->rid_assoc2.raw, reg_block->rid_assoc2.rid_base, reg_block->rid_assoc2.valid));
}
void dump_addr_assoc_reg_block(PCIE_SEL_IDE_ADDR_ASSOC_REG_BLOCK * reg_block)
{
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "addr_assoc1.raw = 0x%08x (mem_base_lower = 0x%x, mem_limit_lower = 0x%x, valid = %x)\n",
                                        reg_block->addr_assoc1.raw, reg_block->addr_assoc1.mem_base_lower, reg_block->addr_assoc1.mem_limit_lower, reg_block->addr_assoc1.valid));
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "addr_assoc2.raw = 0x%08x (mem_limit_upper = 0x%x)\n",
                                        reg_block->addr_assoc2.raw, reg_block->addr_assoc2.mem_limit_upper));
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "addr_assoc3.raw = 0x%08x (mem_base_upper = 0x%x)\n",
                                        reg_block->addr_assoc3.raw, reg_block->addr_assoc3.mem_base_upper));
}

bool open_root_port(ide_common_test_port_context_t *port_context)
{
  char str[MAX_NAME_LENGTH] = {0};

  IDE_PORT *port = port_context->port;
  TEEIO_ASSERT(port->port_type == IDE_PORT_TYPE_ROOTPORT);

  // open configuration space and get ecap offset
  int fd = open_configuration_space(port->bdf);
  if (fd == -1) {
    return false;
  }

  port_context->cfg_space_fd = fd;
  sprintf(str, "host : %s", port->bdf);
  set_deivce_info(fd, str);

  uint32_t ecap_offset = get_extended_cap_offset(fd, PCI_IDE_EXT_CAPABILITY_ID);
  if (ecap_offset == 0)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "ECAP Offset of IDE is NOT found\n"));
    goto InitRootPortFail;
  }
  else
  {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "ECAP Offset of IDE: 0x%016x\n", ecap_offset));
  }

  port_context->ecap_offset = ecap_offset;

  // parse KEYP table and map the kcbar to user space
  if (!parse_keyp_table(port_context, INTEL_KEYP_PROTOCOL_TYPE_PCIE_CXLIO))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "parse_keyp_table failed.\n"));
    goto InitRootPortFail;
  }

  // store the stream_cap in kcbar and pci_ide_cap in ecap(@configuration space)
  INTEL_KEYP_ROOT_COMPLEX_KCBAR *kcbar = (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)port_context->mapped_kcbar_addr;
  port_context->stream_cap.raw = mmio_read_reg32(&kcbar->capabilities);

  uint32_t offset = ecap_offset + 4;
  port_context->ide_cap.raw = device_pci_read_32(offset, fd);

  return true;

InitRootPortFail:
  close(fd);
  unset_device_info(fd);
  return false;
}

/**
 * This function is to find free key/iv slots for PCIE-IDE stream.
 *
 * There are 3 substreams (PR/NPR/CPL) in a PCIE-IDE stream. We assume
 * the key/iv slots allocated for these substreams are continuous. For
 * example, 0|1|2 or 3|4|5.
 */
bool pcie_ide_alloc_slot_ids(ide_common_test_port_context_t* port_context, uint8_t rp_stream_index, ide_key_set_t* k_set)
{
  int i = 0;
  bool found = false;
  uint8_t key_iv_slot_usage_map = 0;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Allocate kv/iv slot for %s. Its rp_stream_index is stream_%c\n", port_context->port->port_name, rp_stream_index + 'a'));

  TEEIO_ASSERT(port_context->port->port_type == IDE_PORT_TYPE_ROOTPORT);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Walk thru Rootport KCBar stream_x to collect the used key/iv slots.\n"));

  INTEL_KEYP_ROOT_COMPLEX_KCBAR *kcbar = (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)port_context->mapped_kcbar_addr;
  INTEL_KEYP_PCIE_STREAM_CAP stream_cap = { .raw = mmio_read_reg32(&kcbar->capabilities)};
  int num_stream_supported = stream_cap.num_stream_supported + 1;
  int num_key_iv_slots = stream_cap.num_tx_key_slots + 1;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "num_stream_supported=%d, num_key_iv_slots=%d\n", num_stream_supported, num_key_iv_slots));
  TEEIO_ASSERT(num_key_iv_slots % 3 == 0);

  // We use key_iv_slot_usage_map to indicate the usage of key/iv slots.
  // We assume PR/NPR/CPL substreams are allocated continuously. So 1 bit in key_iv_slot_usage_map represents 3 key/iv slots. 
  // In the future if the supported key/iv slots exceeds key_iv_slot_usage_map, we will revisit here.
  if(num_key_iv_slots > sizeof(key_iv_slot_usage_map) * 8 * 3) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "supported num_key_iv_slots (%d) exceeds key_iv_slot_usage_map (%d) and it shall be expanded.\n", num_key_iv_slots, sizeof(key_iv_slot_usage_map) * 8 * 3));
    return false;
  }

  for(i = 0; i < num_stream_supported; i++) {
    INTEL_KEYP_STREAM_CONFIG_REG_BLOCK *stream_config_reg_block = &kcbar->stream_config_reg_block + i;
    INTEL_KEYP_STREAM_CONTROL stream_ctrl = {.raw = mmio_read_reg32(&stream_config_reg_block->control)};

    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "stream_%c:\n", 'a' + i));
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    stream_control: enable=%d, stream_id=%d\n", stream_ctrl.en, stream_ctrl.stream_id));
    if(stream_ctrl.en == 0) {
      continue;
    }

    INTEL_KEYP_STREAM_TXRX_CONTROL tx_ctrl = {.raw = mmio_read_reg32(&stream_config_reg_block->tx_ctrl)};
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    tx_control    : key_set_select=%d, prime_key_set_0=%d, prime_key_set_1=%d\n",
                                    tx_ctrl.stream_tx_control.key_set_select,
                                    tx_ctrl.stream_tx_control.prime_key_set_0,
                                    tx_ctrl.stream_tx_control.prime_key_set_1));

    INTEL_KEYP_STREAM_KEYSET_SLOT_ID* keyset_ptr;
    if(tx_ctrl.stream_tx_control.key_set_select == 1) {
      keyset_ptr = &stream_config_reg_block->tx_key_set_0;
    } else if(tx_ctrl.stream_tx_control.key_set_select == 2) {
      keyset_ptr = &stream_config_reg_block->tx_key_set_1;
    } else {
      TEEIO_ASSERT(false);
    }
    INTEL_KEYP_STREAM_KEYSET_SLOT_ID keyset = {.raw = mmio_read_reg32(keyset_ptr)};

    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    keyset_%d    : pr=%d, npr=%d, cpl=%d\n",
                                    tx_ctrl.stream_tx_control.key_set_select - 1,
                                    keyset.pr, keyset.npr, keyset.cpl));

    TEEIO_ASSERT(keyset.pr < num_key_iv_slots);
    TEEIO_ASSERT(keyset.pr % 3 == 0);
    TEEIO_ASSERT(keyset.npr < num_key_iv_slots);
    TEEIO_ASSERT(keyset.cpl < num_key_iv_slots);

    key_iv_slot_usage_map = (1 << keyset.pr/3) | key_iv_slot_usage_map;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "key/iv slot usage: key_iv_slot_usage_map=%02x\n", key_iv_slot_usage_map));
  for(i = 0; i < num_key_iv_slots / 3; i++) {
    uint8_t key_iv_slot = (key_iv_slot_usage_map >> i) & 0x1;
    // 0 indicates the slots are free, 1 indicates the slots are occupied.
    if(key_iv_slot == 0) {
      k_set->slot_id[PCIE_IDE_STREAM_RX][PCIE_IDE_SUB_STREAM_PR] = i * 3;
      k_set->slot_id[PCIE_IDE_STREAM_RX][PCIE_IDE_SUB_STREAM_NPR] = i * 3 + 1;
      k_set->slot_id[PCIE_IDE_STREAM_RX][PCIE_IDE_SUB_STREAM_CPL] = i * 3 + 2;

      k_set->slot_id[PCIE_IDE_STREAM_TX][PCIE_IDE_SUB_STREAM_PR] = i * 3;
      k_set->slot_id[PCIE_IDE_STREAM_TX][PCIE_IDE_SUB_STREAM_NPR] = i * 3 + 1;
      k_set->slot_id[PCIE_IDE_STREAM_TX][PCIE_IDE_SUB_STREAM_CPL] = i * 3 + 2;
      found = true;
      break;
    }
  }

  if(found) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Find free key/iv slots: pr=%d, npr=%d, cpl=%d\n", i*3, i*3 + 1, i*3 + 2));
  } else {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to find free key/iv slots.\n"));
  }

  return found;
}

/**
 * rp_stream_index indicates the Per-Stream Configuration slot in KCBar (Figure 2-1), for example Stream A or B etc.
 * ide_id indicates the Link/Selective IDE Stream Register Block in ecap (Figure 7-1).
*/
bool find_free_rp_stream_index_and_ide_id(ide_common_test_port_context_t* port_context, uint8_t* rp_stream_index, uint8_t* ide_id, IDE_TEST_TOPOLOGY_TYPE top_type, bool rp_or_dev)
{
  int i;

  IDE_TEST_IDE_TYPE ide_type = IDE_TEST_IDE_TYPE_SEL_IDE;
  if(top_type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE) {
    ide_type = IDE_TEST_IDE_TYPE_LNK_IDE;
  } else if(top_type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE) {
    NOT_IMPLEMENTED("find_free_rp_stream_index_and_ide_id for IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE");
    return false;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "find_free_rp_stream_index_and_ide_id for %s\n", port_context->port->port_name));

  PCIE_IDE_CAP ide_cap = {.raw = port_context->ide_cap.raw};

  int num_lnk_ide = ide_cap.lnk_ide_supported == 1 ? ide_cap.num_lnk_ide + 1 : 0;
  int num_sel_ide = ide_cap.sel_ide_supported == 1 ? ide_cap.num_sel_ide + 1 : 0;

  // skip IDE Extended Capability Header, IDE Capability Register and IDE Control Register.
  int offset = port_context->ecap_offset + sizeof(PCIE_CAP_ID) + sizeof(PCIE_IDE_CAP) + sizeof(PCIE_IDE_CTRL);
  bool found = false;

  if(ide_type == IDE_TEST_IDE_TYPE_LNK_IDE) {
    // Try to find a free Link IDE Stream Register Block in ecap
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Walk thru Link IDE Stream Register Blocks to find a free one.\n"));
    if(ide_cap.lnk_ide_supported == 0) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Link IDE is not supported.\n"));;
      goto FindFreeLinkSelectiveIDERegisterBlockDone;
    }
    for(i = 0; i < num_lnk_ide; i++) {
      PCIE_LNK_IDE_STREAM_CTRL lnk_ide_stream_ctrl = {.raw = device_pci_read_32(offset, port_context->cfg_space_fd)};
      PCIE_LINK_IDE_STREAM_STATUS lnk_ide_stream_status = {.raw = device_pci_read_32(offset + 4, port_context->cfg_space_fd)};
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%d: lnk_ide_stream_ctrl = 0x%08x, lnk_ide_stream_status = 0x%08x\n", i, lnk_ide_stream_ctrl.raw, lnk_ide_stream_status.raw));

      if(lnk_ide_stream_ctrl.enabled == 0) {
        // This Link IDE Stream Register Block is not enabled.
        *ide_id = i;
        found = true;
        break;
      }
      offset += LINK_IDE_REGISTER_BLOCK_SIZE;
    }
  } else {
    // find a free Selective IDE Stream Register Block in ecap
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Walk thru Selective IDE Stream Register Blocks to find a free one.\n"));
    if(ide_cap.sel_ide_supported == 0) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Selective IDE is not supported.\n"));
      goto FindFreeLinkSelectiveIDERegisterBlockDone;
    }

    offset += (num_lnk_ide * LINK_IDE_REGISTER_BLOCK_SIZE);
    for(i = num_lnk_ide; i < num_lnk_ide + num_sel_ide; i++) {
      PCIE_SEL_IDE_STREAM_CAP sel_ide_stream_cap = {.raw = device_pci_read_32(offset, port_context->cfg_space_fd)};
      offset += 4;
      PCIE_SEL_IDE_STREAM_CTRL sel_ide_stream_ctrl = {.raw = device_pci_read_32(offset, port_context->cfg_space_fd)};
      offset += 4;
      PCIE_SEL_IDE_STREAM_STATUS sel_ide_stream_status = {.raw = device_pci_read_32(offset, port_context->cfg_space_fd)};
      offset += 4;

      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%d: sel_ide_stream_ctrl = 0x%08x, sel_ide_stream_status = 0x%08x\n", i, sel_ide_stream_ctrl.raw, sel_ide_stream_status.raw));
      if(sel_ide_stream_ctrl.enabled == 0) {
        // This Selective IDE Stream Register Block is not enabled.
        *ide_id = i;
        found = true;
        break;
      }

      // skip 2 RID Assoc Register (2*4) and 3 Addr Assoc Register (3*4).
      // Addr Assoc Register may have num_addr_assoc_reg_blocks
      offset += (2*4 + sel_ide_stream_cap.num_addr_assoc_reg_blocks * 3 * 4);
    }
  }

FindFreeLinkSelectiveIDERegisterBlockDone:
  if(!found) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to find free Link/Selective IDE Register Block.\n"));
    return false;
  }

  // Now try to find a free stream_x in KCBar if it is rootport
  if(rp_or_dev) {
    found = false;
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Walk thru Rootport KCBar stream_x to find a free one.\n"));
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *kcbar = (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)port_context->mapped_kcbar_addr;
    int num_stream_supported = kcbar->capabilities.num_stream_supported + 1;

    for(i = 0; i < num_stream_supported; i++) {
      INTEL_KEYP_STREAM_CONFIG_REG_BLOCK *stream_config_reg_block = (&kcbar->stream_config_reg_block) + i;
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%d: stream_config_reg_block.control = 0x%08x\n", i, stream_config_reg_block->control.raw));
      if(stream_config_reg_block->control.en == 0) {
        *rp_stream_index = (uint8_t)i;
        found = true;
        break;
      }
    }

    if(!found) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to find free Stream_x in KCBar.\n"));
      return false;
    } else {
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "rp_stream_index = %d\n", *rp_stream_index));
    }
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "ide_id = %d\n", *ide_id));

  return true;
}

bool populate_rid_assoc_reg_block(
    PCIE_SEL_IDE_RID_ASSOC_REG_BLOCK *rid_assoc_reg_block,
    uint8_t bus, uint8_t device, uint8_t func)
{
    if(rid_assoc_reg_block == NULL) {
        return false;
    }
    uint16_t rid_base = (uint16_t)(bus<<8) + device;
    uint16_t rid_limit = rid_base + func + 1;
    PCIE_SEL_IDE_RID_ASSOC_1 rid_assoc1 = {.rid_limit = rid_limit, .rsvd0 = 0, .rsvd1 = 0};
    rid_assoc_reg_block->rid_assoc1.raw = rid_assoc1.raw;
    PCIE_SEL_IDE_RID_ASSOC_2 rid_assoc2 = {.rid_base = rid_base, .valid = 1, .rsvd0 = 0, .rsvd1 = 0};
    rid_assoc_reg_block->rid_assoc2.raw = rid_assoc2.raw;

    return true;
}

// If the device is 64-bit and bit3 (prefetchable bit) is set, then use_prefetchable registers
bool check_use_prefetchable(char* bdf)
{
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "check_use_prefetchable at bdf=%s\n", bdf));
  int fd = open_configuration_space(bdf);
  if(fd == -1) {
    TEEIO_ASSERT(false);
    return false;
  }

  bool use_prefetchable = false;

  uint32_t  bar0 = device_pci_read_32(PCIE_BAR0_OFFSET, fd);
  if((bar0 & PCIE_MEM_BASE_ADDR_MASK) == PCIE_MEM_BASE_ADDR_64) {
    // check bit 3
    if((bar0 & PCIE_MEM_BASE_PREFETCHABLE_MASK) != 0) {
      use_prefetchable = true;
    }
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Device bar0 is 0x%08x, use_prefetchable = %d\n", bar0, use_prefetchable));

  close(fd);

  return use_prefetchable;
}

bool populate_addr_assoc_reg_block(
    char *bdf,
    bool use_prefetch_memory,
    PCIE_SEL_IDE_ADDR_ASSOC_REG_BLOCK *addr_assoc_reg_block)
{
    if(addr_assoc_reg_block == NULL) {
      return false;
    }

    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "populate_addr_assoc_reg_block. Value of Mem Base/Limit are read from bdf=%s\n", bdf));
    int cfg_space_fd = open_configuration_space(bdf);
    if(cfg_space_fd == -1) {
      TEEIO_ASSERT(false);
      return false;
    }

    // populate addr_assoc_1
    int mem_base_offset = MEMORY_BASE_OFFSET;
    if(use_prefetch_memory) {
        mem_base_offset = PREFETCH_MEMORY_BASE_OFFSET;
    }

    uint32_t data32 = device_pci_read_32(mem_base_offset, cfg_space_fd);
    uint16_t memory_base = (uint16_t)(data32 & 0x0000fff0) >> 4;
    uint16_t memory_limit = (uint16_t)((data32 & 0xfff00000) >> 20);

    PCIE_SEL_IDE_ADDR_ASSOC_1 addr_assoc1 = {
        .mem_base_lower = memory_base,
        .mem_limit_lower = memory_limit,
        .rsvd = 0,
        .valid = 1};
    addr_assoc_reg_block->addr_assoc1.raw = addr_assoc1.raw;

    // populate addr_assoc 2/3
    PCIE_SEL_IDE_ADDR_ASSOC_2 addr_assoc2 = {.raw = 0};
    PCIE_SEL_IDE_ADDR_ASSOC_3 addr_assoc3 = {.raw = 0};
    if(use_prefetch_memory) {
        addr_assoc2.mem_limit_upper = device_pci_read_32(PREFETCH_MEMORY_LIMIT_UPPER_OFFSET, cfg_space_fd);
        addr_assoc3.mem_base_upper = device_pci_read_32(PREFETCH_MEMORY_BASE_UPPER_OFFSET, cfg_space_fd);
    }

    addr_assoc_reg_block->addr_assoc2.raw = addr_assoc2.raw;
    addr_assoc_reg_block->addr_assoc3.raw = addr_assoc3.raw;

    close(cfg_space_fd);

    return true;
}

/**
 * To check if the stream_id is used in rootport.
 */
static bool check_stream_id_used_in_rootport(uint8_t stream_id, ide_common_test_port_context_t* root_port_context)
{
  TEEIO_ASSERT(root_port_context != NULL);

  int i = 0;

  // First walk thru Key Configuration Unit Register Block
  INTEL_KEYP_ROOT_COMPLEX_KCBAR *kcbar = (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)root_port_context->mapped_kcbar_addr;
  int num_stream_supported = kcbar->capabilities.num_stream_supported + 1;
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Walk thru Rootport KCBar stream_x to check if stream_id(%d) is used.\n", stream_id));
  for(i = 0; i < num_stream_supported; i++) {
    INTEL_KEYP_STREAM_CONFIG_REG_BLOCK *stream_config_reg_block = (&kcbar->stream_config_reg_block) + i;
    if(stream_config_reg_block->control.en == 1 && stream_config_reg_block->control.stream_id == stream_id) {
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "stream_id (%d) is used in stream_%c\n", stream_id, 'a' + i));
      return true;
    }
  }

  // Then walk thru IDE ECap
  uint32_t offset = root_port_context->ecap_offset;
  int fd = root_port_context->cfg_space_fd;
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Walk thru Rootport IDE Extended Cap to check if stream_id(%d) is used.\n", stream_id));

  offset += 4;  // refer to PCIE_IDE_ECAP
  PCIE_IDE_CAP ide_cap = {.raw = device_pci_read_32(offset, fd)};
  int num_lnk_ide = ide_cap.lnk_ide_supported == 1 ? ide_cap.num_lnk_ide + 1 : 0;
  int num_sel_ide = ide_cap.sel_ide_supported == 1 ? ide_cap.num_sel_ide + 1 : 0;

  offset += 4;  // refer to PCIE_IDE_CTRL

  if(num_lnk_ide > 0) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    Check Link IDE Stream Control Register block ...\n"));
    offset += 4;  // refer to PCIE_SEL_IDE_STREAM_CAP

    for(i = 0; i < num_lnk_ide; i++) {
      PCIE_LNK_IDE_STREAM_CTRL lnk_ide_stream_ctrl = {.raw = device_pci_read_32(offset, fd)};
      if(lnk_ide_stream_ctrl.enabled == 1 && lnk_ide_stream_ctrl.stream_id == stream_id) {
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "stream_id (%d) is used in Link IDE Stream Register Block (%d)\n", stream_id, i));
        return true;
      }
      offset += LINK_IDE_REGISTER_BLOCK_SIZE;
    }
  }

  if(num_sel_ide > 0) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    Check Selective IDE Stream Control Register block ...\n"));
    for(i = 0; i < num_sel_ide; i++) {
      PCIE_SEL_IDE_STREAM_CAP sel_ide_stream_cap = {.raw = device_pci_read_32(offset, fd)};
      PCIE_SEL_IDE_STREAM_CTRL sel_ide_stream_ctrl = {.raw = device_pci_read_32(offset + 4, fd)};
      if(sel_ide_stream_ctrl.enabled == 1 && sel_ide_stream_ctrl.stream_id == stream_id) {
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "stream_id (%d) is used in Selective IDE Stream Register Block (%d)\n", stream_id, i));
        return true;
      }

      // skip to next Selective IDE Stream Control Register block
      offset += (3 * 4 + 2 *4 + sel_ide_stream_cap.num_addr_assoc_reg_blocks * 3 * 4);
    }
  }

  return false;
}

/**
 * Initialize rootcomplex port
 * root_port and upper_port is the same port
 */
bool init_root_port(pcie_ide_test_group_context_t *group_context)
{
  TEEIO_ASSERT(group_context != NULL);
  TEEIO_ASSERT(group_context->common.top != NULL);

  // IDE_TEST_TOPOLOGY_TYPE top_type = group_context->common.top->type;
  ide_common_test_port_context_t *port_context = &group_context->common.upper_port;
  TEEIO_ASSERT(port_context != NULL);
  TEEIO_ASSERT(port_context->port != NULL);
  TEEIO_ASSERT(port_context->port->port_type == IDE_PORT_TYPE_ROOTPORT);
  TEEIO_ASSERT(group_context->common.upper_port.port->id == group_context->common.root_port.port->id);

  if(!open_root_port(port_context)) {
    return false;
  }

  // then set the rp_stream_index/ide_id/slot_id
  if(check_stream_id_used_in_rootport(group_context->common.top->stream_id, port_context)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "stream_id (%d) is already used.\n", group_context->common.top->stream_id));
    goto InitHostFail;
  }
  group_context->stream_id = group_context->common.top->stream_id;

  uint8_t rp_stream_index = 0;
  uint8_t ide_id = 0;
  if(!find_free_rp_stream_index_and_ide_id(port_context, &rp_stream_index, &ide_id, group_context->common.top->type, true)) {
    goto InitHostFail;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Host ide_id=%d, rp_stream_index=%d\n", ide_id, rp_stream_index));
  group_context->rp_stream_index = rp_stream_index;
  port_context->ide_id = ide_id;

  // rid & addr assoc_reg_block
  ide_common_test_port_context_t *lower_port_context = &group_context->common.lower_port;
  populate_rid_assoc_reg_block(&port_context->rid_assoc_reg_block, lower_port_context->port->bus, lower_port_context->port->device, lower_port_context->port->function);

  ide_common_test_switch_internal_conn_context_t *itr = NULL;
  char* dev_bdf = NULL;
  if(group_context->common.top->connection == IDE_TEST_CONNECT_SWITCH) {

    itr = group_context->common.sw_conn1;
    while(itr->next != NULL) {
      itr = itr->next;
    }
    dev_bdf = itr->dps.port->bdf;
  } else if(group_context->common.top->connection == IDE_TEST_CONNECT_P2P) {
    itr = group_context->common.sw_conn2;
    while(itr->next != NULL) {
      itr = itr->next;
    }
    dev_bdf = itr->dps.port->bdf;

  } else {
    dev_bdf = port_context->port->bdf;
  }

  bool use_prefetch_memory = check_use_prefetchable(dev_bdf);
  if(!populate_addr_assoc_reg_block(dev_bdf, use_prefetch_memory, &port_context->addr_assoc_reg_block)) {
    goto InitHostFail;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "dump host assoc_reg_block:\n"));
  dump_rid_assoc_reg_block(&port_context->rid_assoc_reg_block);
  dump_addr_assoc_reg_block(&port_context->addr_assoc_reg_block);

  // allocate key/iv slots
  if(!pcie_ide_alloc_slot_ids(port_context, group_context->rp_stream_index, &group_context->k_set)) {
    goto InitHostFail;
  }

  return true;

InitHostFail:
  close(port_context->cfg_space_fd);
  unset_device_info(port_context->cfg_space_fd);
  return false;
}

bool init_pci_doe(int fd)
{
  uint32_t doe_extended_offsets[MAX_PCI_DOE_CNT] = {0};
  int doe_cnt = MAX_PCI_DOE_CNT;
  if(!get_doe_extended_cap_offset(fd, doe_extended_offsets, &doe_cnt)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Can not find PCI DOE Extended Cap!\n"));
    return false;
  }

  PCIE_CAP_ID ecap_id = {.raw = 0};
  uint8_t doe_discovery_version = 0;
  for(int i = 0; i < doe_cnt; i++) {
    g_doe_extended_offset = doe_extended_offsets[i];
    ecap_id.raw = device_pci_read_32(g_doe_extended_offset, fd);
    if(ecap_id.version >= PCIE_DOE_ECAP_VERSION2) {
      // PCIE Spec 6.1 Section 6.30.1.1
      doe_discovery_version = PCIE_DOE_DISCOVERY_VERSION2;
    }
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Try to init pci_doe (doe_offset=0x%04x, ecap_id=0x%08x)\n", g_doe_extended_offset, ecap_id.raw));

    // Before init PCI DOE, we need to abort any ongoing doe operation and check the Error bit.
    trigger_doe_abort();
    libspdm_sleep(1000 * 1000);
    if (is_doe_error_asserted()) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "PCI DOE Error bit is set and cannot be cleared.\n"));
      continue;
    }

    if(pcie_doe_init_request(doe_discovery_version)) {
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "doe_offset=0x%04x is the one to be used in teeio-validator.\n", g_doe_extended_offset));
      return true;
    }
  }

  return false;
}

bool close_dev_port(ide_common_test_port_context_t *port_context, IDE_TEST_TOPOLOGY_TYPE top_type)
{
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "close_dev_port %s(%s)\n", port_context->port->port_name, port_context->port->bdf));

  // clean Link/Selective IDE Stream Control Registers
  port_context->addr_assoc_reg_block.addr_assoc1.raw = 0;
  port_context->addr_assoc_reg_block.addr_assoc2.raw = 0;
  port_context->rid_assoc_reg_block.rid_assoc1.raw = 0;
  port_context->rid_assoc_reg_block.rid_assoc2.raw = 0;

  reset_ide_registers(port_context, top_type, 0, 0, false);

  if(port_context->cfg_space_fd > 0) {
    close(port_context->cfg_space_fd);
    unset_device_info(port_context->cfg_space_fd);
  }
  port_context->cfg_space_fd = 0;

  m_dev_fp = 0;
  g_doe_extended_offset = 0;
  return true;
}

bool open_dev_port(ide_common_test_port_context_t *port_context)
{
  TEEIO_ASSERT(port_context != NULL);
  IDE_PORT *port = port_context->port;
  TEEIO_ASSERT(port != NULL);
  TEEIO_ASSERT(port->port_type == IDE_PORT_TYPE_ENDPOINT);

  char str[MAX_NAME_LENGTH] = {0};

  // open configuration space and get ecap offset
  int fd = open_configuration_space(port->bdf);
  if (fd == -1)
  {
    return false;
  }
  port_context->cfg_space_fd = fd;
  sprintf(str, "dev : %s", port->bdf);
  set_deivce_info(fd, str);

  uint32_t ecap_offset = get_extended_cap_offset(fd, PCI_IDE_EXT_CAPABILITY_ID);
  if (ecap_offset == 0)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "ECAP Offset of IDE is NOT found\n"));
    goto OpenDevFail;
  }
  else
  {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "ECAP Offset of IDE: 0x%016x\n", ecap_offset));
  }
  port_context->ecap_offset = ecap_offset;

  // TODO
  // m_dev_fp indicates the device ide card. It is used in doe_read_write.c.
  // It will be removed later.
  m_dev_fp = fd;

  // initialize pci doe
  if(!init_pci_doe(fd)) {
    goto OpenDevFail;
  }
  port_context->doe_offset = g_doe_extended_offset;

  uint32_t offset = ecap_offset + 4;
  port_context->ide_cap.raw = device_pci_read_32(offset, fd);

  return true;

OpenDevFail:
  close(fd);
  unset_device_info(fd);
  return false;
}

/**
 * Initialize endpoint port
 */
bool init_dev_port(pcie_ide_test_group_context_t *group_context)
{
  TEEIO_ASSERT(group_context != NULL);
  TEEIO_ASSERT(group_context->common.top != NULL);

  IDE_TEST_TOPOLOGY_TYPE top_type = group_context->common.top->type;
  ide_common_test_port_context_t *port_context = &group_context->common.lower_port;
  TEEIO_ASSERT(port_context != NULL);

  if(!open_dev_port(port_context)) {
    return false;
  }

  // ide_id
  uint8_t ide_id = 0;
  if(!find_free_rp_stream_index_and_ide_id(port_context, NULL, &ide_id, top_type, false)) {
    goto InitDevFailed;
  }
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Device ide_id = %d\n", ide_id));
  port_context->ide_id = ide_id;

  // rid&addr assoc_reg_block
  port_context->rid_assoc_reg_block.rid_assoc1.raw = m_rid_assoc_reg_block.rid_assoc1.raw;
  port_context->rid_assoc_reg_block.rid_assoc2.raw = m_rid_assoc_reg_block.rid_assoc2.raw;
  port_context->addr_assoc_reg_block.addr_assoc1.raw = m_addr_assoc_reg_block.addr_assoc1.raw;
  port_context->addr_assoc_reg_block.addr_assoc2.raw = m_addr_assoc_reg_block.addr_assoc2.raw;
  port_context->addr_assoc_reg_block.addr_assoc3.raw = m_addr_assoc_reg_block.addr_assoc3.raw;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "dump dev assoc_reg_block:\n"));
  dump_rid_assoc_reg_block(&port_context->rid_assoc_reg_block);
  dump_addr_assoc_reg_block(&port_context->addr_assoc_reg_block);

  return true;

InitDevFailed:
  close(port_context->cfg_space_fd);
  unset_device_info(port_context->cfg_space_fd);
  return false;
}

uint32_t get_ide_reg_block_offset(int fd, TEST_IDE_TYPE ide_type, uint8_t ide_id, uint32_t ide_ecap_offset)
{
    uint32_t ide_reg_block_offset = 0;
    PCIE_IDE_CAP ide_cap = {.raw = 0};
    ide_cap.raw = device_pci_read_32(ide_ecap_offset + sizeof(PCIE_CAP_ID), fd);

    uint8_t num_lnk_ide = ide_cap.lnk_ide_supported? ide_cap.num_lnk_ide + 1 : 0;

    if(ide_type == TEST_IDE_TYPE_LNK_IDE) {
        TEEIO_ASSERT(ide_id < num_lnk_ide);
        TEEIO_ASSERT(ide_cap.lnk_ide_supported);
        ide_reg_block_offset = ide_ecap_offset
                                + sizeof(PCIE_CAP_ID)
                                + sizeof(PCIE_IDE_CAP)
                                + sizeof(PCIE_IDE_CTRL);
        return ide_reg_block_offset;
    }

    // Now selective ide link
    TEEIO_ASSERT(ide_cap.sel_ide_supported);
    TEEIO_ASSERT(ide_id >= num_lnk_ide && ide_id <= ide_cap.num_sel_ide + num_lnk_ide);

    // skip IDE Control Reg + (Link IDE Stream Control Reg + Link IDE Stream Status Reg)*num_link_ide
    ide_reg_block_offset = ide_ecap_offset
                            + sizeof(PCIE_CAP_ID)
                            + sizeof(PCIE_IDE_CAP)
                            + sizeof(PCIE_IDE_CTRL)
                            + sizeof(PCIE_LNK_IDE_STREAM_REG_BLOCK) * num_lnk_ide;


    PCIE_SEL_IDE_STREAM_CAP ide_stream_cap = {.raw = 0};

    for (uint8_t i = num_lnk_ide; i < ide_id; i++) {
        // Calculate size of Assoc ADDR blocks
        ide_stream_cap.raw = device_pci_read_32(ide_reg_block_offset, fd);
        // It seems num_addr_assoc_reg_blocks shall be 1
        TEEIO_ASSERT(ide_stream_cap.num_addr_assoc_reg_blocks == 1);

        uint64_t size_of_assoc_block = sizeof(PCIE_SEL_IDE_ADDR_ASSOC_REG_BLOCK) *
                                        ide_stream_cap.num_addr_assoc_reg_blocks;

        // Advance to next sel_ide_reg_block
        ide_reg_block_offset += sizeof(PCIE_SEL_IDE_STREAM_REG_BLOCK);
        ide_reg_block_offset += size_of_assoc_block;
    }

    return ide_reg_block_offset;
}

bool set_pcrc_in_ecap(
    int fd, TEST_IDE_TYPE ide_type,
    uint8_t ide_id, uint32_t ide_ecap_offset,
    bool enable
)
{
    uint32_t offset = get_ide_reg_block_offset(fd, ide_type, ide_id, ide_ecap_offset);

    // For sel_ide, ide_ctrl is preceded by ide_cap.
    if(ide_type == TEST_IDE_TYPE_SEL_IDE) {
        offset += 4;
    }
    PCIE_SEL_IDE_STREAM_CTRL ide_stream_ctrl = {.raw = 0};
    ide_stream_ctrl.raw = device_pci_read_32(offset, fd);

    ide_stream_ctrl.pcrc_en = enable ? 1 : 0;
    device_pci_write_32(offset, ide_stream_ctrl.raw, fd);

    return true;
}

/**
 * Set Selective IDE for Configuration Req (bit9) in ide_stream ctrl
*/
bool set_sel_ide_for_cfg_req_in_ecap(
    int fd,
    TEST_IDE_TYPE ide_type,
    uint8_t ide_id,
    uint32_t ide_ecap_offset,
    bool enable
)
{
    uint32_t offset = get_ide_reg_block_offset(fd, ide_type, ide_id, ide_ecap_offset);

    // For sel_ide, ide_ctrl is preceded by ide_cap.
    if(ide_type == TEST_IDE_TYPE_SEL_IDE) {
        offset += 4;
    }
    PCIE_SEL_IDE_STREAM_CTRL ide_stream_ctrl = {.raw = 0};
    ide_stream_ctrl.raw = device_pci_read_32(offset, fd);

    ide_stream_ctrl.cfg_sel_ide = enable ? 1 : 0;
    device_pci_write_32(offset, ide_stream_ctrl.raw, fd);

    return true;
}

/**
 * read ide_stream_ctrl register in ecap
*/
uint32_t read_ide_stream_ctrl_in_ecap(
    int fd,
    TEST_IDE_TYPE ide_type,
    uint8_t ide_id,
    uint32_t ide_ecap_offset
)
{
    uint32_t offset = get_ide_reg_block_offset(fd, ide_type, ide_id, ide_ecap_offset);

    // For sel_ide, ide_stream_ctrl is preceded by ide_cap.
    if(ide_type == TEST_IDE_TYPE_SEL_IDE) {
        offset += 4;
    }
    uint32_t ide_stream_ctrl = device_pci_read_32(offset, fd);

    return ide_stream_ctrl;
}


// setup the ide ecap regs
// IDE Extended Capability is defined in [PCI-SIG IDE] Sec 7.9.99
bool setup_ide_ecap_regs (
    int fd, TEST_IDE_TYPE ide_type,
    uint8_t ide_id, uint32_t ide_ecap_offset,
    PCIE_SEL_IDE_STREAM_CTRL stream_ctrl_reg,
    PCIE_SEL_IDE_RID_ASSOC_1 rid_assoc_1,
    PCIE_SEL_IDE_RID_ASSOC_2 rid_assoc_2,
    PCIE_SEL_IDE_ADDR_ASSOC_1 addr_assoc_1,
    PCIE_SEL_IDE_ADDR_ASSOC_2 addr_assoc_2,
    PCIE_SEL_IDE_ADDR_ASSOC_3 addr_assoc_3)

{
    uint32_t offset = get_ide_reg_block_offset(fd, ide_type, ide_id, ide_ecap_offset);

    // For sel_ide, ide_ctrl is preceded by ide_cap.
    if(ide_type == TEST_IDE_TYPE_SEL_IDE) {
        offset += 4;
    }

    uint32_t ctrl_offset = offset;

    if(ide_type == TEST_IDE_TYPE_SEL_IDE) {
        // rid_assoc and addr_assoc are valid in sel_ide
        offset += 2 * 4;
        device_pci_write_32(offset, rid_assoc_1.raw, fd);
        offset += 4;
        device_pci_write_32(offset, rid_assoc_2.raw, fd);

        offset += 4;
        device_pci_write_32(offset, addr_assoc_1.raw, fd);
        offset += 4;
        device_pci_write_32(offset, addr_assoc_2.raw, fd);
        offset += 4;
        device_pci_write_32(offset, addr_assoc_3.raw, fd);
    }

    device_pci_write_32(ctrl_offset, stream_ctrl_reg.raw, fd);

    return true;
}

bool reset_ide_registers(
  ide_common_test_port_context_t *port_context,
  IDE_TEST_TOPOLOGY_TYPE top_type,
  uint8_t stream_id,
  uint8_t rp_stream_index,
  bool reset_kcbar)
{
  TEST_IDE_TYPE ide_type = TEST_IDE_TYPE_SEL_IDE;
  if(top_type == IDE_TEST_TOPOLOGY_TYPE_SEL_IDE) {
    ide_type = TEST_IDE_TYPE_SEL_IDE;
  } else if(top_type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE) {
    ide_type = TEST_IDE_TYPE_LNK_IDE;
  } else {
    NOT_IMPLEMENTED("selective_and_link_ide topology");
  }

  // initialize kcbar registers
  if (reset_kcbar && !initialize_kcbar_registers(
          (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)port_context->mapped_kcbar_addr,
          stream_id,
          rp_stream_index))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to initialize kcbar_registers\n"));
    return false;
  }

  // setup ide_ecap_regs
  PCIE_SEL_IDE_STREAM_CTRL stream_ctrl_reg = {.raw = 0};
  stream_ctrl_reg.stream_id = stream_id;

  return setup_ide_ecap_regs(
      port_context->cfg_space_fd,
      ide_type,
      port_context->ide_id,
      port_context->ecap_offset,
      stream_ctrl_reg,
      port_context->rid_assoc_reg_block.rid_assoc1,
      port_context->rid_assoc_reg_block.rid_assoc2,
      port_context->addr_assoc_reg_block.addr_assoc1,
      port_context->addr_assoc_reg_block.addr_assoc2,
      port_context->addr_assoc_reg_block.addr_assoc3);
}

void prime_rp_ide_key_set(
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
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Before prime_rp_ide_key_set direction=%s ks=%s: %s=0x%08x\n",
      direct_names[direction],
      ks_names[key_set_select],
      ctrl_reg_names[direction],
      stream_txrx_control.raw));

    if (key_set_select == PCIE_IDE_STREAM_KS0)
    {
        stream_txrx_control.common.prime_key_set_0 = 1;
        stream_txrx_control.common.prime_key_set_1 = 0;
    }
    else if (key_set_select == PCIE_IDE_STREAM_KS1)
    {
        stream_txrx_control.common.prime_key_set_0 = 0;
        stream_txrx_control.common.prime_key_set_1 = 1;
    }
    mmio_write_reg32(ctrl_reg_ptr, stream_txrx_control.raw);

    stream_txrx_control.raw = mmio_read_reg32(ctrl_reg_ptr);
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "After prime_rp_ide_key_set direction=%s ks=%s: %s=0x%08x\n",
      direct_names[direction],
      ks_names[key_set_select],
      ctrl_reg_names[direction],
      stream_txrx_control.raw));

    // check if ready_key_set_x is 1 after prime
    uint32_t data32 = mmio_read_reg32(status_reg_ptr);
    INTEL_KEYP_STREAM_TXRX_STATUS txrx_status = {.raw = data32};
    if (key_set_select == PCIE_IDE_STREAM_KS0) {
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "ready_key_set_0 = %d\n", txrx_status.common.ready_key_set_0));
    } else if (key_set_select == PCIE_IDE_STREAM_KS1) {
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "ready_key_set_1 = %d\n", txrx_status.common.ready_key_set_1));
    }
}

void set_rp_ide_key_set_select(
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

bool enable_ide_stream_in_ecap(int cfg_space_fd, uint32_t ecap_offset, TEST_IDE_TYPE ide_type, uint8_t ide_id, bool enable){
    uint32_t data;
    uint32_t offset = get_ide_reg_block_offset(cfg_space_fd, ide_type, ide_id, ecap_offset);
    if(ide_type == TEST_IDE_TYPE_SEL_IDE) {
        offset += 4;
    }

    data = device_pci_read_32(offset, cfg_space_fd);
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "IDE Stream Control register: 0x%x\n", data));

    if(enable) {
      data |= IDE_STREAM_CTRL_ENABLE;
    } else {
      data &= (~IDE_STREAM_CTRL_ENABLE);
    }

    device_pci_write_32(offset, data, cfg_space_fd);

    return true;
}

// enable root port ide stream
void enable_rootport_ide_stream(int cfg_space_fd, uint32_t ecap_offset, TEST_IDE_TYPE ide_type, uint8_t ide_id, uint8_t *kcbar_addr, uint8_t rp_stream_index, bool enable)
{
    enable_ide_stream_in_kcbar((INTEL_KEYP_ROOT_COMPLEX_KCBAR *)kcbar_addr, rp_stream_index, enable);

    enable_ide_stream_in_ecap(cfg_space_fd, ecap_offset, ide_type, ide_id, enable);
}

uint32_t read_stream_status_in_rp_ecap(int cfg_space_fd, uint32_t ecap_offset, TEST_IDE_TYPE ide_type, uint8_t ide_id)
{
    uint32_t data;
    uint32_t offset = get_ide_reg_block_offset(cfg_space_fd, ide_type, ide_id, ecap_offset);

    if(ide_type == TEST_IDE_TYPE_SEL_IDE) {
        // skip capability register and control register
        offset += 8;
    } else if(ide_type == TEST_IDE_TYPE_LNK_IDE) {
      // skip control register
      offset += 4;
    } else {
      TEEIO_ASSERT(false);
    }

    data = device_pci_read_32(offset, cfg_space_fd);
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "IDE Stream Status register: 0x%x\n", data));

    return data;
}

bool is_ide_enabled(int cfg_space_fd, TEST_IDE_TYPE ide_type, uint8_t ide_id, uint32_t ecap_offset)
{
  uint32_t offset = get_ide_reg_block_offset(cfg_space_fd, ide_type, ide_id, ecap_offset);
  if(ide_type == TEST_IDE_TYPE_SEL_IDE) {
      offset += 4;
  }

  uint32_t stream_ctrl_reg = device_pci_read_32(offset, cfg_space_fd);
  return (stream_ctrl_reg & (uint32_t)IDE_STREAM_CTRL_ENABLE) == IDE_STREAM_CTRL_ENABLE;
}

void dump_ecap(
    int fd,
    uint8_t ide_id,
    uint32_t ide_ecap_offset,
    TEST_IDE_TYPE ide_type
)
{
    uint32_t offset = ide_ecap_offset;
    TEEIO_PRINT(("IDE Extended Cap:\n"));

    // refer to PCIE_IDE_ECAP
    PCIE_CAP_ID cap_id = {.raw = device_pci_read_32(offset, fd)};
    TEEIO_PRINT(("    cap_id        : %08x\n", cap_id.raw));

    offset += 4;
    PCIE_IDE_CAP ide_cap = {.raw = device_pci_read_32(offset, fd)};
    TEEIO_PRINT(("    ide_cap       : %08x\n", ide_cap.raw));
    TEEIO_PRINT(("                  : lnk_ide=%x, sel_ide=%x, ft=%x, aggr=%x, pcrc=%x, alog=%x, sel_ide_cfg_req=%x\n",
                                                            ide_cap.lnk_ide_supported, ide_cap.sel_ide_supported,
                                                            ide_cap.ft_supported, ide_cap.aggr_supported,
                                                            ide_cap.pcrc_supported, ide_cap.supported_algo,
                                                            ide_cap.sel_ide_cfg_req_supported));
    TEEIO_PRINT(("                  : num_lnk_ide=%x, num_sel_ide=%x\n",
                                                            ide_cap.num_lnk_ide, ide_cap.num_sel_ide));

    offset += 4;
    PCIE_IDE_CTRL ide_ctrl = {.raw = device_pci_read_32(offset, fd)};
    TEEIO_PRINT(("    ide_ctrl      : %08x (ft_supported=%x)\n", ide_ctrl.raw, ide_ctrl.ft_supported));

    offset = get_ide_reg_block_offset(fd, ide_type, ide_id, ide_ecap_offset);

    if(ide_type == TEST_IDE_TYPE_SEL_IDE) {
        PCIE_SEL_IDE_STREAM_CAP stream_cap = {.raw = device_pci_read_32(offset, fd)};
        TEEIO_PRINT(("    stream_cap    : %08x (num_addr_assoc_reg_blocks=%d)\n", stream_cap.raw, stream_cap.num_addr_assoc_reg_blocks));
        offset += 4;
    }

    PCIE_SEL_IDE_STREAM_CTRL stream_ctrl = {.raw = device_pci_read_32(offset, fd)};
    TEEIO_PRINT(("    stream_ctrl   : %08x\n", stream_ctrl.raw));
    TEEIO_PRINT(("                  : enabled=%x, pcrc_en=%x, cfg_sel_ide=%x, stream_id=%x\n",
                                                          stream_ctrl.enabled, stream_ctrl.pcrc_en, stream_ctrl.cfg_sel_ide, stream_ctrl.stream_id));

    offset += 4;
    PCIE_SEL_IDE_STREAM_STATUS stream_status = {.raw = device_pci_read_32(offset, fd)};
    TEEIO_PRINT(("    stream_status : %08x (state=%x, recv_intg_check_fail_msg=%x)\n",
                                                          stream_status.raw, stream_status.state, stream_status.recv_intg_check_fail_msg));

    if(ide_type == TEST_IDE_TYPE_SEL_IDE) {
        offset += 4;
        PCIE_SEL_IDE_RID_ASSOC_1 rid_assoc1 = {.raw = device_pci_read_32(offset, fd)};
        TEEIO_PRINT(("    rid_assoc1    : %08x\n", rid_assoc1.raw));

        offset += 4;
        PCIE_SEL_IDE_RID_ASSOC_2 rid_assoc2 = {.raw = device_pci_read_32(offset, fd)};
        TEEIO_PRINT(("    rid_assoc2    : %08x\n", rid_assoc2.raw));

        offset += 4;
        PCIE_SEL_IDE_ADDR_ASSOC_1 addr_assoc1 = {.raw = device_pci_read_32(offset, fd)};
        TEEIO_PRINT(("    addr_assoc1   : %08x\n", addr_assoc1.raw));

        offset += 4;
        PCIE_SEL_IDE_ADDR_ASSOC_2 addr_assoc2 = {.raw = device_pci_read_32(offset, fd)};
        TEEIO_PRINT(("    addr_assoc2   : %08x\n", addr_assoc2.raw));

        offset += 4;
        PCIE_SEL_IDE_ADDR_ASSOC_3 addr_assoc3 = {.raw = device_pci_read_32(offset, fd)};
        TEEIO_PRINT(("    addr_assoc3   : %08x\n", addr_assoc3.raw));
    }
}

void dump_rootport_registers(uint8_t *kcbar_addr, uint8_t rp_stream_index, int cfg_space_fd, uint8_t ide_id, uint32_t ecap_offset, TEST_IDE_TYPE ide_type)
{
    dump_kcbar((INTEL_KEYP_ROOT_COMPLEX_KCBAR *)kcbar_addr, rp_stream_index);
    dump_ecap(cfg_space_fd, ide_id, ecap_offset, ide_type);
}

void dump_dev_registers(int cfg_space_fd, uint8_t ide_id, uint32_t ecap_offset, TEST_IDE_TYPE ide_type)
{
    dump_ecap(cfg_space_fd, ide_id, ecap_offset, ide_type);
}