/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#include "hal/base.h"
#include "hal/library/debuglib.h"

#include "ide_test.h"
#include "pcie.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "pcie_ide_lib.h"
#include "pcie_ide_test_lib.h"

extern const char *IDE_TEST_IDE_TYPE_NAMES[];

const char *IDE_STREAM_STATUS_NAME[] = {
    "Secure",
    "Insecure",
    "Unkown"};

typedef enum
{
  IDE_STREAM_STATUS_TYPE_SECURE = 0,
  IDE_STREAM_STATUS_TYPE_INSECURE,
  IDE_STREAM_STATUS_TYPE_UNKNOWN,
  IDE_STREAM_STATUS_TYPE_NUM
} IDE_STREAM_STATUS_TYPE;

// check if the ide_stream is secure or insecure
bool test_config_check_common(void *test_context, const char* assertion_msg)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = (pcie_ide_test_group_context_t *)config_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  ide_common_test_port_context_t *port = &group_context->common.upper_port;
  TEST_IDE_TYPE ide_type = TEST_IDE_TYPE_SEL_IDE;
  if (group_context->common.top->type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE)
  {
    ide_type = TEST_IDE_TYPE_LNK_IDE;
  }
  else if(group_context->common.top->type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE)
  {
    NOT_IMPLEMENTED("selective_and_link_ide topology");
  }

  uint32_t data = read_stream_status_in_rp_ecap(port->cfg_space_fd, port->ecap_offset, ide_type, port->ide_id);
  PCIE_SEL_IDE_STREAM_STATUS stream_status = {.raw = data};
  uint8_t state = stream_status.state;
  IDE_STREAM_STATUS_TYPE status = IDE_STREAM_STATUS_TYPE_UNKNOWN;
  if (state == IDE_STREAM_STATUS_SECURE)
  {
    status = IDE_STREAM_STATUS_TYPE_SECURE;
  }
  else if (state == IDE_STREAM_STATUS_INSECURE)
  {
    status = IDE_STREAM_STATUS_TYPE_INSECURE;
  }

  const char *ide_type_name = IDE_TEST_IDE_TYPE_NAMES[ide_type];

  TEEIO_DEBUG((
      TEEIO_DEBUG_INFO,
      "%s: %s status register - BIT_3:0 (%s state) - %s(%d)\n",
      assertion_msg,
      ide_type_name, ide_type_name,
      IDE_STREAM_STATUS_NAME[status],
      state));

  bool res = status == IDE_STREAM_STATUS_TYPE_SECURE;
  teeio_record_config_item_result(IDE_TEST_CONFIGURATION_TYPE_DEFAULT, TEEIO_TEST_CONFIG_FUNC_CHECK, res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED); 
  return res;
}

// set ft_supported
bool set_ft_supported_in_switch(char* port_bdf)
{
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "set_ft_supported_in_switch bdf=%s\n", port_bdf));

    if(port_bdf == NULL) {
        return false;
    }

    bool ret = false;
    int fd = open_configuration_space(port_bdf);
    if(fd == -1) {
        return false;
    }

    char dev_info[MAX_LINE_LENGTH] = {0};
    sprintf(dev_info, "switch-port: %s", port_bdf);
    set_deivce_info(fd, dev_info);

    uint32_t ecap_offset = get_extended_cap_offset(fd, 0x30);

    if (ecap_offset == 0) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "ECAP Offset of %s is NOT found\n", port_bdf));
        goto SetFailed;
    }

    uint32_t offset = ecap_offset + 8;
    PCIE_IDE_CTRL ide_ctrl = {.raw = device_pci_read_32(offset, fd)};
    if(ide_ctrl.ft_supported == 0) {
      ide_ctrl.ft_supported = 1;
      device_pci_write_32(offset, ide_ctrl.raw, fd);
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "ft_supported is set.\n"));
    } else {
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "ft_supported is already set.\n"));
    }

    ret = true;

SetFailed:
    close(fd);
    unset_device_info(fd);

    return ret;
}

static bool test_config_reset_ide_registers(pcie_ide_test_group_context_t *group_context)
{
  bool res;

  // reset ide registers
  res = reset_ide_registers(&group_context->common.upper_port,
                            group_context->common.top->type,
                            group_context->stream_id,
                            group_context->rp_stream_index,
                            true);
  if (!res)
  {
    TEEIO_ASSERT(false);
    return false;
  }

  res = reset_ide_registers(&group_context->common.lower_port,
                            group_context->common.top->type,
                            group_context->stream_id,
                            group_context->rp_stream_index,
                            false);
  if (!res)
  {
    TEEIO_ASSERT(false);
    return false;
  }

  return true;
}

bool test_config_enable_common(void *test_context)
{
  bool res = true;
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  // if connection is via switch or P2P, then we shall set ft_supported in the switch's port
  ide_common_test_switch_internal_conn_context_t* conn;
  if(group_context->common.top->connection == IDE_TEST_CONNECT_SWITCH || group_context->common.top->connection == IDE_TEST_CONNECT_P2P) {
    conn = group_context->common.sw_conn1;
    TEEIO_ASSERT(conn);

    res = true;

    while(res && conn) {
      res = set_ft_supported_in_switch(conn->dps.port->bdf) && set_ft_supported_in_switch(conn->ups.port->bdf);
      conn = conn->next;
    }
  }
  if(!res) {
    teeio_record_config_item_result(IDE_TEST_CONFIGURATION_TYPE_DEFAULT, TEEIO_TEST_CONFIG_FUNC_ENABLE, TEEIO_TEST_RESULT_FAILED); 
    return false;
  }

  if(group_context->common.top->connection == IDE_TEST_CONNECT_P2P) {
    conn = group_context->common.sw_conn2;
    TEEIO_ASSERT(conn);
    res = true;
    while(res && conn) {
      res = set_ft_supported_in_switch(conn->dps.port->bdf) && set_ft_supported_in_switch(conn->ups.port->bdf);
      conn = conn->next;
    }
  }

  if(!res) {
    teeio_record_config_item_result(IDE_TEST_CONFIGURATION_TYPE_DEFAULT, TEEIO_TEST_CONFIG_FUNC_ENABLE, TEEIO_TEST_RESULT_FAILED); 
    return res;
  }

  // then reset registers
  res = test_config_reset_ide_registers(group_context);
  teeio_record_config_item_result(IDE_TEST_CONFIGURATION_TYPE_DEFAULT, TEEIO_TEST_CONFIG_FUNC_ENABLE, res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED); 

  return res;
}

static bool get_pcie_ide_cap(PCIE_IDE_CAP *cap, uint32_t ecap_offset, int fd) {
    TEEIO_ASSERT(cap != NULL);
    TEEIO_ASSERT(fd > 0);

    uint32_t offset = ecap_offset + 4;
    cap->raw = device_pci_read_32(offset, fd);

    return true;
}

// check if the switch port supports ft_supported
bool check_ft_supported_in_switch_port(char* bdf)
{
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "check_ft_supported_in_switch_port bdf(%s)\n", bdf));

    int fd = open_configuration_space(bdf);
    if(fd == -1) {
        return false;
    }

    char dev_info[MAX_LINE_LENGTH] = {0};
    sprintf(dev_info, "switch-port: %s", bdf);
    set_deivce_info(fd, dev_info);

    bool supported = false;

    uint32_t ecap_offset = get_extended_cap_offset(fd, 0x30);

    if (ecap_offset == 0) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "ECAP Offset of %s is NOT found\n", bdf));
        goto CheckFailed;
    }

    PCIE_IDE_CAP ide_cap = {0};
    if(!get_pcie_ide_cap(&ide_cap, ecap_offset, fd)) {
        goto CheckFailed;
    }

    supported = ide_cap.ft_supported == 1;
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "ide_cap.ft_supported=%d\n", ide_cap.ft_supported));

CheckFailed:
    close(fd);
    unset_device_info(fd);

    return supported;
}

bool test_config_support_common(void *test_context)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  PCIE_IDE_CAP *host_cap = &group_context->common.upper_port.ide_cap;
  PCIE_IDE_CAP *dev_cap = &group_context->common.lower_port.ide_cap;

  bool supported = false;
  if(group_context->common.top->type == IDE_TEST_TOPOLOGY_TYPE_SEL_IDE) {
    supported = host_cap->sel_ide_supported == 1 && dev_cap->sel_ide_supported == 1;
  } else if(group_context->common.top->type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE) {
    supported = host_cap->sel_ide_supported == 1 && dev_cap->sel_ide_supported == 1;
  } else if(group_context->common.top->type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE) {
    supported = host_cap->sel_ide_supported == 1 && dev_cap->sel_ide_supported == 1 && host_cap->sel_ide_supported == 1 && dev_cap->sel_ide_supported == 1;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO,
              "test_config_support_common: host_cap->sel_ide_supported == %d && dev_cap->sel_ide_supported == %d\n",
              host_cap->sel_ide_supported, dev_cap->sel_ide_supported));

  if(!supported) {
    teeio_record_config_item_result(IDE_TEST_CONFIGURATION_TYPE_DEFAULT, TEEIO_TEST_CONFIG_FUNC_SUPPORT, TEEIO_TEST_RESULT_FAILED); 
    return false;
  }

  // check ft_supported if it is via switch
  ide_common_test_switch_internal_conn_context_t* conn;
  if(group_context->common.top->connection == IDE_TEST_CONNECT_SWITCH || group_context->common.top->connection == IDE_TEST_CONNECT_P2P) {
    conn = group_context->common.sw_conn1;
    TEEIO_ASSERT(conn);

    supported = true;
    while(conn) {
      if(!check_ft_supported_in_switch_port(conn->dps.port->bdf)) {
        supported = false;
        break;
      }

      if(!check_ft_supported_in_switch_port(conn->ups.port->bdf)) {
        supported = false;
        break;
      }

      conn = conn->next;
    }
  }
  if(!supported) {
    teeio_record_config_item_result(IDE_TEST_CONFIGURATION_TYPE_DEFAULT, TEEIO_TEST_CONFIG_FUNC_SUPPORT, TEEIO_TEST_RESULT_FAILED); 
    return false;
  }

  if(group_context->common.top->connection == IDE_TEST_CONNECT_P2P) {
    conn = group_context->common.sw_conn2;;
    TEEIO_ASSERT(conn);

    supported = true;
    while(conn) {
      if(!check_ft_supported_in_switch_port(conn->dps.port->bdf)) {
        supported = false;
        break;
      }

      if(!check_ft_supported_in_switch_port(conn->ups.port->bdf)) {
        supported = false;
        break;
      }

      conn = conn->next;
    }
  }

  teeio_record_config_item_result(IDE_TEST_CONFIGURATION_TYPE_DEFAULT, TEEIO_TEST_CONFIG_FUNC_SUPPORT, supported ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED); 
  return supported;
}
