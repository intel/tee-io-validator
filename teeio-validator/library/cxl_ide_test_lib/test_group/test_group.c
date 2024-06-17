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
#include "cxl_ide_lib.h"
#include "pcie_ide_lib.h"
#include "teeio_spdmlib.h"

bool cxl_scan_devices(void *test_context)
{
  bool ret = false;
  ide_common_test_group_context_t *context = (ide_common_test_group_context_t *)test_context;
  TEEIO_ASSERT(context->signature == GROUP_CONTEXT_SIGNATURE);

  IDE_TEST_TOPOLOGY *top = context->top;

  TEEIO_ASSERT(top->ide_type == IDE_HW_TYPE_CXL_MEM);
  TEEIO_ASSERT(top->connection == IDE_TEST_CONNECT_DIRECT || top->connection == IDE_TEST_CONNECT_SWITCH);

  ret = scan_devices_at_bus(context->root_port.port, context->lower_port.port, context->sw_conn1, context->top->bus);
  if(ret) {
    context->upper_port.port->bus = context->root_port.port->bus;
    strncpy(context->upper_port.port->bdf, context->root_port.port->bdf, BDF_LENGTH);
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

  ide_common_test_group_context_t *context = (ide_common_test_group_context_t *)test_context;
  TEEIO_ASSERT(context->signature == GROUP_CONTEXT_SIGNATURE);

  // first scan devices
  if(!cxl_scan_devices(test_context)) {
    return false;
  }

  // initialize lower_port
  ret = cxl_init_dev_port(context);
  if(!ret) {
    return false;
  }

  IDE_TEST_TOPOLOGY *top = context->top;
  TEEIO_ASSERT(top->connection == IDE_TEST_CONNECT_DIRECT || top->connection == IDE_TEST_CONNECT_SWITCH);

  ret = cxl_init_root_port(context);
  if (!ret) {
    return false;
  }

  // init spdm_context
  void *spdm_context = spdm_client_init();
  TEEIO_ASSERT(spdm_context != NULL);

  uint32_t session_id = 0;
  ret = spdm_connect(spdm_context, &session_id);
  if (!ret) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "spdm_connect failed.\n"));
    return false;
  }

  context->spdm_context = spdm_context;
  context->session_id = session_id;

  return true;
}

static bool common_test_group_teardown(void *test_context)
{
  ide_common_test_group_context_t *context = (ide_common_test_group_context_t *)test_context;
  TEEIO_ASSERT(context->signature == GROUP_CONTEXT_SIGNATURE);

  // close spdm_session and free spdm_context
  if(context->spdm_context != NULL) {
    spdm_stop(context->spdm_context, context->session_id);
    free(context->spdm_context);
    context->spdm_context = NULL;
    context->session_id = 0;
  }

  IDE_TEST_TOPOLOGY *top = context->top;
  TEEIO_ASSERT(top->connection == IDE_TEST_CONNECT_DIRECT || top->connection == IDE_TEST_CONNECT_SWITCH);

  // close ports
  cxl_close_dev_port(&context->lower_port, top->type);
  cxl_close_root_port(context);

  return true;
}

bool cxl_ide_test_group_setup(void *test_context)
{
  return common_test_group_setup(test_context);
}

bool cxl_ide_test_group_teardown(void *test_context)
{
  return common_test_group_teardown(test_context);
}
