/**
 *  Copyright Notice:
 *  Copyright 2025 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>

#include "hal/base.h"
#include "hal/library/debuglib.h"

#include "ide_test.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "pcie_ide_lib.h"
#include "pcie_ide_test_lib.h"

// Check if disabling PCIE Flit Mode is supported on the given port context
bool check_flit_mode_disable_supported(ide_common_test_port_context_t *port_context, bool is_upper_port)
{
  if (!pcie_check_flit_mode_enabled(port_context)) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Flit Mode is not enabled.\n"));
    return false;
  }

  // Downstream Port should support dll active reporting for link down/up operation
  if (is_upper_port) {
    int fd = port_context->cfg_space_fd;
    PCIE_LINK_CAP upper_link_cap;

    uint32_t pcie_cap_offset = get_cap_offset(fd, PCIE_CAPABILITY_ID);
    uint32_t offset;

    offset = pcie_cap_offset + 0x0C; // Link Cap Offset
    upper_link_cap.raw = device_pci_read_32(offset, fd);
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Upper Port Link Cap: 0x%08x\n", upper_link_cap.raw));
    if (upper_link_cap.dll_active_reporting) {
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Upper Port DLL Active Reporting Supported\n"));
      return true;
    } else {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Upper Port DLL Active Reporting Not Supported\n"));
      return false;
    }
  }

  return true;
}

bool test_config_flit_mode_disable_support_common(void *test_context)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  bool result = true;

  IDE_TEST_TOPOLOGY *top = group_context->common.top;
  if(top->connection == IDE_TEST_CONNECT_DIRECT || top->connection == IDE_TEST_CONNECT_SWITCH) {
    result = check_flit_mode_disable_supported(&group_context->common.upper_port, true);
    if (!result) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Flit mode not enabled on upper port.\n"));
      return false;
    }

    result = check_flit_mode_disable_supported(&group_context->common.lower_port, false);
    if (!result) {
      TEEIO_DEBUG((TEEIO_DEBUG_WARN, "Flit mode not enabled on lower port.\n"));
      TEEIO_DEBUG((TEEIO_DEBUG_WARN, "Some device does not implement pcie link status2 register properly\n"));
      TEEIO_DEBUG((TEEIO_DEBUG_WARN, "Using upper port status as exam indicator\n"));
    }
  } else if (top->connection == IDE_TEST_CONNECT_P2P ){
    NOT_IMPLEMENTED("Open both root_port and upper_port for peer2peer connection.");
  } else {
    return false; // unsupported connection type
  }

  return true;
}

bool set_link_disable (ide_common_test_port_context_t *port_context, bool is_disable)
{
  int fd = port_context->cfg_space_fd;
  uint32_t pcie_cap_offset = get_cap_offset(fd, PCIE_CAPABILITY_ID);
  TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "PCIE Capability Offset: 0x%02x\n", pcie_cap_offset));
  uint32_t offset;

  offset = pcie_cap_offset + 0x10;
  PCIE_LINK_CTRL pcie_link_ctrl;
  pcie_link_ctrl.raw = device_pci_read_16(offset, fd);

  // set link disable
  pcie_link_ctrl.link_disable = is_disable; // Set Link Disable
  device_pci_write_16(offset, pcie_link_ctrl.raw, fd);
  libspdm_sleep(10 * 1000);; // Wait for 10ms for the change to take effect
  pcie_link_ctrl.raw = device_pci_read_16(offset, fd);
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "PCIE Link Control after set/clear Link Disable: 0x%04x\n", pcie_link_ctrl.raw));
  if (pcie_link_ctrl.link_disable != is_disable) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to set/clear Link Disable.\n"));
    return false;
  }

  // check dll_active_reporting
  offset = pcie_cap_offset + 0x12;
  PCIE_LINK_STATUS pcie_link_status;
  pcie_link_status.raw = device_pci_read_16(offset, fd);
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "PCIE Link Status: 0x%04x\n", pcie_link_status.raw));
  uint32_t max_wait_loop = 10;
  while (pcie_link_status.data_link_layer_active == is_disable && max_wait_loop > 0) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Waiting for DLL Active to be changed...\n"));
    libspdm_sleep(10 * 1000);; // Wait for 10ms
    pcie_link_status.raw = device_pci_read_16(offset, fd);
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "PCIE Link Status: 0x%04x\n", pcie_link_status.raw));
    max_wait_loop--;
  }

  if (pcie_link_status.data_link_layer_active == is_disable) {
    return false;
  }
  return true;
}


bool set_flit_mode_disable (ide_common_test_port_context_t *port_context, bool is_disable)
{
  int fd = port_context->cfg_space_fd;
  uint32_t pcie_cap_offset = get_cap_offset(fd, PCIE_CAPABILITY_ID);
  TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "PCIE Capability Offset: 0x%02x\n", pcie_cap_offset));
  uint32_t offset;
  bool result;

  // set flit mode disable
  offset = pcie_cap_offset + 0x10;
  PCIE_LINK_CTRL pcie_link_ctrl;
  pcie_link_ctrl.raw = device_pci_read_16(offset, fd);
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "PCIE Link Control: 0x%04x\n", pcie_link_ctrl.raw));

  pcie_link_ctrl.flit_mode_disable = is_disable;
  device_pci_write_16(offset, pcie_link_ctrl.raw, fd);
  libspdm_sleep(10 * 1000);; // Wait for 10ms for the change to take effect
  pcie_link_ctrl.raw = device_pci_read_16(offset, fd);
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "PCIE Link Control after %s Flit Mode Disable: 0x%04x\n", is_disable ? "set" : "clear", pcie_link_ctrl.raw));
  if (pcie_link_ctrl.flit_mode_disable != is_disable) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to %s Flit Mode Disable.\n", is_disable ? "set" : "clear"));
    return false;
  }

  result = set_link_disable(port_context, true);
  if (!result) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to disable link.\n"));
    return result;
  }

  result = set_link_disable(port_context, false);
  if (!result) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to enable link.\n"));
    return result;
  }

  // check flit_mode_status
  if (pcie_check_flit_mode_enabled(port_context) != is_disable) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Flit Mode is %s.\n", is_disable ? "disabled" : "enabled"));
    return true;
  } else {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to %s Flit Mode.\n", is_disable ? "disable" : "enable"));
    return false;
  }
}

bool test_config_flit_mode_disable_common (void *test_context, bool is_enable)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  bool result;
  result = set_flit_mode_disable(&group_context->common.upper_port, is_enable);
  if (!result) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to %s Flit Mode on upper port.\n", is_enable ? "enable" : "disable"));
    return false;
  }

  result = reset_ide_registers(&group_context->common.lower_port,
                               group_context->common.top->type,
                               group_context->stream_id,
                               group_context->rp_stream_index,
                               false);
  if (!result) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to reset IDE registers on lower port.\n"));
    return false;
  }

  return true;
}

//  selective_ide flit mode disable test config
bool pcie_ide_test_config_flit_mode_disable_enable_sel(void *test_context)
{
  return test_config_flit_mode_disable_common(test_context, true);
}

bool pcie_ide_test_config_flit_mode_disable_disable_sel(void *test_context)
{
  return test_config_flit_mode_disable_common(test_context, false);
}

bool pcie_ide_test_config_flit_mode_disable_support_sel(void *test_context)
{
  return test_config_flit_mode_disable_support_common(test_context);
}

bool pcie_ide_test_config_flit_mode_disable_check_sel(void *test_context)
{
  return true;
}

//  link_ide flit mode disable test config
bool pcie_ide_test_config_flit_mode_disable_enable_link(void *test_context)
{
  return test_config_flit_mode_disable_common(test_context, true);
}

bool pcie_ide_test_config_flit_mode_disable_disable_link(void *test_context)
{
  return test_config_flit_mode_disable_common(test_context, false);
}


bool pcie_ide_test_config_flit_mode_disable_support_link(void *test_context)
{
  return test_config_flit_mode_disable_support_common(test_context);
}

bool pcie_ide_test_config_flit_mode_disable_check_link(void *test_context)
{
  return true;
}

// selective_and_link_ide flit mode disable test config
bool pcie_ide_test_config_flit_mode_disable_enable_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return false;
}

bool pcie_ide_test_config_flit_mode_disable_disable_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return false;
}

bool pcie_ide_test_config_flit_mode_disable_support_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return false;
}

bool pcie_ide_test_config_flit_mode_disable_check_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return false;
}
