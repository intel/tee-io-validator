/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
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

extern const char *m_ide_type_name[];
const char* m_config_name = "Selective IDE for Configuration Request";

bool set_sel_ide_for_cfg_req_in_ecap(
    int fd,
    TEST_IDE_TYPE ide_type,
    uint8_t ide_id,
    uint32_t ide_ecap_offset,
    bool enable
);

/**
 * Enable/Disable Selective_IDE for Configuration
*/
static bool test_config_set_sel_ide_for_cfg_req(void* test_context, bool enable)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  TEST_IDE_TYPE ide_type = map_top_type_to_ide_type(group_context->top->type);

  // enable cfg_sel_ide bit in upper port and lower port
  ide_common_test_port_context_t* port_context = &group_context->upper_port;
  set_sel_ide_for_cfg_req_in_ecap(port_context->cfg_space_fd,
                          ide_type,
                          port_context->ide_id,
                          port_context->ecap_offset,
                          enable);

  port_context = &group_context->lower_port;
  set_sel_ide_for_cfg_req_in_ecap(port_context->cfg_space_fd,
                          ide_type,
                          port_context->ide_id,
                          port_context->ecap_offset,
                          enable);

  return true;
}

/**
 * Check if Selective_IDE for Configuration Request is supported.
*/
static bool test_config_check_sel_ide_for_cfg_req_support(void* test_context)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  TEST_IDE_TYPE ide_type = map_top_type_to_ide_type(group_context->top->type);
  if(ide_type != TEST_IDE_TYPE_SEL_IDE) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "\"%s\" is not avaible in %s\n", m_config_name, m_ide_type_name[ide_type]));
    return false;    
  }

  PCIE_IDE_CAP *upper_cap = &group_context->upper_port.ide_cap;
  PCIE_IDE_CAP *lower_cap = &group_context->lower_port.ide_cap;
  bool supported = upper_cap->sel_ide_cfg_req_supported && lower_cap->sel_ide_cfg_req_supported;
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%s is %s.\n", m_config_name, supported ? "supported" : "NOT supported"));
  return supported;
}

// selective_ide test sel_ide_for_cfg_req
bool pcie_ide_test_config_enable_sel_ide_for_cfg_req(void *test_context)
{
  return test_config_set_sel_ide_for_cfg_req(test_context, true);
}

bool pcie_ide_test_config_disable_sel_ide_for_cfg_req(void *test_context)
{
  return test_config_set_sel_ide_for_cfg_req(test_context, false);
}

bool pcie_ide_test_config_support_sel_ide_for_cfg_req(void *test_context)
{
  return test_config_check_sel_ide_for_cfg_req_support(test_context);
}

bool pcie_ide_test_config_check_sel_ide_for_cfg_req(void *test_context)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  TEST_IDE_TYPE ide_type = map_top_type_to_ide_type(group_context->top->type);
  TEEIO_ASSERT(ide_type == TEST_IDE_TYPE_SEL_IDE);

  ide_common_test_port_context_t* port_context = &group_context->upper_port;
  uint32_t data1 =read_ide_stream_ctrl_in_ecap(port_context->cfg_space_fd,
                          ide_type,
                          port_context->ide_id,
                          port_context->ecap_offset);

  port_context = &group_context->lower_port;
  uint32_t data2 = read_ide_stream_ctrl_in_ecap(port_context->cfg_space_fd,
                          ide_type,
                          port_context->ide_id,
                          port_context->ecap_offset);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Read ide_stream_ctrl : rootport = 0x%08x, dev = 0x%08x\n", data1, data2));

  PCIE_SEL_IDE_STREAM_CTRL rp_ide_stream_ctrl = {.raw = data1};
  PCIE_SEL_IDE_STREAM_CTRL dev_ide_stream_ctrl = {.raw = data2};
  bool pass = rp_ide_stream_ctrl.cfg_sel_ide == 1 && dev_ide_stream_ctrl.cfg_sel_ide;
  if(pass) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Check %s pass\n", m_config_name));
  } else {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Check %s failed\n", m_config_name));
  }

  return pass;
}
