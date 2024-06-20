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

extern uint32_t g_doe_extended_offset;
extern uint32_t g_ide_extended_offset;
extern uint32_t g_aer_extended_offset;
extern int m_dev_fp;

/**
 * Initialize rootcomplex port
 * root_port and upper_port is the same port
 */
bool cxl_init_root_port(ide_common_test_group_context_t *group_context)
{
  TEEIO_ASSERT(group_context != NULL);
  TEEIO_ASSERT(group_context->top != NULL);
  TEEIO_ASSERT(group_context->top->type == IDE_TEST_TOPOLOGY_TYPE_SEL_IDE);

  ide_common_test_port_context_t *port_context = &group_context->upper_port;
  TEEIO_ASSERT(port_context != NULL);
  TEEIO_ASSERT(port_context->port != NULL);
  TEEIO_ASSERT(port_context->port->port_type == IDE_PORT_TYPE_ROOTPORT);
  TEEIO_ASSERT(group_context->upper_port.port->id == group_context->root_port.port->id);

  if(!cxl_open_root_port(port_context)) {
    return false;
  }

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

  // open configuration space and get ecap offset
  int fd = open_configuration_space(port->bdf);
  if (fd == -1) {
    return false;
  }

  port_context->cfg_space_fd = fd;
  sprintf(str, "cxl.host : %s", port->bdf);
  set_deivce_info(fd, str);

  int dvsec_cnt = MAX_IDE_TEST_DVSEC_COUNT;
  if(!cxl_find_dvsec_in_config_space(fd, port_context->priv_data.cxl.desecs, &dvsec_cnt)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Find CXL DVSECs failed.\n"));
    goto InitRootPortFail;
  }

  cxl_dump_dvsecs(port_context->priv_data.cxl.desecs, dvsec_cnt);

  // check CXL DVSECs
  if(!cxl_check_rootport_dvsecs(port_context->priv_data.cxl.desecs, dvsec_cnt)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Check CXL DVSECs failed.\n"));
    goto InitRootPortFail;
  }


  // uint32_t ecap_offset = get_extended_cap_offset(fd, PCI_IDE_EXT_CAPABILITY_ID);
  // if (ecap_offset == 0)
  // {
  //   TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "ECAP Offset of CXL IDE is NOT found\n"));
  //   goto InitRootPortFail;
  // }
  // else
  // {
  //   TEEIO_DEBUG((TEEIO_DEBUG_INFO, "ECAP Offset of CXL IDE: 0x%016x\n", ecap_offset));
  // }

  port_context->ecap_offset = 0;

  // parse KEYP table and map the kcbar to user space
  if (!parse_keyp_table(port_context, INTEL_KEYP_PROTOCOL_TYPE_CXL_MEMCACHE))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "parse_keyp_table failed.\n"));
    goto InitRootPortFail;
  }

  // store the Link_Enc_Global_Config in kcbar and cxl_cap/cxl_cap2/cxl_cap3 in ecap(@configuration space)
  INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar = (INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)port_context->mapped_kcbar_addr;
  CXL_PRIV_DATA * cxl_data = &port_context->priv_data.cxl;
  cxl_data->link_enc_global_config.raw = mmio_read_reg32(&kcbar->link_enc_global_config);

  // // check CXL_DVS_HEADER1 CXL_DVS_HEADER2
  // CXL_DVS_HEADER1 header1 = {.raw = device_pci_read_32(ecap_offset + CXL_DVS_HEADER1_OFFSET, fd)};
  // CXL_DVS_HEADER2 header2 = {.raw = device_pci_read_16(ecap_offset + CXL_DVS_HEADER2_OFFSET, fd)};
  // TEEIO_DEBUG((TEEIO_DEBUG_INFO, "CXL Rootport (%s) header1=0x%08x, header2=0x%04x\n", port->bdf, header1.raw, header2.raw));

  // // read and save cap/cap2/cap3
  // cxl_data->cap.raw = device_pci_read_16(ecap_offset + CXL_CAPABILITY_OFFSET, fd);
  // cxl_data->cap2.raw = device_pci_read_16(ecap_offset + CXL_CAPABILITY2_OFFSET, fd);
  // cxl_data->cap3.raw = device_pci_read_16(ecap_offset + CXL_CAPABILITY3_OFFSET, fd);
  // TEEIO_DEBUG((TEEIO_DEBUG_INFO, "CXL Rootport (%s) cap=0x%04x, cap2=0x%04x, cap3=0x%04x\n", port->bdf, cxl_data->cap.raw, cxl_data->cap2.raw, cxl_data->cap3.raw));

  return true;

InitRootPortFail:
  close(fd);
  unset_device_info(fd);
  return false;
}

bool cxl_reset_ecap_registers(ide_common_test_port_context_t *port_context)
{
  return true;
}

bool cxl_reset_kcbar_registers(ide_common_test_port_context_t *port_context)
{
  return true;
}

/*
 * Close rootcomplex port
 */
bool cxl_close_root_port(ide_common_test_group_context_t *group_context)
{
  // clean Control Registers @ecap and KCBar corresponding registers
  ide_common_test_port_context_t* port_context = &group_context->upper_port;
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "close cxl rootport %s(%s)\n", port_context->port->port_name, port_context->port->bdf));

  cxl_reset_ecap_registers(port_context);
  cxl_reset_kcbar_registers(port_context);

  CXL_PRIV_DATA* cxl_data = &port_context->priv_data.cxl;
  cxl_data->cap.raw = 0;
  cxl_data->cap2.raw = 0;
  cxl_data->cap3.raw = 0;
  cxl_data->link_enc_global_config.raw = 0;

  if(group_context->upper_port.kcbar_fd > 0) {
    unmap_kcbar_addr(group_context->upper_port.kcbar_fd, group_context->upper_port.mapped_kcbar_addr);
  }
  group_context->upper_port.kcbar_fd = 0;
  group_context->upper_port.mapped_kcbar_addr = 0;

  if(group_context->upper_port.cfg_space_fd > 0) {
    close(group_context->upper_port.cfg_space_fd);
    unset_device_info(group_context->upper_port.cfg_space_fd);
  }
  group_context->upper_port.cfg_space_fd = 0;
  
  return true;
}

/*
 * Initialize device port
 */
bool cxl_init_dev_port(ide_common_test_group_context_t *group_context)
{
  TEEIO_ASSERT(group_context != NULL);
  TEEIO_ASSERT(group_context->top != NULL);

  IDE_TEST_TOPOLOGY *top = group_context->top;
  TEEIO_ASSERT(top->type == IDE_TEST_TOPOLOGY_TYPE_SEL_IDE);
  TEEIO_ASSERT(top->ide_type == IDE_HW_TYPE_CXL_MEM);

  // IDE_TEST_TOPOLOGY_TYPE top_type = top->type;

  ide_common_test_port_context_t *port_context = &group_context->lower_port;
  TEEIO_ASSERT(port_context != NULL);

  if(!cxl_open_dev_port(port_context)) {
    return false;
  }

  // prepare range registers?

  return true;

// InitDevFailed:
//   close(port_context->cfg_space_fd);
//   unset_device_info(port_context->cfg_space_fd);
//   return false;
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
  if(!cxl_find_dvsec_in_config_space(fd, &port_context->priv_data.cxl.desecs[0], &dvsec_cnt)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Find CXL DVSECs failed.\n"));
    goto OpenDevFail;
  }

  cxl_dump_dvsecs(port_context->priv_data.cxl.desecs, dvsec_cnt);

  // check CXL DVSECs
  if(!cxl_check_ep_dvsec(port_context->priv_data.cxl.desecs, dvsec_cnt)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Check CXL DVSECs failed.\n"));
    goto OpenDevFail;
  }

  // uint32_t ecap_offset = get_extended_cap_offset(fd, PCI_IDE_EXT_CAPABILITY_ID);
  // if (ecap_offset == 0)
  // {
  //   TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "ECAP Offset of CXL IDE is NOT found\n"));
  //   goto OpenDevFail;
  // }
  // else
  // {
  //   TEEIO_DEBUG((TEEIO_DEBUG_INFO, "ECAP Offset of CXL IDE: 0x%016x\n", ecap_offset));
  // }
  // port_context->ecap_offset = ecap_offset;

  // TODO
  // m_dev_fp indicates the device ide card. It is used in doe_read_write.c.
  // It will be removed later.
  m_dev_fp = fd;

  // initialize pci doe
  if(!init_pci_doe(fd)) {
    goto OpenDevFail;
  }
  port_context->doe_offset = g_doe_extended_offset;

  // TODO
  // // check CXL_DVS_HEADER1 CXL_DVS_HEADER2
  // CXL_DVS_HEADER1 header1 = {.raw = device_pci_read_32(ecap_offset + CXL_DVS_HEADER1_OFFSET, fd)};
  // CXL_DVS_HEADER2 header2 = {.raw = device_pci_read_16(ecap_offset + CXL_DVS_HEADER2_OFFSET, fd)};
  // TEEIO_DEBUG((TEEIO_DEBUG_INFO, "CXL Dev (%s) header1=0x%08x, header2=0x%04x\n", port->bdf, header1.raw, header2.raw));

  // // read and save cap/cap2/cap3
  // CXL_PRIV_DATA* cxl_data = &port_context->priv_data.cxl;

  // cxl_data->cap.raw = device_pci_read_16(ecap_offset + CXL_CAPABILITY_OFFSET, fd);
  // cxl_data->cap2.raw = device_pci_read_16(ecap_offset + CXL_CAPABILITY2_OFFSET, fd);
  // cxl_data->cap3.raw = device_pci_read_16(ecap_offset + CXL_CAPABILITY3_OFFSET, fd);
  // TEEIO_DEBUG((TEEIO_DEBUG_INFO, "CXL Dev (%s) cap=0x%04x, cap2=0x%04x, cap3=0x%04x\n", port->bdf, cxl_data->cap.raw, cxl_data->cap2.raw, cxl_data->cap3.raw));

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

  CXL_PRIV_DATA* cxl_data = &port_context->priv_data.cxl;
  cxl_data->cap.raw = 0;
  cxl_data->cap2.raw = 0;
  cxl_data->cap3.raw = 0;
  cxl_data->link_enc_global_config.raw = 0;

  if(port_context->cfg_space_fd > 0) {
    close(port_context->cfg_space_fd);
    unset_device_info(port_context->cfg_space_fd);
  }
  port_context->cfg_space_fd = 0;

  m_dev_fp = 0;
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
  // TODO iv_size == 8?
  // in cxl_ide_common_lib.h, 
  // uint32_t iv[3]
  TEEIO_ASSERT(iv_size == 8);

  if(direction == CXL_IDE_STREAM_DIRECTION_RX) {
    enc_keys = &kcbar_ptr->tx_enc_keys;
    txrx_iv = &kcbar_ptr->tx_iv;
  } else {
    enc_keys = &kcbar_ptr->rx_enc_keys;
    txrx_iv = &kcbar_ptr->tx_iv;
  }

  reg_memcpy_dw(enc_keys, key_size, key, key_size);
  reg_memcpy_dw(txrx_iv, iv_size, iv, iv_size);
}

void cxl_cfg_cache_enable(int fd, uint32_t ecap_offset, bool enable)
{
  CXL_CONTROL ctrl = {.raw = device_pci_read_16(ecap_offset + CXL_CONTROL_OFFSET, fd)};
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_cfg_cache_enable(%d) cxl_control.raw=0x%04x\n", enable, ctrl.raw));
  ctrl.cache_enable = enable ? 1 : 0;
  device_pci_write_16(ecap_offset + CXL_CONTROL_OFFSET, ctrl.raw, fd);
}

void cxl_cfg_mem_enable(int fd, uint32_t ecap_offset, bool enable)
{
  CXL_CONTROL ctrl = {.raw = device_pci_read_16(ecap_offset + CXL_CONTROL_OFFSET, fd)};
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cxl_cfg_mem_enable(%d) cxl_control.raw=0x%04x\n", enable, ctrl.raw));
  ctrl.mem_enable = enable ? 1 : 0;
  device_pci_write_16(ecap_offset + CXL_CONTROL_OFFSET, ctrl.raw, fd);
}

void cxl_cfg_rp_txrx_key_valid(
    INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr,
    bool valid
    )
{
  INTEL_KEYP_CXL_LINK_ENC_CONTROL enc_ctrl = {.raw = mmio_read_reg32(&kcbar_ptr->link_enc_control)};
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "link_enc_control = 0x%08x\n", enc_ctrl.raw));

  enc_ctrl.rxkey_valid = valid ? 1 : 0;
  enc_ctrl.txkey_valid = valid ? 1 : 0;

  mmio_write_reg32(&kcbar_ptr->link_enc_control, enc_ctrl.raw);
}

void cxl_cfg_rp_start_trigger(
    INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr,
    bool start
    )
{
  INTEL_KEYP_CXL_LINK_ENC_CONTROL enc_ctrl = {.raw = mmio_read_reg32(&kcbar_ptr->link_enc_control)};
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "link_enc_control = 0x%08x\n", enc_ctrl.raw));

  enc_ctrl.start_trigger = start ? 1 : 0;

  mmio_write_reg32(&kcbar_ptr->link_enc_control, enc_ctrl.raw); 
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

void cxl_dump_ecap(int fd, uint32_t ecap_offset)
{
  uint32_t offset = ecap_offset;
  TEEIO_PRINT(("CXL IDE Extended Cap:\n"));

  // refer to PCIE_IDE_ECAP
  PCIE_CAP_ID cap_id = {.raw = device_pci_read_32(offset, fd)};
  TEEIO_PRINT(("    cap_id        : %08x\n", cap_id.raw));

  // DVS_HEADER1
  offset += 4;
  CXL_DVS_HEADER1 header1 = {.raw = device_pci_read_32(offset, fd)};
  TEEIO_PRINT(("    dvs header1   : %08x\n", header1.raw));
  TEEIO_PRINT(("                  : vendor_id=%04x, revision=%02x, length=%04x\n",
                                    header1.vendor_id, header1.revision, header1.length));

  // DVS_HEADER2
  offset += 4;
  CXL_DVS_HEADER2 header2 = {.raw = device_pci_read_16(offset, fd)};
  TEEIO_PRINT(("    dvs header2   : %04x\n", header2.raw));
  TEEIO_PRINT(("                  : id=%04x\n",
                                    header2.id));

  // CXL_CAPABILITY
  offset += 2;
  CXL_CAPABILITY cap = {.raw = device_pci_read_16(offset, fd)};
  TEEIO_PRINT(("    CXL Capability: %04x\n", cap));
  TEEIO_PRINT(("                  : cache_capable=%d, io_capable=%d, mem_capable=%d\n",
                                    cap.cache_capable, cap.io_capable, cap.mem_capable));
  TEEIO_PRINT(("                  : mem_hwinit_mode=%d, hdm_count=%d, cache_writeback_and_invalidate_capable=%d, cxl_reset_capable=%d\n",
                                    cap.mem_hwinit_mode, cap.hdm_count, cap.cache_writeback_and_invalidate_capable, cap.cxl_reset_capable));
  TEEIO_PRINT(("                  : cxl_reset_timeout=%d, cxl_mem_clr_capable=%d, tsp_capable=%d\n",
                                    cap.cxl_reset_timeout, cap.cxl_mem_clr_capable, cap.tsp_capable));
  TEEIO_PRINT(("                  : multiple_logical_device=%d, viral_capable=%d, pm_init_capable=%d\n",
                                    cap.multiple_logical_device, cap.viral_capable, cap.pm_init_completion_reporting_capable));

  // CXL Control
  offset += 2;
  CXL_CONTROL control = {.raw = device_pci_read_16(offset, fd)};
  TEEIO_PRINT(("    CXL Control   : %04x\n", control));
  TEEIO_PRINT(("                  : cache_enable=%d, io_enable=%d, mem_enable=%d\n",
                                    control.cache_enable, control.io_enable, control.mem_enable));
  TEEIO_PRINT(("                  : cache_sf_coverage=%d, cache_sf_granularity=%d, cache_clean_eviction=%d\n",
                                    control.cache_sf_coverage, control.cache_sf_granularity, control.cache_clean_eviction));
  TEEIO_PRINT(("                  : direct_p2p_mem_enable=%d, viral_enable=%d\n",
                                    control.direct_p2p_mem_enable, control.viral_enable));

  // CXL Status
  offset += 2;
  CXL_STATUS status = {.raw = device_pci_read_16(offset, fd)};
  TEEIO_PRINT(("    CXL Status    : %04x\n", status));
  TEEIO_PRINT(("                  : viral_status=%d\n",
                                    status.viral_status));

  // CXL Control2
  offset += 2;

  // CXL Status2
  offset += 2;

  // CXL Lock
  offset += 2;

  // CXL Capability2
  offset += 2;

  // Range1
  offset += 2;

  // Range2
  offset += 4 * 4;

  // CXL Capability3
  offset += 4 * 4;

}