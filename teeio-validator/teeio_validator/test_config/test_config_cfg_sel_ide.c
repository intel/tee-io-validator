/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>

#include "hal/base.h"
#include "hal/library/debuglib.h"
#include "hal/library/platform_lib.h"
#include "ide_test.h"
#include "utils.h"
#include "teeio_debug.h"

bool test_config_enable_common(void *test_context);
bool test_config_support_common(void *test_context);
bool test_config_check_common(void *test_context, const char* assertion_msg);
bool set_cfg_sel_ide_in_ecap(
    int fd,
    TEST_IDE_TYPE ide_type,
    uint8_t ide_id,
    uint32_t ide_ecap_offset,
    bool enable
);


/**
 * Enable/Disable Selective_IDE for Configuration
*/
static bool test_config_set_sel_ide_for_config_req(void* test_context, bool enable)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  ide_common_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  TEST_IDE_TYPE ide_type = map_top_type_to_ide_type(group_context->top->type);

  // enable cfg_sel_ide bit in upper port and lower port
  ide_common_test_port_context_t* port_context = &group_context->upper_port;
  set_cfg_sel_ide_in_ecap(port_context->cfg_space_fd,
                          ide_type,
                          port_context->ide_id,
                          port_context->ecap_offset,
                          enable);

  port_context = &group_context->lower_port;
  set_cfg_sel_ide_in_ecap(port_context->cfg_space_fd,
                          ide_type,
                          port_context->ide_id,
                          port_context->ecap_offset,
                          enable);

  return true;
}

/**
 * Check if Selective_IDE for Config Req is supported.
*/
static bool test_config_check_sel_ide_for_config_req_support(void* test_context)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  ide_common_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  PCIE_IDE_CAP *upper_cap = &group_context->upper_port.ide_cap;
  PCIE_IDE_CAP *lower_cap = &group_context->lower_port.ide_cap;
  if(!upper_cap->sel_ide_cfg_req_supported || !lower_cap->sel_ide_cfg_req_supported) {
    return false;
  }

  return true;
}

// enable selective_ide_for_config_req
bool test_config_sel_ide_for_config_req_enable(void *test_context)
{
  bool res = test_config_enable_common(test_context);
  if(res) {
    res = test_config_set_sel_ide_for_config_req(test_context, true);
  }

  return res;
}

// disable selective_ide_for_config_req
bool test_config_sel_ide_for_config_req_disable(void *test_context)
{
  return test_config_set_sel_ide_for_config_req(test_context, false);
}

// check if selective_ide_for_config_req is supported
bool test_config_sel_ide_for_config_req_support(void *test_context)
{
  return test_config_support_common(test_context) &&
          test_config_check_sel_ide_for_config_req_support(test_context);

}

// check if the test config of selective_ide_for_config_req is successful
bool test_config_sel_ide_for_config_req_check(void *test_context)
{
  return test_config_check_common(test_context, "Selective IDE for Configuration Req Assertion");
}
