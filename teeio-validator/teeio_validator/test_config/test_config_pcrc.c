/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>

#include "assert.h"
#include "hal/base.h"
#include "hal/library/debuglib.h"
#include "hal/library/platform_lib.h"
#include "ide_test.h"
#include "utils.h"
#include "teeio_debug.h"

bool set_pcrc_in_ecap(
    int fd, TEST_IDE_TYPE ide_type,
    uint8_t ide_id, uint32_t ide_ecap_offset,
    bool enable
);
bool test_config_enable_common(void *test_context);
bool test_config_check_common(void *test_context, const char* assertion_msg);
bool test_config_support_common(void *test_context);

// set pcrc for selective_ide and link_ide.
static bool test_config_set_pcrc_common(void *test_context, bool enable)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  assert(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  ide_common_test_group_context_t *group_context = config_context->group_context;
  assert(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  // enable pcrc bit in
  TEST_IDE_TYPE ide_type = TEST_IDE_TYPE_SEL_IDE;
  IDE_TEST_TOPOLOGY *top = group_context->top;
  if(top->type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE) {
    ide_type = TEST_IDE_TYPE_LNK_IDE;
  } else if (top->type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE){
    NOT_IMPLEMENTED("selective_and_link_ide topology");
  }

  ide_common_test_port_context_t *port_context = &group_context->upper_port;
  set_pcrc_in_ecap(port_context->cfg_space_fd, ide_type, port_context->ide_id, port_context->ecap_offset, enable);

  return true;
}

static bool test_config_check_pcrc_support_common(void *test_context)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  assert(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  ide_common_test_group_context_t *group_context = config_context->group_context;
  assert(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  PCIE_IDE_CAP *host_cap = &group_context->upper_port.ide_cap;
  PCIE_IDE_CAP *dev_cap = &group_context->lower_port.ide_cap;
  if(!host_cap->pcrc_supported || !dev_cap->pcrc_supported) {
    return false;
  }

  return true;
}

// selective_ide test pcrc
bool test_config_pcrc_enable_sel(void *test_context)
{
  bool res = test_config_enable_common(test_context);
  if(res) {
    res = test_config_set_pcrc_common(test_context, true);
  }

  return res;
}

bool test_config_pcrc_disable_sel(void *test_context)
{
  return test_config_set_pcrc_common(test_context, false);
}

bool test_config_pcrc_support_sel(void *test_context)
{
  return test_config_support_common(test_context) &&
          test_config_check_pcrc_support_common(test_context);
}

bool test_config_pcrc_check_sel(void *test_context)
{
  return test_config_check_common(test_context, "PCRC Config Assertion");
}

// link_ide test pcrc
bool test_config_pcrc_enable_link(void *test_context)
{
  bool res = test_config_enable_common(test_context);
  if(res) {
    res = test_config_set_pcrc_common(test_context, true);
  }

  return res;
}

bool test_config_pcrc_disable_link(void *test_context)
{
  return test_config_set_pcrc_common(test_context, false);
}

bool test_config_pcrc_support_link(void *test_context)
{
  return test_config_check_pcrc_support_common(test_context);
}

bool test_config_pcrc_check_link(void *test_context)
{
  return test_config_check_common(test_context, "PCRC Config Assertion");
}

// selective_and_link_ide test pcrc
bool test_config_pcrc_enable_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return false;
}

bool test_config_pcrc_disable_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return false;
}

bool test_config_pcrc_support_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return false;
}

bool test_config_pcrc_check_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return false;
}
