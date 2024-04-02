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
#include "hal/library/platform_lib.h"
#include "library/spdm_requester_lib.h"
#include "library/spdm_crypt_lib.h"
#include "ide_test.h"
#include "utils.h"
#include "teeio_debug.h"

extern void *m_pci_doe_context;

void trigger_doe_abort();
bool is_doe_error_asserted();
bool init_host_port(ide_common_test_group_context_t *group_context);
bool close_host_port(ide_common_test_group_context_t *group_context);
bool init_both_root_upper_port(ide_common_test_group_context_t *group_context);
bool close_both_root_upper_port(ide_common_test_group_context_t *group_context);
bool init_dev_port(ide_common_test_group_context_t *group_context);
bool close_dev_port(ide_common_test_port_context_t *port_context, IDE_TEST_TOPOLOGY_TYPE top_type);
void *spdm_client_init(void);
bool spdm_connect (void *spdm_context, uint32_t *session_id);
bool spdm_stop(void *spdm_context, uint32_t session_id);
libspdm_return_t pci_doe_init_request();
bool scan_devices(void *test_context);

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
  if(!scan_devices(test_context)) {
    return false;
  }

  // initialize lower_port
  ret = init_dev_port(context);
  if(!ret) {
    return false;
  }

  IDE_TEST_TOPOLOGY *top = context->top;
  if(top->connection == IDE_TEST_CONNECT_DIRECT || top->connection == IDE_TEST_CONNECT_SWITCH) {
    ret = init_host_port(context);
  } else if(top->connection == IDE_TEST_CONNECT_P2P ){
    // TODO
    // init both root_port and upper_port
    ret = init_both_root_upper_port(context);
  } else {
    ret = false;
  }

  if (!ret) {
    return false;
  }

  // init doe
  trigger_doe_abort();
  libspdm_sleep(1000000);
  if (is_doe_error_asserted())
  {
    TEEIO_ASSERT(false);
  }
  libspdm_return_t status = pci_doe_init_request();
  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "pci_doe_init_request - %x\n", (uint32_t)status));
    TEEIO_ASSERT(false);
  }

  context->doe_context = m_pci_doe_context;

  // init spdm_context
  void *spdm_context = spdm_client_init();
  TEEIO_ASSERT(spdm_context != NULL);

  uint32_t session_id = 0;
  ret = spdm_connect(spdm_context, &session_id);
  TEEIO_ASSERT(ret);

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
  // close ports
  close_dev_port(&context->lower_port, top->type);

  if(top->connection == IDE_TEST_CONNECT_DIRECT || top->connection == IDE_TEST_CONNECT_SWITCH) {
    close_host_port(context);
  } else if(top->connection == IDE_TEST_CONNECT_P2P ){
    // close both root_port and upper_port
    close_both_root_upper_port(context);
  }

  return true;
}

bool test_group_setup_sel(void *test_context)
{
  return common_test_group_setup(test_context);
}

bool test_group_teardown_sel(void *test_context)
{
  return common_test_group_teardown(test_context);
}

// link_ide test group
bool test_group_setup_link(void *test_context)
{
  return common_test_group_setup(test_context);
}

bool test_group_teardown_link(void *test_context)
{
  return common_test_group_teardown(test_context);
}

// selective_and_link_ide test group
bool test_group_setup_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return true;
}

bool test_group_teardown_sel_link(void *test_context)
{
  NOT_IMPLEMENTED("selective_and_link_ide topology");
  return true;
}