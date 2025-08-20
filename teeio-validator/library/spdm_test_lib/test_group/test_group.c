/**
 *  Copyright Notice:
 *  Copyright 2025 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include "ide_test.h"
#include "pcie_ide_lib.h"
#include "library/spdm_transport_pcidoe_lib.h"
#include "teeio_spdmlib.h"
#include "helperlib.h"

extern int m_dev_fp;
extern uint32_t g_doe_extended_offset;

 bool spdm_scan_devices(void *test_context)
{
  bool ret = false;
  spdm_test_group_context_t *context = (spdm_test_group_context_t *)test_context;
  TEEIO_ASSERT(context->common.signature == GROUP_CONTEXT_SIGNATURE);

  IDE_TEST_TOPOLOGY *top = context->common.top;

  // first scan pcie devices to populate the complete bdf of each devices in a specific topology
  if(top->connection == IDE_TEST_CONNECT_DIRECT || top->connection == IDE_TEST_CONNECT_SWITCH) {
    // root_port and upper_port is the same port
    ret = scan_devices_at_bus(context->common.root_port.port, context->common.lower_port.port, context->common.sw_conn1, context->common.top->segment, context->common.top->bus);
    if(ret) {
      context->common.upper_port.port->bus = context->common.root_port.port->bus;
      strncpy(context->common.upper_port.port->bdf, context->common.root_port.port->bdf, BDF_LENGTH);
    }
  } else if(top->connection == IDE_TEST_CONNECT_P2P) {
    // root_port and upper_port is not the same port
    ret = scan_devices_at_bus(context->common.root_port.port, context->common.upper_port.port, context->common.sw_conn1, context->common.top->segment, context->common.top->bus);
    if(ret) {
      ret = scan_devices_at_bus(context->common.root_port.port, context->common.lower_port.port, context->common.sw_conn2, context->common.top->segment, context->common.top->bus);
    }
  }

  return ret;
}

/**
 * Initialize endpoint port
 */
bool spdm_init_dev_port(spdm_test_group_context_t *group_context)
{
  TEEIO_ASSERT(group_context != NULL);

  ide_common_test_port_context_t *port_context = &group_context->common.lower_port;
  TEEIO_ASSERT(port_context != NULL);

  if(!open_dev_port(port_context)) {
    return false;
  }
  
  return true;
}

bool spdm_close_dev_port(ide_common_test_port_context_t *port_context)
{
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "close_dev_port %s(%s)\n", port_context->port->port_name, port_context->port->bdf));

  if(port_context->cfg_space_fd > 0) {
    close(port_context->cfg_space_fd);
    unset_device_info(port_context->cfg_space_fd);
  }
  port_context->cfg_space_fd = 0;

  m_dev_fp = 0;
  g_doe_extended_offset = 0;
  return true;
}

/**
 * Initialize rootcomplex port
 * root_port and upper_port is the same port
 */
bool spdm_init_root_port(spdm_test_group_context_t *group_context)
{
  TEEIO_ASSERT(group_context != NULL);
  ide_common_test_port_context_t *port_context = &group_context->common.upper_port;
  TEEIO_ASSERT(port_context != NULL);

  if(!open_root_port(port_context)) {
    return false;
  }

  return true;
}

bool spdm_close_root_port(spdm_test_group_context_t *group_context)
{
  // clean Link/Selective IDE Stream Control Registers and KCBar corresponding registers
  ide_common_test_port_context_t* port_context = &group_context->common.upper_port;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "close_root_port %s(%s)\n", port_context->port->port_name, port_context->port->bdf));

  if(group_context->common.upper_port.cfg_space_fd > 0) {
    close(group_context->common.upper_port.cfg_space_fd);
    unset_device_info(group_context->common.upper_port.cfg_space_fd);
  }
  group_context->common.upper_port.cfg_space_fd = 0;
  
  return true;
}

void *spdm_init_client(void)
{
  void *spdm_context;
  void *scratch_buffer;
  size_t scratch_buffer_size;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "spdm_init_client\n"));

  spdm_context = (void *)malloc(libspdm_get_context_size());
  if (spdm_context == NULL) {
    return NULL;
  }
  libspdm_init_context(spdm_context);

  libspdm_register_device_io_func(spdm_context, device_doe_send_message, device_doe_receive_message);

  libspdm_register_transport_layer_func(
    spdm_context,
    LIBSPDM_MAX_SPDM_MSG_SIZE,
    LIBSPDM_TRANSPORT_HEADER_SIZE,
    LIBSPDM_TRANSPORT_TAIL_SIZE,
    libspdm_transport_pci_doe_encode_message,
    libspdm_transport_pci_doe_decode_message);

  libspdm_register_device_buffer_func(
    spdm_context,
    LIBSPDM_SENDER_BUFFER_SIZE,
    LIBSPDM_RECEIVER_BUFFER_SIZE,
    spdm_device_acquire_sender_buffer,
    spdm_device_release_sender_buffer,
    spdm_device_acquire_receiver_buffer,
    spdm_device_release_receiver_buffer);

  scratch_buffer_size = libspdm_get_sizeof_required_scratch_buffer(spdm_context);
  scratch_buffer = (void *)malloc(scratch_buffer_size);
  if (scratch_buffer == NULL) {
    free(spdm_context);
    return NULL;
  }
  libspdm_set_scratch_buffer (spdm_context, scratch_buffer, scratch_buffer_size);

  return spdm_context;
}

bool spdm_test_group_setup(void *test_context)
{
  bool ret = false;

  spdm_test_group_context_t *context = (spdm_test_group_context_t *)test_context;
  TEEIO_ASSERT(context->common.signature == GROUP_CONTEXT_SIGNATURE);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "test_group_setup start\n"));

  // first scan devices
  if(!spdm_scan_devices(test_context)) {
    teeio_record_group_result(TEEIO_TEST_GROUP_FUNC_SETUP, TEEIO_TEST_RESULT_FAILED, "Scan device failed.");
    return false;
  }

  // initialize lower_port
  ret = spdm_init_dev_port(context);
  if(!ret) {
    teeio_record_group_result(TEEIO_TEST_GROUP_FUNC_SETUP, TEEIO_TEST_RESULT_FAILED, "Initialize device port failed.");
    return false;
  }

  IDE_TEST_TOPOLOGY *top = context->common.top;
  TEEIO_ASSERT(top->connection == IDE_TEST_CONNECT_DIRECT || top->connection == IDE_TEST_CONNECT_SWITCH);

  ret = spdm_init_root_port(context);
  if (!ret) {
    teeio_record_group_result(TEEIO_TEST_GROUP_FUNC_SETUP, TEEIO_TEST_RESULT_FAILED, "Initialize root port failed.");
    return false;
  }

  // init spdm_context
  void *spdm_context = spdm_init_client();
  if(spdm_context == NULL) {
    teeio_record_group_result(TEEIO_TEST_GROUP_FUNC_SETUP, TEEIO_TEST_RESULT_FAILED, "Initialize spdm failed.");
    return false;
  }

  context->spdm_doe.spdm_context = spdm_context;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "test_group_setup done\n"));

  teeio_record_group_result(TEEIO_TEST_GROUP_FUNC_SETUP, TEEIO_TEST_RESULT_PASS, "");

  return true;
}

bool spdm_test_group_teardown(void *test_context)
{
  spdm_test_group_context_t *context = (spdm_test_group_context_t *)test_context;
  TEEIO_ASSERT(context->common.signature == GROUP_CONTEXT_SIGNATURE);

  // close spdm_session and free spdm_context
  if(context->spdm_doe.spdm_context != NULL) {
    free(context->spdm_doe.spdm_context);
    context->spdm_doe.spdm_context = NULL;
    context->spdm_doe.session_id = 0;
  }

  IDE_TEST_TOPOLOGY *top = context->common.top;
  // close ports
  spdm_close_dev_port(&context->common.lower_port);

  if(top->connection == IDE_TEST_CONNECT_DIRECT || top->connection == IDE_TEST_CONNECT_SWITCH) {
    spdm_close_root_port(context);
  } else if(top->connection == IDE_TEST_CONNECT_P2P ){
    // close both root_port and upper_port
    NOT_IMPLEMENTED("Close both root_port and upper_port for peer2peer connection.");
  }

  teeio_record_group_result(TEEIO_TEST_GROUP_FUNC_TEARDOWN, TEEIO_TEST_RESULT_PASS, "");

  return true;
}
