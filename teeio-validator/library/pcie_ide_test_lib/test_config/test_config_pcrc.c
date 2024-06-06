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

extern const char *m_ide_test_topology_name[];

// set pcrc for selective_ide and link_ide.
static bool test_config_set_pcrc_common(void *test_context, bool enable)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  ide_common_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  // enable pcrc bit in
  TEST_IDE_TYPE ide_type = TEST_IDE_TYPE_SEL_IDE;
  IDE_TEST_TOPOLOGY *top = group_context->top;
  if(top->type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE) {
    ide_type = TEST_IDE_TYPE_LNK_IDE;
  } else if (top->type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE){
    NOT_IMPLEMENTED("selective_and_link_ide topology");
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%s pcrc for %s\n", enable ? "enable" : "disable", m_ide_test_topology_name[top->type]));

  ide_common_test_port_context_t *port_context = &group_context->upper_port;
  set_pcrc_in_ecap(port_context->cfg_space_fd, ide_type, port_context->ide_id, port_context->ecap_offset, enable);
  port_context = &group_context->lower_port;
  set_pcrc_in_ecap(port_context->cfg_space_fd, ide_type, port_context->ide_id, port_context->ecap_offset, enable);

  return true;
}

static bool test_config_check_pcrc_support_common(void *test_context)
{
  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

  ide_common_test_group_context_t *group_context = config_context->group_context;
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  PCIE_IDE_CAP *host_cap = &group_context->upper_port.ide_cap;
  PCIE_IDE_CAP *dev_cap = &group_context->lower_port.ide_cap;
  if(!host_cap->pcrc_supported || !dev_cap->pcrc_supported) {
    TEEIO_DEBUG((TEEIO_DEBUG_WARN, "check pcrc and it is NOT supported. host=%d, device=%d\n", host_cap->pcrc_supported, dev_cap->pcrc_supported));
    return false;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "check pcrc and it is supported.\n"));

  return true;
}

// selective_ide test pcrc
bool pcie_ide_test_config_pcrc_enable_sel(void *test_context)
{
  return test_config_set_pcrc_common(test_context, true);
}

bool pcie_ide_test_config_pcrc_disable_sel(void *test_context)
{
  return test_config_set_pcrc_common(test_context, false);
}

bool pcie_ide_test_config_pcrc_support_sel(void *test_context)
{
  return test_config_check_pcrc_support_common(test_context);
}

bool pcie_ide_test_config_pcrc_check_sel(void *test_context)
{
  return true;
}

// link_ide test pcrc
bool pcie_ide_test_config_pcrc_enable_link(void *test_context)
{
    return test_config_set_pcrc_common(test_context, true);
}

bool pcie_ide_test_config_pcrc_disable_link(void *test_context)
{
  return test_config_set_pcrc_common(test_context, false);
}

bool pcie_ide_test_config_pcrc_support_link(void *test_context)
{
  return test_config_check_pcrc_support_common(test_context);
}

bool pcie_ide_test_config_pcrc_check_link(void *test_context)
{
  return true;
}

// selective_and_link_ide test pcrc
bool pcie_ide_test_config_pcrc_enable_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return false;
}

bool pcie_ide_test_config_pcrc_disable_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return false;
}

bool pcie_ide_test_config_pcrc_support_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return false;
}

bool pcie_ide_test_config_pcrc_check_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return false;
}
