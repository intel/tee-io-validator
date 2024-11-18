/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "hal/base.h"
#include "hal/library/debuglib.h"

#include "library/spdm_requester_lib.h"
#include "library/spdm_crypt_lib.h"
#include "ide_test.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "pcie_ide_lib.h"
#include "teeio_spdmlib.h"

bool scan_devices(void *test_context)
{
  bool ret = false;
  pcie_ide_test_group_context_t *context = (pcie_ide_test_group_context_t *)test_context;
  TEEIO_ASSERT(context->common.signature == GROUP_CONTEXT_SIGNATURE);

  IDE_TEST_TOPOLOGY *top = context->common.top;

  // first scan pcie devices to populate the complete bdf of each devices in a specific topology
  if(top->connection == IDE_TEST_CONNECT_DIRECT || top->connection == IDE_TEST_CONNECT_SWITCH) {
    // root_port and upper_port is the same port
    ret = scan_devices_at_bus(context->common.root_port.port, context->common.lower_port.port, context->common.sw_conn1, context->common.top->bus);
    if(ret) {
      context->common.upper_port.port->bus = context->common.root_port.port->bus;
      strncpy(context->common.upper_port.port->bdf, context->common.root_port.port->bdf, BDF_LENGTH);
    }
  } else if(top->connection == IDE_TEST_CONNECT_P2P) {
    // root_port and upper_port is not the same port
    ret = scan_devices_at_bus(context->common.root_port.port, context->common.upper_port.port, context->common.sw_conn1, context->common.top->bus);
    if(ret) {
      ret = scan_devices_at_bus(context->common.root_port.port, context->common.lower_port.port, context->common.sw_conn2, context->common.top->bus);
    }
  }

  return ret;
}

// selective_ide test group

/**
 * This function works to setup selective_ide and link_ide
 *
 * 1. open cofiguration_space and find the ecap_offset
 * 2. map kcbar to user space
 * 3. initialize spdm_context and doe_context
 * 4. setup spdm_session
 */
static bool common_test_group_setup(void *test_context)
{
  bool ret = false;

  pcie_ide_test_group_context_t *context = (pcie_ide_test_group_context_t *)test_context;
  TEEIO_ASSERT(context->common.signature == GROUP_CONTEXT_SIGNATURE);

  // first scan devices
  if(!scan_devices(test_context)) {
    return false;
  }

  // initialize lower_port
  ret = init_dev_port(context);
  if(!ret) {
    return false;
  }

  IDE_TEST_TOPOLOGY *top = context->common.top;
  if(top->connection == IDE_TEST_CONNECT_DIRECT || top->connection == IDE_TEST_CONNECT_SWITCH) {
    ret = init_root_port(context);
  } else if(top->connection == IDE_TEST_CONNECT_P2P ){
    NOT_IMPLEMENTED("Open both root_port and upper_port for peer2peer connection.");
  } else {
    ret = false;
  }

  if (!ret) {
    return false;
  }

  // init spdm_context
  void *spdm_context = spdm_client_init();
  TEEIO_ASSERT(spdm_context != NULL);

  uint32_t session_id = 0;
  ret = spdm_connect(spdm_context, &session_id);
  TEEIO_ASSERT(ret);

  context->spdm_doe.spdm_context = spdm_context;
  context->spdm_doe.session_id = session_id;

  return true;
}

static bool common_test_group_teardown(void *test_context)
{
  pcie_ide_test_group_context_t *context = (pcie_ide_test_group_context_t *)test_context;
  TEEIO_ASSERT(context->common.signature == GROUP_CONTEXT_SIGNATURE);

  // close spdm_session and free spdm_context
  if(context->spdm_doe.spdm_context != NULL) {
    spdm_stop(context->spdm_doe.spdm_context, context->spdm_doe.session_id);
    free(context->spdm_doe.spdm_context);
    context->spdm_doe.spdm_context = NULL;
    context->spdm_doe.session_id = 0;
  }

  IDE_TEST_TOPOLOGY *top = context->common.top;
  // close ports
  close_dev_port(&context->common.lower_port, top->type);

  if(top->connection == IDE_TEST_CONNECT_DIRECT || top->connection == IDE_TEST_CONNECT_SWITCH) {
    close_root_port(context);
  } else if(top->connection == IDE_TEST_CONNECT_P2P ){
    // close both root_port and upper_port
    NOT_IMPLEMENTED("Close both root_port and upper_port for peer2peer connection.");
  }

  return true;
}

bool pcie_ide_test_group_setup_sel(void *test_context)
{
  return common_test_group_setup(test_context);
}

bool pcie_ide_test_group_teardown_sel(void *test_context)
{
  return common_test_group_teardown(test_context);
}

// link_ide test group
bool pcie_ide_test_group_setup_link(void *test_context)
{
  return common_test_group_setup(test_context);
}

bool pcie_ide_test_group_teardown_link(void *test_context)
{
  return common_test_group_teardown(test_context);
}

// selective_and_link_ide test group
bool pcie_ide_test_group_setup_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return true;
}

bool pcie_ide_test_group_teardown_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return true;
}