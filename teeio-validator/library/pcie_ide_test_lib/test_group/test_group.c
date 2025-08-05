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
#include "library/pci_ide_km_common_lib.h"
#include "library/pci_ide_km_requester_lib.h"

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

// use ide_km to query the port_index matching with BDF of lower port
bool ide_query_port_index(void *test_context)
{
  libspdm_return_t status;
  pcie_ide_test_group_context_t *context = (pcie_ide_test_group_context_t *)test_context;
  uint8_t port_index = 0;
  uint8_t dev_func = ((context->common.lower_port.port->device & 0x1f) << 3) | (context->common.lower_port.port->function & 0x7);
  uint8_t bus = context->common.lower_port.port->bus;
  uint8_t segment = 0;
  uint8_t max_port_index = 0;
  uint32_t ide_reg_block[PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT] = {0};
  uint32_t ide_reg_block_count = PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT;
  uint8_t port_dev_func = dev_func;
  uint8_t port_bus = bus;
  bool found = false;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Querying PCI IDE KM for dev_func=0x%x, bus=0x%x, segment=0x%x\n",
               port_dev_func, port_bus, segment));
  status = pci_ide_km_query(context->spdm_doe.doe_context,
                            context->spdm_doe.spdm_context,
                            &context->spdm_doe.session_id,
                            port_index, &dev_func, &bus,
                            &segment, &max_port_index,
                            ide_reg_block, &ide_reg_block_count);
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "pci_ide_km_query failed with status=0x%x\n", status));
    return false;
  }
  TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "pci_ide_km_query result: port_index=%d dev_func=0x%x bus=0x%x segment=0x%x max_port_index=%d\n",
               port_index, dev_func, bus, segment, max_port_index));

  if (port_dev_func == dev_func && port_bus == bus){
    context->common.lower_port.port->port_index = port_index;
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Found matching port_index=%d for lower_port\n", port_index));
    found = true;
  } else if (max_port_index > 0) {
    for (port_index = 1; port_index < max_port_index + 1; port_index++) {
      status = pci_ide_km_query(context->spdm_doe.doe_context,
                                context->spdm_doe.spdm_context,
                                &context->spdm_doe.session_id,
                                port_index, &dev_func, &bus,
                                &segment, &max_port_index,
                                ide_reg_block, &ide_reg_block_count);
      if (LIBSPDM_STATUS_IS_ERROR(status)) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "pci_ide_km_query failed with status=0x%x\n", status));
        return false;
      }
      TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "pci_ide_km_query result: port_index=%d dev_func=0x%x bus=0x%x segment=0x%x max_port_index=%d\n",
                  port_index, dev_func, bus, segment, max_port_index));

      if (port_dev_func == dev_func && port_bus == bus) {
        context->common.lower_port.port->port_index = port_index;
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Found matching port_index=%d for lower_port\n", port_index));
        found = true;
      }
    }
  }

  // if not found, use default port_index 0
  // this is not ideal, but it is a fallback mechanism to ensure that the test can still run
  // for example, Key Prog cases without querying port_index.
  if (!found) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to find matching port_index for lower_port\n"));
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Using default port_index=0 for lower_port\n"));
    context->common.lower_port.port->port_index = 0;
  }

  return true;
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
    teeio_record_group_result(TEEIO_TEST_GROUP_FUNC_SETUP, TEEIO_TEST_RESULT_FAILED, "Scan device failed.");
    return false;
  }

  // initialize lower_port
  ret = init_dev_port(context);
  if(!ret) {
    teeio_record_group_result(TEEIO_TEST_GROUP_FUNC_SETUP, TEEIO_TEST_RESULT_FAILED, "Initialize device port failed.");
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
    teeio_record_group_result(TEEIO_TEST_GROUP_FUNC_SETUP, TEEIO_TEST_RESULT_FAILED, "Initialize root port failed.");
    return false;
  }

  // init spdm_context
  void *spdm_context = spdm_client_init();
  if(spdm_context == NULL) {
    teeio_record_group_result(TEEIO_TEST_GROUP_FUNC_SETUP, TEEIO_TEST_RESULT_FAILED, "Initialize spdm failed.");
    return false;
  }

  uint32_t session_id = 0;
  if(!spdm_connect(spdm_context, &session_id)) {
    teeio_record_group_result(TEEIO_TEST_GROUP_FUNC_SETUP, TEEIO_TEST_RESULT_FAILED, "Spdm connect failed.");
    return false;
  }

  context->spdm_doe.spdm_context = spdm_context;
  context->spdm_doe.session_id = session_id;

  if (!ide_query_port_index(test_context)) {
    teeio_record_group_result(TEEIO_TEST_GROUP_FUNC_SETUP, TEEIO_TEST_RESULT_FAILED, "Query port_index for lower_port failed.");
    return false;
  }

  teeio_record_group_result(TEEIO_TEST_GROUP_FUNC_SETUP, TEEIO_TEST_RESULT_PASS, "");

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

  teeio_record_group_result(TEEIO_TEST_GROUP_FUNC_TEARDOWN, TEEIO_TEST_RESULT_PASS, "");

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