/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
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
#include "pcie_ide_lib.h"
#include "cxl_ide_lib.h"
#include "cxl_ide_internal.h"
#include "library/cxl_ide_km_common_lib.h"

extern uint32_t g_doe_extended_offset;
extern uint32_t g_ide_extended_offset;
extern uint32_t g_aer_extended_offset;
extern int m_dev_fp;

bool init_pci_doe(int fd);

uint32_t m_pcie_bar_offset[] = {
  PCIE_BAR0_OFFSET,
  PCIE_BAR1_OFFSET,
  PCIE_BAR2_OFFSET,
  PCIE_BAR3_OFFSET,
  PCIE_BAR4_OFFSET,
  PCIE_BAR5_OFFSET
};

// CXL Spec 3.1 Table 8-22
const char* m_cxl_capability_names[] = {
  "CXL NULL Capability",
  "CXL Capability",
  "CXL RAS Capability",
  "CXL Security Capability",
  "CXL Link Capability",
  "CXL HDM Decoder Capability",
  "CXL Extended Security Capability",
  "CXL IDE Capability",
  "CXL Snoop Filter Capability",
  "CXL Timeout and Isolation Capability",
  "CXL.cachemem Extended Register Capability",
  "CXL BI Route Table Capability",
  "CXL BI Decoder Capability",
  "CXL Cache ID Route Table Capability",
  "CXL Cache ID Decoder Capability",
  "CXL Extended HDM Decoder Capability",
  "CXL Extended Metadata Capability"
};

bool cxl_reset_ecap_registers(ide_common_test_port_context_t *port_context)
{
  return true;
}

bool cxl_reset_kcbar_registers(ide_common_test_port_context_t *port_context)
{
  INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR* kcbar_ptr = (INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)port_context->mapped_kcbar_addr;
  INTEL_KEYP_CXL_LINK_ENC_CONTROL enc_ctrl = {.raw = mmio_read_reg32(&kcbar_ptr->link_enc_control)};

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_reset_kcbar_registers before link_enc_ctrl=0x%08x\n", enc_ctrl.raw));

  enc_ctrl.rxkey_valid = 0;
  enc_ctrl.txkey_valid = 0;
  enc_ctrl.start_trigger = 0;
  mmio_write_reg32(&kcbar_ptr->link_enc_control, enc_ctrl.raw);

  enc_ctrl.raw = mmio_read_reg32(&kcbar_ptr->link_enc_control);
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_reset_kcbar_registers after  link_enc_ctrl=0x%08x\n", enc_ctrl.raw));

  return true;
}

void cxl_clear_rootport_key_ivs(ide_common_test_port_context_t* port_context)
{
  TEEIO_ASSERT(port_context);
  TEEIO_ASSERT(port_context->mapped_kcbar_addr);

  // clear the keys & ivs in rootport
  cxl_ide_km_aes_256_gcm_key_buffer_t keys;
  memset(&keys, 0, sizeof(cxl_ide_km_aes_256_gcm_key_buffer_t));
  cxl_cfg_rp_link_enc_key_iv((INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)port_context->mapped_kcbar_addr, CXL_IDE_KM_KEY_DIRECTION_TX, 0, (uint8_t *)keys.key, sizeof(keys.key), (uint8_t *)keys.iv, sizeof(keys.iv));
  cxl_cfg_rp_link_enc_key_iv((INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)port_context->mapped_kcbar_addr, CXL_IDE_KM_KEY_DIRECTION_RX, 0, (uint8_t *)keys.key, sizeof(keys.key), (uint8_t *)keys.iv, sizeof(keys.iv));
}

/**
 * Initialize rootcomplex port
 * root_port and upper_port is the same port
 */
bool cxl_init_root_port(cxl_ide_test_group_context_t *group_context)
{
  TEEIO_ASSERT(group_context != NULL);
  TEEIO_ASSERT(group_context->common.top != NULL);

  ide_common_test_port_context_t *port_context = &group_context->common.upper_port;
  TEEIO_ASSERT(port_context != NULL);
  TEEIO_ASSERT(port_context->port != NULL);
  TEEIO_ASSERT(port_context->port->port_type == IDE_PORT_TYPE_ROOTPORT);
  TEEIO_ASSERT(group_context->common.upper_port.port->id == group_context->common.root_port.port->id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_init_root_port start.\n"));

  if(!cxl_open_root_port(port_context)) {
    return false;
  }

  cxl_reset_kcbar_registers(port_context);

  // clear the keys & ivs in rootport
  cxl_clear_rootport_key_ivs(port_context);

  return true;
}

// walk thru configuration space to find out all CXL DVSEC
bool cxl_find_dvsec_in_config_space(int fd, IDE_TEST_CXL_PCIE_DVSEC* dvsec, int* count)
{
  uint32_t walker = PCIE_EXT_CAP_START;
  uint32_t cap_ext_header = 0;
  int dvsec_cnt = 0;
  CXL_DVSEC_COMMON_HEADER header = {0};

  TEEIO_ASSERT(dvsec != NULL);
  TEEIO_ASSERT(count != NULL);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_find_dvsec_in_config_space\n"));

  while (walker < PCIE_CONFIG_SPACE_SIZE && walker != 0)
  {
    cap_ext_header = device_pci_read_32(walker, fd);

    if (((PCIE_CAP_ID *)&cap_ext_header)->id == PCI_DVSCE_EXT_CAPABILITY_ID)
    {
      header.ide_ecap.raw = device_pci_read_32(walker, fd);
      header.header1.raw = device_pci_read_32(walker + 4, fd);
      header.header2.raw = device_pci_read_16(walker + 8, fd);

      if(header.header1.vendor_id == DVSEC_VENDOR_ID_CXL) {
        // This is CXL DVSEC block
        TEEIO_ASSERT(dvsec_cnt < *count);
        dvsec->offset = walker;
        dvsec->dvsec_id = header.header2.id;
        dvsec += 1;
        dvsec_cnt += 1;
      }
    }

    walker = ((PCIE_CAP_ID *)&cap_ext_header)->next_cap_offset;
  }

  *count = dvsec_cnt;

  return true;
}

static CXL_DVSEC_ID mandatory_rootport_dvsec_id[] = {
  CXL_DVSEC_ID_CXL_EXTENSIONS_DVSEC_FOR_PORTS,
  CXL_DVSEC_ID_GPF_DVSEC_FOR_CXL_PORTS,
  CXL_DVSEC_ID_PCIE_DVSEC_FOR_FLEX_BUS_PORT,
  CXL_DVSEC_ID_REGISTER_LOCATOR_DVSEC,
  CXL_DVSEC_IN_INVALID
};

static CXL_DVSEC_ID mandatory_endpoint_dvsec_id[] = {
  CXL_DVSEC_ID_PCIE_DVSEC_FOR_CXL_DEVICES,
  CXL_DVSEC_ID_GPF_DVSEC_FOR_CXL_DEVICES,
  CXL_DVSEC_ID_PCIE_DVSEC_FOR_FLEX_BUS_PORT,
  CXL_DVSEC_ID_REGISTER_LOCATOR_DVSEC,
  CXL_DVSEC_IN_INVALID
};

// check if CXL DVSECs in rootport are valid
bool cxl_check_rootport_dvsecs(IDE_TEST_CXL_PCIE_DVSEC* dvsec, int count)
{
  bool valid = false;
  int i = 0;

  while(mandatory_rootport_dvsec_id[i] != CXL_DVSEC_IN_INVALID) {
    valid = false;

    for(int j = 0; j < count; j++) {
      if(mandatory_rootport_dvsec_id[i] == dvsec[j].dvsec_id) {
        valid = true;
        break;
      }
    }
    if(!valid) {
      break;
    }
    i++;
  }

  return valid;
}

// check if CXL DVSECs in endpoint are valid
bool cxl_check_ep_dvsec(IDE_TEST_CXL_PCIE_DVSEC* dvsec, int count)
{
  bool valid = false;
  int i = 0;

  while(mandatory_endpoint_dvsec_id[i] != CXL_DVSEC_IN_INVALID) {
    valid = false;

    for(int j = 0; j < count; j++) {
      if(mandatory_endpoint_dvsec_id[i] == dvsec[j].dvsec_id) {
        valid = true;
        break;
      }
    }
    if(!valid) {
      break;
    }
    i++;
  }

  return valid;
}

void cxl_dump_dvsecs(IDE_TEST_CXL_PCIE_DVSEC* dvsec, int count)
{
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Dump CXL DVSECs\n"));
  for(int i = 0; i < count; i++) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  DVSEC %04x, offset = 0x%04x\n", dvsec[i].dvsec_id, dvsec[i].offset));
  }
}

/*
 * Open rootcomplex port
 */
bool cxl_open_root_port(ide_common_test_port_context_t *port_context)
{
  char str[MAX_NAME_LENGTH] = {0};

  IDE_PORT *port = port_context->port;
  TEEIO_ASSERT(port->port_type == IDE_PORT_TYPE_ROOTPORT);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_open_root_port %s.\n", port->bdf));

  // open configuration space and get ecap offset
  int fd = open_configuration_space(port->bdf);
  if (fd == -1) {
    return false;
  }

  port_context->cfg_space_fd = fd;
  sprintf(str, "cxl.host : %s", port->bdf);
  set_deivce_info(fd, str);

  CXL_PRIV_DATA *cxl_data = &port_context->cxl_data;

  int dvsec_cnt = MAX_IDE_TEST_DVSEC_COUNT;
  if(!cxl_find_dvsec_in_config_space(fd, cxl_data->ecap.dvsecs, &dvsec_cnt)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Find CXL DVSECs failed.\n"));
    goto InitRootPortFail;
  }
  cxl_data->ecap.dvsec_cnt = dvsec_cnt;

  cxl_dump_dvsecs(cxl_data->ecap.dvsecs, dvsec_cnt);

  // check CXL DVSECs
  if(!cxl_check_rootport_dvsecs(cxl_data->ecap.dvsecs, dvsec_cnt)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Check CXL DVSECs failed.\n"));
    goto InitRootPortFail;
  }

  // map cxl.memcache reg block
  if(!cxl_init_memcache_reg_block(fd, &cxl_data->memcache, cxl_data->ecap.dvsecs, cxl_data->ecap.dvsec_cnt)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Map CXL.memcache reg block failed.\n"));
    goto InitRootPortFail;
  }
  // dump CXL IDE Capability in memcache reg block
  cxl_dump_ide_capability(cxl_data->memcache.cap_headers, cxl_data->memcache.cap_headers_cnt, cxl_data->memcache.mapped_memcache_reg_block);

  port_context->ecap_offset = 0;

  // parse KEYP table and map the kcbar to user space
  if (!parse_keyp_table(port_context, INTEL_KEYP_PROTOCOL_TYPE_CXL_MEMCACHE))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "parse_keyp_table failed.\n"));
    goto InitRootPortFail;
  }

  // store the Link_Enc_Global_Config in kcbar and cxl_cap/cxl_cap2/cxl_cap3 in ecap(@configuration space)
  INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar = (INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)port_context->mapped_kcbar_addr;
  cxl_data->kcbar.link_enc_global_config.raw = mmio_read_reg32(&kcbar->link_enc_global_config);

  return true;

InitRootPortFail:
  close(fd);
  unset_device_info(fd);
  return false;
}

uint8_t* cxl_map_bar_addr(int cfg_space_fd, uint32_t bar, uint64_t offset_in_bar, int* mapped_fd)
{
  // first read BAR value
  uint32_t bar_val = device_pci_read_32(bar, cfg_space_fd);
  bool is_64_bit = (bar_val & PCIE_MEM_BASE_ADDR_MASK) == PCIE_MEM_BASE_ADDR_64;
  size_t map_size = CXL_CACHEMEM_REG_BLOCK_SIZE;
  off_t target = 0;

  int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
  if(mem_fd == -1) {
      TEEIO_DEBUG ((TEEIO_DEBUG_ERROR, "Failed to open /dev/mem\n"));
      return NULL;
  }

  uint8_t* mem_ptr = NULL;

  if(is_64_bit) {
    uint32_t bar_high = device_pci_read_32(bar + 4, cfg_space_fd);
    uint64_t val64 = ((uint64_t)bar_high<<32) | bar_val;
    target = val64 & ~(map_size - 1);
  } else {
    target = bar_val & ~(map_size - 1);
  }

  mem_ptr = (uint8_t *)mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, target + (uint32_t)offset_in_bar + CXL_IO_REG_BLOCK_SIZE);
  if (mem_ptr == MAP_FAILED) {
      TEEIO_DEBUG ((TEEIO_DEBUG_ERROR, "Failed to mmap CXL.cachemem component reg block\n"));
      close(mem_fd);
      mem_ptr = NULL;
      mem_fd = 0;
  }

  *mapped_fd = mem_fd;
  return mem_ptr;
}

void cxl_unmap_memcache_reg_block(int mapped_fd, uint8_t* mapped_addr)
{
  if(mapped_fd > 0 && mapped_addr != NULL) {
    munmap(mapped_addr, CXL_CACHEMEM_REG_BLOCK_SIZE);
    close(mapped_fd);
  }
}

void cxl_dump_cap_headers(CXL_CAPABILITY_HEADER cap_header, CXL_CAPABILITY_XXX_HEADER* cap_xxx_header, int cap_headers_cnt)
{
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Dump memcache register block\n"));

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  Capability header=0x%08x\n", cap_header.raw));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    cap_id=0x%04x, cap_version=0x%02x, cache_mem_version=0x%02x, array_size=0x%04x\n",
                                cap_header.cap_id, cap_header.cap_version, cap_header.cache_mem_version, cap_header.array_size));

  for(int i = 0; i < cap_headers_cnt; i++) {
    TEEIO_ASSERT(cap_xxx_header[i].cap_id < CXL_CAPABILITY_ID_NUM);
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  %s header=0x%08x\n", m_cxl_capability_names[cap_xxx_header[i].cap_id], cap_xxx_header[i].raw));
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    cap_id=0x%04x, cap_version=0x%02x, pointer=0x%04x\n",
                                  cap_xxx_header[i].cap_id, cap_xxx_header[i].cap_version, cap_xxx_header[i].pointer));
  }
}

bool cxl_populate_memcache_reg_block(CXL_PRIV_DATA_MEMCACHE_REG_DATA* memcache_reg)
{
  uint8_t* ptr = memcache_reg->mapped_memcache_reg_block;
  int cap_headers_cnt = CXL_CAPABILITY_ID_NUM;

  TEEIO_ASSERT(ptr != NULL);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Walk thru memcache register block. ptr=%08x\n", ptr));

  CXL_CAPABILITY_HEADER cap_header = {.raw = mmio_read_reg32(ptr)};

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cap_header=%08x\n", cap_header.raw));

  // CXL Spec 3.1 Sectiono 8.2.4.1
  TEEIO_ASSERT(cap_header.cap_id == 1);
  TEEIO_ASSERT(cap_header.cap_version == 1);
  TEEIO_ASSERT(cap_header.cache_mem_version == 1);

  if(cap_headers_cnt < cap_header.array_size) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cap_xxx_header buffer too small. (%d<%d)\n", cap_headers_cnt, cap_header.array_size));
    return false;
  }
  
  ptr += 4;
  for(int i = 0; i < cap_header.array_size; i++) {
    memcache_reg->cap_headers[i].raw = mmio_read_reg32(ptr + i * 4);
  }

  memcache_reg->cap_headers_cnt = cap_header.array_size;
  cap_headers_cnt = cap_header.array_size;
  cxl_dump_cap_headers(cap_header, memcache_reg->cap_headers, cap_header.array_size);

  //
  // walk thru to find CXL IDE Capability
  int i = 0;
  for(; i < cap_headers_cnt; i++) {
    if(memcache_reg->cap_headers[i].cap_id == CXL_CAPABILITY_ID_IDE_CAP) {
      break;
    }
  }

  if(i == cap_headers_cnt) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Cannot find CXL IDE Capability!\n"));
    return false;
  }

  // cache the pointer to CXL_IDE_CAPABILITY_STRUCT
  memcache_reg->cxl_ide_capability_struct_ptr = memcache_reg->mapped_memcache_reg_block + memcache_reg->cap_headers[i].pointer;
  ptr = memcache_reg->cxl_ide_capability_struct_ptr + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, cap);
  memcache_reg->ide_cap.raw = mmio_read_reg32(ptr);

  return true; 
}

bool cxl_init_memcache_reg_block(int cfg_space_fd, CXL_PRIV_DATA_MEMCACHE_REG_DATA* memcache_regs, IDE_TEST_CXL_PCIE_DVSEC* dvsec, int count)
{
  int i;
  for(i = 0; i < count; i++) {
    if(dvsec[i].dvsec_id == CXL_DVSEC_ID_REGISTER_LOCATOR_DVSEC) {
      break;
    }
  }

  if(i == count) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Cannot find REGISTER LOCATOR DVSEC!\n"));
    return NULL;
  }

  dvsec += i;
  TEEIO_ASSERT(dvsec->offset != 0);
  CXL_DVSEC_COMMON_HEADER header = {0};
  int offset = dvsec->offset;
  offset += 4;  // skip ecap_id
  header.header1.raw = device_pci_read_32(offset, cfg_space_fd);
  offset += 4;
  header.header2.raw = device_pci_read_16(offset, cfg_space_fd);
  offset += 4;  // header2 + rsvd

  TEEIO_ASSERT(header.header1.vendor_id == DVSEC_VENDOR_ID_CXL);
  TEEIO_ASSERT(header.header2.id == CXL_DVSEC_ID_REGISTER_LOCATOR_DVSEC);

  int length = header.header1.length - sizeof(CXL_DVSEC_REGISTER_LOCATOR);
  TEEIO_ASSERT(length % 8 == 0);
  int reg_block_cnt = length/8;

  CXL_REGISTER_BLOCK reg_block = {0};
  uint8_t* mapped_memcache_reg_block = NULL;
  uint64_t offset_in_bar = 0;
  int mapped_fd = 0;

  for(i = 0; i < reg_block_cnt; i++) {
    offset += i*8;
    reg_block.low.raw = device_pci_read_32(offset, cfg_space_fd);
    reg_block.high.register_block_offset_high = device_pci_read_32(offset + 4, cfg_space_fd);

    if(reg_block.low.register_block_id != CXL_DVSEC_REG_BLOCK_ID_COMPONENT_REG) {
      continue;
    }
    TEEIO_ASSERT(reg_block.low.register_bir < sizeof(m_pcie_bar_offset)/sizeof(uint32_t));

    offset_in_bar = ((uint64_t)reg_block.high.register_block_offset_high << 32) | ((uint32_t)reg_block.low.register_block_offset_low<<16);
    mapped_memcache_reg_block = cxl_map_bar_addr(cfg_space_fd, m_pcie_bar_offset[reg_block.low.register_bir], offset_in_bar, &mapped_fd);
    break;
  }

  if(mapped_memcache_reg_block == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to map cxl.memcache reg block.\n"));
    return false;
  }

  memcache_regs->mapped_fd = mapped_fd;
  memcache_regs->mapped_memcache_reg_block = mapped_memcache_reg_block;
  cxl_populate_memcache_reg_block(memcache_regs);

  return true;;
}

/*
 * Close rootcomplex port
 */
bool cxl_close_root_port(cxl_ide_test_group_context_t *group_context)
{
  // clean Control Registers @ecap and KCBar corresponding registers
  ide_common_test_port_context_t* port_context = &group_context->common.upper_port;
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "close cxl rootport %s(%s)\n", port_context->port->port_name, port_context->port->bdf));

  cxl_reset_ecap_registers(port_context);
  cxl_reset_kcbar_registers(port_context);

  CXL_PRIV_DATA* cxl_data = &port_context->cxl_data;

  cxl_unmap_memcache_reg_block(cxl_data->memcache.mapped_fd, cxl_data->memcache.mapped_memcache_reg_block);

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
  
  memset(&port_context->cxl_data, 0, sizeof(CXL_PRIV_DATA));

  return true;
}

/*
 * Initialize device port
 */
bool cxl_init_dev_port(cxl_ide_test_group_context_t *group_context)
{
  TEEIO_ASSERT(group_context != NULL);
  TEEIO_ASSERT(group_context->common.top != NULL);

  TEEIO_ASSERT(group_context->common.suite_context->test_category == TEEIO_TEST_CATEGORY_CXL_IDE);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_init_dev_port start.\n"));

  ide_common_test_port_context_t *port_context = &group_context->common.lower_port;
  TEEIO_ASSERT(port_context != NULL);

  if(!cxl_open_dev_port(port_context)) {
    return false;
  }

  return true;
}

void cxl_dump_ide_capability(CXL_CAPABILITY_XXX_HEADER* cap_header, int cap_headers_cnt, uint8_t* mapped_memcache_reg_block)
{
  // walk thru to find CXL IDE Capability
  int i = 0;
  for(; i < cap_headers_cnt; i++) {
    if(cap_header[i].cap_id == CXL_CAPABILITY_ID_IDE_CAP) {
      break;
    }
  }

  if(i == cap_headers_cnt) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Cannot find CXL IDE Capability!\n"));
    return;
  }

  uint8_t* ptr = mapped_memcache_reg_block + cap_header[i].pointer + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, cap);
  CXL_IDE_CAPABILITY ide_cap = {.raw = mmio_read_reg32(ptr)};

  ptr = mapped_memcache_reg_block + cap_header[i].pointer + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, control);
  CXL_IDE_CONTROL ide_control = {.raw = mmio_read_reg32(ptr)};

  ptr = mapped_memcache_reg_block + cap_header[i].pointer + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, status);
  CXL_IDE_STATUS ide_status = {.raw = mmio_read_reg32(ptr)};

  ptr = mapped_memcache_reg_block + cap_header[i].pointer + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, key_refresh_time_capability);
  CXL_KEY_REFRESH_TIME_CAPABILITY key_refresh_time_cap = {.raw = mmio_read_reg32(ptr)};

  ptr = mapped_memcache_reg_block + cap_header[i].pointer + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, truncation_transmit_delay_capability);
  CXL_TRUNCATION_TRANSMIT_DELAY_CAPABILITY truncation_transmit_delay_cap = {.raw = mmio_read_reg32(ptr)};

  ptr = mapped_memcache_reg_block + cap_header[i].pointer + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, key_refresh_time_control);
  CXL_KEY_REFRESH_TIME_CONTROL key_refresh_time_ctrl = {.raw = mmio_read_reg32(ptr)};

  ptr = mapped_memcache_reg_block + cap_header[i].pointer + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, truncation_transmit_delay_control);
  CXL_TRUNCATION_TRANSMIT_DELAY_CONTROL truncation_transmit_delay_ctrl = {.raw = mmio_read_reg32(ptr)};

  // CXL_IDE_ERROR_STATUS error_status;
  ptr = mapped_memcache_reg_block + cap_header[i].pointer + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, error_status);
  CXL_IDE_ERROR_STATUS error_status = {.raw = mmio_read_reg32(ptr)};

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Dump CXL IDE Capability\n"));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  ide_cap = 0x%08x\n", ide_cap.raw));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    cxl_ide_capable=0x%x, cxl_ide_modes=0x%02x\n",
                                  ide_cap.cxl_ide_capable, ide_cap.supported_cxl_ide_modes));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    supported_algo=0x%02x, ide_stop_capable=%d, lopt_ide_capable=%d\n",
                                  ide_cap.supported_algo, ide_cap.ide_stop_capable,
                                  ide_cap.lopt_ide_capable));

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  ide_control = 0x%08x\n", ide_control.raw));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    pcrc_disable=%d, ide_stop_enable=%d\n",
                                  ide_control.pcrc_disable, ide_control.ide_stop_enable));

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  key_refresh_time_cap.rx_min_key_refresh_time = 0x%08x\n", key_refresh_time_cap.rx_min_key_refresh_time));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  key_refresh_time_ctrl.tx_key_refresh_time    = 0x%08x\n", key_refresh_time_ctrl.tx_key_refresh_time));

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  truncation_transmit_delay_cap.rx_min_truncation_transmit_delay = 0x%08x\n", truncation_transmit_delay_cap.rx_min_truncation_transmit_delay));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  truncation_transmit_delay_ctrl.tx_truncation_transmit_delay    = 0x%08x\n", truncation_transmit_delay_ctrl.tx_truncation_transmit_delay));

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  ide_status = 0x%08x\n", ide_status.raw));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    rx_ide_status=0x%02x, tx_ide_status=0x%02x\n",
                                  ide_status.rx_ide_status, ide_status.tx_ide_status));

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  error_status = 0x%08x\n", error_status.raw));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    rx_error_status=0x%02x, tx_error_status=0x%02x, unexpected_ide_stop_received=0x%02x\n",
                                  error_status.rx_error_status, error_status.tx_error_status, error_status.unexpected_ide_stop_received));

}

void cxl_dump_ide_status(CXL_CAPABILITY_XXX_HEADER* cap_header, int cap_headers_cnt, uint8_t* mapped_memcache_reg_block)
{
  // walk thru to find CXL IDE Capability
  int i = 0;
  for(; i < cap_headers_cnt; i++) {
    if(cap_header[i].cap_id == CXL_CAPABILITY_ID_IDE_CAP) {
      break;
    }
  }

  if(i == cap_headers_cnt) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Cannot find CXL IDE Capability!\n"));
    return;
  }

  uint8_t* ptr = mapped_memcache_reg_block + cap_header[i].pointer + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, status);
  CXL_IDE_STATUS ide_status = {.raw = mmio_read_reg32(ptr)};

  ptr = mapped_memcache_reg_block + cap_header[i].pointer + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, error_status);
  CXL_IDE_ERROR_STATUS error_status = {.raw = mmio_read_reg32(ptr)};

  ptr = mapped_memcache_reg_block + cap_header[i].pointer + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, control);
  CXL_IDE_CONTROL ide_control = {.raw = mmio_read_reg32(ptr)};

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Dump CXL IDE Status\n"));

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  ide_status = 0x%08x off=0x%04x\n", ide_status.raw, OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, status)));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    rx_ide_status=0x%02x, tx_ide_status=0x%02x\n",
                                  ide_status.rx_ide_status, ide_status.tx_ide_status));

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  error_status = 0x%08x off=0x%04x\n", error_status.raw, OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, error_status)));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    rx_error_status=0x%02x, tx_error_status=0x%02x, unexpected_ide_stop_received=0x%02x\n",
                                  error_status.rx_error_status, error_status.tx_error_status, error_status.unexpected_ide_stop_received));

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  ide_control : 0x%08x\n", ide_control.raw));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    pcrc_disable=0x%02x, ide_stop_enable=0x%02x\n",
                                  ide_control.pcrc_disable, ide_control.ide_stop_enable));
}


/*
 * Open device port
 */
bool cxl_open_dev_port(ide_common_test_port_context_t *port_context)
{
  TEEIO_ASSERT(port_context != NULL);
  IDE_PORT *port = port_context->port;
  TEEIO_ASSERT(port != NULL);
  TEEIO_ASSERT(port->port_type == IDE_PORT_TYPE_ENDPOINT);

  char str[MAX_NAME_LENGTH] = {0};

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_open_dev_port %s\n", port->bdf));

  // open configuration space and get ecap offset
  int fd = open_configuration_space(port->bdf);
  if (fd == -1)
  {
    return false;
  }
  port_context->cfg_space_fd = fd;
  sprintf(str, "cxl.dev : %s", port->bdf);
  set_deivce_info(fd, str);

  int dvsec_cnt = MAX_IDE_TEST_DVSEC_COUNT;
  CXL_PRIV_DATA* cxl_data = &port_context->cxl_data;
  if(!cxl_find_dvsec_in_config_space(fd, cxl_data->ecap.dvsecs, &dvsec_cnt)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Find CXL DVSECs failed.\n"));
    goto OpenDevFail;
  }
  cxl_data->ecap.dvsec_cnt = dvsec_cnt;

  cxl_dump_dvsecs(cxl_data->ecap.dvsecs, dvsec_cnt);

  cxl_populate_dev_caps_in_ecap(fd, &cxl_data->ecap);

  // check CXL DVSECs
  if(!cxl_check_ep_dvsec(cxl_data->ecap.dvsecs, dvsec_cnt)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Check CXL DVSECs failed.\n"));
    goto OpenDevFail;
  }

  if(!cxl_init_memcache_reg_block(fd, &cxl_data->memcache, cxl_data->ecap.dvsecs, cxl_data->ecap.dvsec_cnt)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Map CXL.memcache reg block failed.\n"));
    goto OpenDevFail;
  }

  // dump CXL IDE Capability
  cxl_dump_ide_capability(cxl_data->memcache.cap_headers, cxl_data->memcache.cap_headers_cnt, cxl_data->memcache.mapped_memcache_reg_block);

  // TODO
  // m_dev_fp indicates the device ide card. It is used in doe_read_write.c.
  // It will be removed later.
  m_dev_fp = fd;

  // initialize pci doe
  if(!init_pci_doe(fd)) {
    goto OpenDevFail;
  }
  port_context->doe_offset = g_doe_extended_offset;

  return true;

OpenDevFail:
  close(fd);
  unset_device_info(fd);
  return false;
}

/*
 * Close device port
 */
bool cxl_close_dev_port(ide_common_test_port_context_t *port_context, IDE_TEST_TOPOLOGY_TYPE top_type)
{
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "close cxl dev %s(%s)\n", port_context->port->port_name, port_context->port->bdf));

  cxl_reset_ecap_registers(port_context);

  CXL_PRIV_DATA* cxl_data = &port_context->cxl_data;
  cxl_unmap_memcache_reg_block(cxl_data->memcache.mapped_fd, cxl_data->memcache.mapped_memcache_reg_block);

  if(port_context->cfg_space_fd > 0) {
    close(port_context->cfg_space_fd);
    unset_device_info(port_context->cfg_space_fd);
  }
  port_context->cfg_space_fd = 0;

  memset(&port_context->cxl_data, 0, sizeof(CXL_PRIV_DATA));

  m_dev_fp = 0;
  g_doe_extended_offset = 0;
  return true;
}

void cxl_cfg_rp_link_enc_key_iv(
    INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr,
    CXL_IDE_STREAM_DIRECTION direction, // RX TX
    const uint8_t key_slot,             // key 0, 1, 2, 3
    uint8_t* key, uint32_t key_size,    // key
    uint8_t* iv, uint32_t iv_size       // iv vals
    )
{
  INTEL_KEYP_CXL_TXRX_ENC_KEYS_BLOCK* enc_keys = NULL;
  INTEL_KEYP_CXL_TXRX_IV* txrx_iv = NULL;
  TEEIO_ASSERT(key_slot < CXL_LINK_ENC_KEYS_SLOT_NUM);
  TEEIO_ASSERT(key_size == 32);

  if(direction == CXL_IDE_STREAM_DIRECTION_RX) {
    enc_keys = &kcbar_ptr->tx_enc_keys;
    txrx_iv = &kcbar_ptr->tx_iv;
  } else {
    enc_keys = &kcbar_ptr->rx_enc_keys;
    txrx_iv = &kcbar_ptr->rx_iv;
  }

  if(iv_size > sizeof(txrx_iv->iv)) {
    iv_size = sizeof(txrx_iv->iv);
  }

  TEEIO_ASSERT(iv_size == 8);

  reg_memcpy_dw(enc_keys, key_size, key, key_size);
  reg_memcpy_dw(txrx_iv, iv_size, iv, iv_size);
}

void cxl_cfg_cache_enable(int fd, uint32_t ecap_offset, bool enable)
{
}

void cxl_cfg_mem_enable(int fd, uint32_t ecap_offset, bool enable)
{
}

void cxl_cfg_rp_mode(
    INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr,
    INTEL_CXL_IDE_MODE mode
    )
{
  INTEL_KEYP_CXL_LINK_ENC_GLOBAL_CONFIG global_cfg = {.raw = mmio_read_reg32(&kcbar_ptr->link_enc_global_config)};
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_cfg_rp_mode before global_cfg = 0x%08x\n", global_cfg.raw));

  global_cfg.mode = mode;

  mmio_write_reg32(&kcbar_ptr->link_enc_global_config, global_cfg.raw);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_cfg_rp_mode after  global_cfg = 0x%08x\n", global_cfg.raw));
}

void cxl_cfg_rp_txrx_key_valid(
    INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr,
    CXL_IDE_STREAM_DIRECTION direction,
    bool valid
    )
{
  INTEL_KEYP_CXL_LINK_ENC_CONTROL enc_ctrl = {.raw = mmio_read_reg32(&kcbar_ptr->link_enc_control)};
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_cfg_rp_txrx_key_valid (direct=%d) before link_enc_control = 0x%08x\n", direction, enc_ctrl.raw));

  if(direction == CXL_IDE_STREAM_DIRECTION_RX) {
    enc_ctrl.rxkey_valid = valid ? 1 : 0;
  } else {
    enc_ctrl.txkey_valid = valid ? 1 : 0;
  }

  mmio_write_reg32(&kcbar_ptr->link_enc_control, enc_ctrl.raw);

  enc_ctrl.raw = mmio_read_reg32(&kcbar_ptr->link_enc_control);
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_cfg_rp_txrx_key_valid (direct=%d) after  link_enc_control = 0x%08x\n", direction, enc_ctrl.raw));
}

void cxl_cfg_rp_start_trigger(
    INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr,
    bool start
    )
{
  INTEL_KEYP_CXL_LINK_ENC_CONTROL enc_ctrl = {.raw = mmio_read_reg32(&kcbar_ptr->link_enc_control)};
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_cfg_rp_start_trigger before link_enc_control = 0x%08x\n", enc_ctrl.raw));

  enc_ctrl.start_trigger = start ? 1 : 0;

  mmio_write_reg32(&kcbar_ptr->link_enc_control, enc_ctrl.raw);

  enc_ctrl.raw = mmio_read_reg32(&kcbar_ptr->link_enc_control);
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_cfg_rp_start_trigger after  link_enc_control = 0x%08x\n", enc_ctrl.raw));
}

void cxl_cfg_rp_linkenc_enable(
    INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr,
    bool enable
    )
{
  INTEL_KEYP_CXL_LINK_ENC_GLOBAL_CONFIG global_cfg = {.raw = mmio_read_reg32(&kcbar_ptr->link_enc_global_config)};
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_cfg_rp_linkenc_enable before global_cfg = 0x%08x\n", global_cfg.raw));

  global_cfg.link_enc_enable = enable ? 1 : 0;

  mmio_write_reg32(&kcbar_ptr->link_enc_global_config, global_cfg.raw);
  global_cfg.raw = mmio_read_reg32(&kcbar_ptr->link_enc_global_config);
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_cfg_rp_linkenc_enable after  global_cfg = 0x%08x\n", global_cfg.raw));
}


bool cxl_populate_dev_caps_in_ecap(int fd, CXL_PRIV_DATA_ECAP* ecap)
{
  TEEIO_ASSERT(ecap != NULL);
  TEEIO_ASSERT(ecap->dvsec_cnt != 0);
  
  int i;
  for(i = 0; i < ecap->dvsec_cnt; i++) {
    if(ecap->dvsecs[i].dvsec_id == CXL_DVSEC_ID_PCIE_DVSEC_FOR_CXL_DEVICES) {
      break;
    }
  }

  if(i == ecap->dvsec_cnt) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "CXL DVSEC for CXL Devices is not found.\n"));
    return false;
  }

  int offset = ecap->dvsecs[i].offset + OFFSET_OF(CXL_DVSEC_FOR_DEVICE, capability);
  ecap->cap.raw = device_pci_read_16(offset, fd);
  offset = ecap->dvsecs[i].offset + OFFSET_OF(CXL_DVSEC_FOR_DEVICE, capability2);
  ecap->cap2.raw = device_pci_read_16(offset, fd);
  offset = ecap->dvsecs[i].offset + OFFSET_OF(CXL_DVSEC_FOR_DEVICE, capability3);
  ecap->cap3.raw = device_pci_read_16(offset, fd);

  cxl_dump_caps_in_ecap(ecap);

  return true;
}

void cxl_dump_kcbar(INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr)
{
    TEEIO_PRINT(("Dump Key Programming Register Block:\n"));
    INTEL_KEYP_CXL_LINK_ENC_GLOBAL_CONFIG global_config = {.raw = mmio_read_reg32(&kcbar_ptr->link_enc_global_config)};
    TEEIO_PRINT(("global_config : %08x\n", global_config.raw));
    TEEIO_PRINT(("    link_enc_enable=%d, mode=%02x, algorithm=%02x\n",
                      global_config.link_enc_enable, global_config.mode, global_config.algorithm));

    INTEL_KEYP_CXL_LINK_ENC_CONTROL enc_control = {.raw = mmio_read_reg32(&kcbar_ptr->link_enc_control)};
    TEEIO_PRINT(("enc_control   : %08x\n", enc_control.raw));
    TEEIO_PRINT(("    start_trigger=%d, rxkey_valid=%d, txkey_valid=%d\n",
                      enc_control.start_trigger, enc_control.rxkey_valid, enc_control.txkey_valid));

}

void cxl_dump_caps_in_ecap(CXL_PRIV_DATA_ECAP* ecap)
{
  TEEIO_PRINT(("CXL IDE Extended Cap:\n"));

  // CXL_DEV_CAPABILITY
  CXL_DEV_CAPABILITY cap = {.raw = ecap->cap.raw};
  TEEIO_PRINT(("    CXL Capability: %04x\n", cap));
  TEEIO_PRINT(("                  : cache_capable=%d, io_capable=%d, mem_capable=%d\n",
                                    cap.cache_capable, cap.io_capable, cap.mem_capable));
  TEEIO_PRINT(("                  : mem_hwinit_mode=%d, hdm_count=%d, cache_writeback_and_invalidate_capable=%d, cxl_reset_capable=%d\n",
                                    cap.mem_hwinit_mode, cap.hdm_count, cap.cache_writeback_and_invalidate_capable, cap.cxl_reset_capable));
  TEEIO_PRINT(("                  : cxl_reset_timeout=%d, cxl_mem_clr_capable=%d, tsp_capable=%d\n",
                                    cap.cxl_reset_timeout, cap.cxl_mem_clr_capable, cap.tsp_capable));
  TEEIO_PRINT(("                  : multiple_logical_device=%d, viral_capable=%d, pm_init_capable=%d\n",
                                    cap.multiple_logical_device, cap.viral_capable, cap.pm_init_completion_reporting_capable));
}

bool cxl_ide_set_key_refresh_control_reg(ide_common_test_port_context_t* host_port, ide_common_test_port_context_t* dev_port)
{
  uint8_t* host_ptr;
  uint8_t* dev_ptr;

  uint8_t *host_cxl_ide_capability_struct_ptr = host_port->cxl_data.memcache.cxl_ide_capability_struct_ptr;
  uint8_t *dev_cxl_ide_capability_struct_ptr = dev_port->cxl_data.memcache.cxl_ide_capability_struct_ptr;

  if(host_cxl_ide_capability_struct_ptr == NULL || dev_cxl_ide_capability_struct_ptr == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "pointer to cxl_ide_capability_struct_ptr is NULL(host:%llx, dev:%llx)!\n",
                                    (uint64_t)host_cxl_ide_capability_struct_ptr,
                                    (uint64_t)dev_cxl_ide_capability_struct_ptr));
    return false;
  }

  // Check Key Refresh Time Control in Host side
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Check Key Refresh Time Control in Host side\n"));
  dev_ptr = dev_cxl_ide_capability_struct_ptr + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, key_refresh_time_capability);
  host_ptr = host_cxl_ide_capability_struct_ptr + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, key_refresh_time_control);

  CXL_KEY_REFRESH_TIME_CAPABILITY dev_key_refresh_time_cap = {.raw = mmio_read_reg32(dev_ptr)};
  CXL_KEY_REFRESH_TIME_CONTROL host_key_refresh_time_ctrl = {.raw = mmio_read_reg32(host_ptr)};

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Device CXL_KEY_REFRESH_TIME_CAPABILITY.rx_min_key_refresh_time = 0x%x\n", dev_key_refresh_time_cap.rx_min_key_refresh_time));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Host   CXL_KEY_REFRESH_TIME_CONTROL.tx_key_refresh_time        = 0x%x\n", host_key_refresh_time_ctrl.tx_key_refresh_time));

  if(host_key_refresh_time_ctrl.tx_key_refresh_time < dev_key_refresh_time_cap.rx_min_key_refresh_time) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Program Host CXL_KEY_REFRESH_TIME_CONTROL.tx_key_refresh_time as 0x%x\n", dev_key_refresh_time_cap.rx_min_key_refresh_time));
    mmio_write_reg32(host_ptr, dev_key_refresh_time_cap.rx_min_key_refresh_time);
  }

  // Check Key Refresh Time Control in Device side
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Check Key Refresh Time Control in Device side\n"));
  host_ptr = host_cxl_ide_capability_struct_ptr + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, key_refresh_time_capability);
  dev_ptr = dev_cxl_ide_capability_struct_ptr + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, key_refresh_time_control);

  CXL_KEY_REFRESH_TIME_CAPABILITY host_key_refresh_time_cap = {.raw = mmio_read_reg32(host_ptr)};
  CXL_KEY_REFRESH_TIME_CONTROL dev_key_refresh_time_ctrl = {.raw = mmio_read_reg32(dev_ptr)};

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Host   CXL_KEY_REFRESH_TIME_CAPABILITY.rx_min_key_refresh_time = 0x%x\n", host_key_refresh_time_cap.rx_min_key_refresh_time));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Device CXL_KEY_REFRESH_TIME_CONTROL.tx_key_refresh_time        = 0x%x\n", dev_key_refresh_time_ctrl.tx_key_refresh_time));

  if(dev_key_refresh_time_ctrl.tx_key_refresh_time < host_key_refresh_time_cap.rx_min_key_refresh_time) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Program Device CXL_KEY_REFRESH_TIME_CONTROL.tx_key_refresh_time as 0x%x\n", host_key_refresh_time_cap.rx_min_key_refresh_time));
    mmio_write_reg32(dev_ptr, host_key_refresh_time_cap.rx_min_key_refresh_time);
  }

  return true;
}

bool cxl_ide_set_truncation_transmit_control_reg(ide_common_test_port_context_t* host_port, ide_common_test_port_context_t* dev_port)
{
  uint8_t* host_ptr;
  uint8_t* dev_ptr;

  uint8_t *host_cxl_ide_capability_struct_ptr = host_port->cxl_data.memcache.cxl_ide_capability_struct_ptr;
  uint8_t *dev_cxl_ide_capability_struct_ptr = dev_port->cxl_data.memcache.cxl_ide_capability_struct_ptr;

  if(host_cxl_ide_capability_struct_ptr == NULL || dev_cxl_ide_capability_struct_ptr == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "pointer to cxl_ide_capability_struct_ptr is NULL!\n"));
    return false;
  }

  // Truncation Transmit Delay Control in Host side
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Check Truncation Transmit Delay Control in Host side\n"));
  dev_ptr = dev_cxl_ide_capability_struct_ptr + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, truncation_transmit_delay_capability);
  host_ptr = host_cxl_ide_capability_struct_ptr + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, truncation_transmit_delay_control);

  CXL_TRUNCATION_TRANSMIT_DELAY_CAPABILITY dev_truncation_transmit_delay_cap = {.raw = mmio_read_reg32(dev_ptr)};
  CXL_TRUNCATION_TRANSMIT_DELAY_CONTROL host_truncation_transmit_delay_ctrl = {.raw = mmio_read_reg32(host_ptr)};

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Device CXL_TRUNCATION_TRANSMIT_DELAY_CAPABILITY.rx_min_truncation_transmit_delay = 0x%x\n", dev_truncation_transmit_delay_cap.rx_min_truncation_transmit_delay));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Host   CXL_TRUNCATION_TRANSMIT_DELAY_CONTROL.tx_truncation_transmit_delay        = 0x%x\n", host_truncation_transmit_delay_ctrl.tx_truncation_transmit_delay));

  if(host_truncation_transmit_delay_ctrl.tx_truncation_transmit_delay < dev_truncation_transmit_delay_cap.rx_min_truncation_transmit_delay) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Program Host CXL_TRUNCATION_TRANSMIT_DELAY_CONTROL.tx_truncation_transmit_delay as 0x%x\n", dev_truncation_transmit_delay_cap.rx_min_truncation_transmit_delay));
    mmio_write_reg32(host_ptr, dev_truncation_transmit_delay_cap.rx_min_truncation_transmit_delay);
  }

  // Truncation Transmit Delay Control in Device side
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Check Truncation Transmit Delay Control in Device side\n"));
  host_ptr = host_cxl_ide_capability_struct_ptr + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, truncation_transmit_delay_capability);
  dev_ptr = dev_cxl_ide_capability_struct_ptr + OFFSET_OF(CXL_IDE_CAPABILITY_STRUCT, truncation_transmit_delay_control);

  CXL_TRUNCATION_TRANSMIT_DELAY_CAPABILITY host_truncation_transmit_delay_cap = {.raw = mmio_read_reg32(host_ptr)};
  CXL_TRUNCATION_TRANSMIT_DELAY_CONTROL dev_truncation_transmit_delay_ctrl = {.raw = mmio_read_reg32(dev_ptr)};

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Host   CXL_TRUNCATION_TRANSMIT_DELAY_CAPABILITY.rx_min_truncation_transmit_delay = 0x%x\n", host_truncation_transmit_delay_cap.rx_min_truncation_transmit_delay));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Device CXL_TRUNCATION_TRANSMIT_DELAY_CONTROL.tx_truncation_transmit_delay        = 0x%x\n", dev_truncation_transmit_delay_ctrl.tx_truncation_transmit_delay));

  if(dev_truncation_transmit_delay_ctrl.tx_truncation_transmit_delay < host_truncation_transmit_delay_cap.rx_min_truncation_transmit_delay) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Program Device CXL_TRUNCATION_TRANSMIT_DELAY_CONTROL.tx_truncation_transmit_delay as 0x%x\n", host_truncation_transmit_delay_cap.rx_min_truncation_transmit_delay));
    mmio_write_reg32(dev_ptr, host_truncation_transmit_delay_cap.rx_min_truncation_transmit_delay);
  }

  return true;
}

bool cxl_scan_devices(void *test_context)
{
  bool ret = false;
  cxl_ide_test_group_context_t *context = (cxl_ide_test_group_context_t *)test_context;
  TEEIO_ASSERT(context->common.signature == GROUP_CONTEXT_SIGNATURE);

  IDE_TEST_TOPOLOGY *top = context->common.top;

  TEEIO_ASSERT(context->common.suite_context->test_category == TEEIO_TEST_CATEGORY_CXL_IDE);
  TEEIO_ASSERT(top->connection == IDE_TEST_CONNECT_DIRECT || top->connection == IDE_TEST_CONNECT_SWITCH);

  ret = scan_devices_at_bus(context->common.root_port.port, context->common.lower_port.port, context->common.sw_conn1, context->common.top->segment, context->common.top->bus);
  if(ret) {
    context->common.upper_port.port->bus = context->common.root_port.port->bus;
    strncpy(context->common.upper_port.port->bdf, context->common.root_port.port->bdf, BDF_LENGTH);
  }

  return ret;
}
