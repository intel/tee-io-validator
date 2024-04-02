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
#include "hal/library/platform_lib.h"
#include "ide_test.h"
#include "pcie.h"
#include "utils.h"
#include "teeio_debug.h"

extern const char *IDE_TEST_IDE_TYPE_NAMES[];
int open_configuration_space(char *bdf);
uint32_t get_extended_cap_offset(int fd, uint32_t ext_id);
uint32_t read_host_stream_status_in_ecap(int cfg_space_fd, uint32_t ecap_offset, TEST_IDE_TYPE ide_type, uint8_t ide_id);
bool reset_ide_registers(
    ide_common_test_port_context_t *port_context,
    IDE_TEST_TOPOLOGY_TYPE top_type,
    uint8_t stream_id,
    uint8_t rp_stream_index,
    bool reset_kcbar);

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

  ide_common_test_group_context_t *group_context = (ide_common_test_group_context_t *)config_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  ide_common_test_port_context_t *port = &group_context->upper_port;
  TEST_IDE_TYPE ide_type = TEST_IDE_TYPE_SEL_IDE;
  if (group_context->top->type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE)
  {
    ide_type = TEST_IDE_TYPE_LNK_IDE;
  }
  else if(group_context->top->type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE)
  {
    NOT_IMPLEMENTED("selective_and_link_ide topology");
  }

  uint32_t data = read_host_stream_status_in_ecap(port->cfg_space_fd, port->ecap_offset, ide_type, port->ide_id);
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

  config_context->test_result = status == IDE_STREAM_STATUS_TYPE_SECURE ? IDE_COMMON_TEST_CONFIG_RESULT_SUCCESS : IDE_COMMON_TEST_CONFIG_RESULT_FAILED;

  return true;
}

// set ft_supported
bool set_ft_supported_in_switch(char* port_bdf)
{
    if(port_bdf == NULL) {
        return false;
    }

    bool ret = false;
    int fd = open_configuration_space(port_bdf);
    if(fd == -1) {
        return false;
    }

    uint32_t ecap_offset = get_extended_cap_offset(fd, 0x30);

    if (ecap_offset == 0) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "ECAP Offset of %s is NOT found\n", port_bdf));
        goto SetFailed;
    }

    uint32_t offset = ecap_offset + 8;
    PCIE_IDE_CTRL ide_ctrl = {.raw = device_pci_read_32(offset, fd)};
    ide_ctrl.ft_supported = 1;
    device_pci_write_32(offset, ide_ctrl.raw, fd);

    ret = true;

SetFailed:
    close(fd);

    return ret;
}

static bool test_config_reset_ide_registers(ide_common_test_group_context_t *group_context)
{
  bool res;

  // reset ide registers
  res = reset_ide_registers(&group_context->upper_port,
                            group_context->top->type,
                            group_context->stream_id,
                            group_context->rp_stream_index,
                            true);
  if (!res)
  {
    TEEIO_ASSERT(false);
    return false;
  }

  res = reset_ide_registers(&group_context->lower_port,
                            group_context->top->type,
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

  ide_common_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  // if connection is via switch or P2P, then we shall set ft_supported in the switch's port
  ide_common_test_switch_internal_conn_context_t* conn;
  if(group_context->top->connection == IDE_TEST_CONNECT_SWITCH || group_context->top->connection == IDE_TEST_CONNECT_P2P) {
    conn = group_context->sw_conn1;
    TEEIO_ASSERT(conn);

    res = true;

    while(res && conn) {
      res = set_ft_supported_in_switch(conn->dps.port->bdf) && set_ft_supported_in_switch(conn->ups.port->bdf);
      conn = conn->next;
    }
  }
  if(!res) {
    return false;
  }

  if(group_context->top->connection == IDE_TEST_CONNECT_P2P) {
    conn = group_context->sw_conn2;
    TEEIO_ASSERT(conn);
    res = true;
    while(res && conn) {
      res = set_ft_supported_in_switch(conn->dps.port->bdf) && set_ft_supported_in_switch(conn->ups.port->bdf);
      conn = conn->next;
    }
  }

  if(!res) {
    return res;
  }

  // then reset registers
  res = test_config_reset_ide_registers(group_context);

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
    int fd = open_configuration_space(bdf);
    if(fd == -1) {
        return false;
    }

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

CheckFailed:
    close(fd);

    return supported;
}

bool test_config_support_common(void *test_context)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  ide_common_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  PCIE_IDE_CAP *host_cap = &group_context->upper_port.ide_cap;
  PCIE_IDE_CAP *dev_cap = &group_context->lower_port.ide_cap;

  bool supported = false;
  if(group_context->top->type == IDE_TEST_TOPOLOGY_TYPE_SEL_IDE) {
    supported = host_cap->sel_ide_supported == 1 && dev_cap->sel_ide_supported == 1;
  } else if(group_context->top->type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE) {
    supported = host_cap->sel_ide_supported == 1 && dev_cap->sel_ide_supported == 1;
  } else if(group_context->top->type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE) {
    supported = host_cap->sel_ide_supported == 1 && dev_cap->sel_ide_supported == 1 && host_cap->sel_ide_supported == 1 && dev_cap->sel_ide_supported == 1;
  }

  if(!supported) {
    return false;
  }

  // check ft_supported if it is via switch
  ide_common_test_switch_internal_conn_context_t* conn;
  if(group_context->top->connection == IDE_TEST_CONNECT_SWITCH || group_context->top->connection == IDE_TEST_CONNECT_P2P) {
    conn = group_context->sw_conn1;
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
    return false;
  }

  if(group_context->top->connection == IDE_TEST_CONNECT_P2P) {
    conn = group_context->sw_conn2;;
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

  return supported;
}
