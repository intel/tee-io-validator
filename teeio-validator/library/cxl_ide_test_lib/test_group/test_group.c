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
// #include "library/cxl_ide_km_common_lib.h"
#include "library/cxl_ide_km_requester_lib.h"
#include "ide_test.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "cxl_ide_lib.h"
#include "pcie_ide_lib.h"
#include "teeio_spdmlib.h"

bool cxl_scan_devices(void *test_context)
{
  bool ret = false;
  cxl_ide_test_group_context_t *context = (cxl_ide_test_group_context_t *)test_context;
  TEEIO_ASSERT(context->signature == GROUP_CONTEXT_SIGNATURE);

  IDE_TEST_TOPOLOGY *top = context->top;

  TEEIO_ASSERT(context->suite_context->test_category == TEEIO_TEST_CATEGORY_CXL_IDE);
  TEEIO_ASSERT(top->connection == IDE_TEST_CONNECT_DIRECT || top->connection == IDE_TEST_CONNECT_SWITCH);

  ret = scan_devices_at_bus(context->root_port.port, context->lower_port.port, context->sw_conn1, context->top->bus);
  if(ret) {
    context->upper_port.port->bus = context->root_port.port->bus;
    strncpy(context->upper_port.port->bdf, context->root_port.port->bdf, BDF_LENGTH);
  }

  return ret;
}

// CXL Spec 3.1 Section 8.2.4.22
// CXL IDE Capability Structure
bool cxl_check_device_ide_reg_block(uint32_t* ide_reg_block, uint32_t ide_reg_block_count)
{
  int i;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "ide_reg_block:\n"));
  for (i = 0; i < ide_reg_block_count; i++)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "ide_reg_block %04x: 0x%08x\n", i, ide_reg_block[i]));
  }

  // Section 8.2.4.22.1
  // check CXL IDE Capability at offset 00h
  TEEIO_ASSERT(ide_reg_block_count > 1);
  CXL_IDE_CAPABILITY ide_cap = {.raw = ide_reg_block[0]};
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "CXL IDE Capability : 0x%08x\n", ide_cap.raw));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    cxl_ide_capable=0x%x, cxl_ide_modes=0x%02x\n",
                                  ide_cap.cxl_ide_capable, ide_cap.supported_cxl_ide_modes));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "    supported_algo=0x%02x, ide_stop_capable=%d, lopt_ide_capable=%d\n",
                                  ide_cap.supported_algo, ide_cap.ide_stop_capable,
                                  ide_cap.lopt_ide_capable));

  if(ide_cap.cxl_ide_capable == 0) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "CXL IDE is not supported by device.\n"));
    return false;
  }

  return true;
}

bool cxl_ide_query(cxl_ide_test_group_context_t *group_context)
{
  libspdm_return_t status;

  CXL_QUERY_RESP_CAPS caps;
  uint8_t bus_num;
  uint8_t segment;
  uint8_t dev_func_num;
  uint8_t max_port_index;
  uint32_t ide_reg_block[CXL_IDE_KM_IDE_CAP_REG_BLOCK_MAX_COUNT] = {0};
  uint32_t ide_reg_block_count;

  // query
  caps.raw = 0;
  ide_reg_block_count = CXL_IDE_KM_IDE_CAP_REG_BLOCK_MAX_COUNT;
  status = cxl_ide_km_query(group_context->doe_context,
                            group_context->spdm_context,
                            &group_context->session_id,
                            0, // port_index
                            &dev_func_num,
                            &bus_num,
                            &segment,
                            &max_port_index,
                            &caps.raw,
                            ide_reg_block,
                            &ide_reg_block_count);
  if (LIBSPDM_STATUS_IS_ERROR(status))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "cxl_ide_km_query failed with status=0x%x\n", status));
    return false;
  }
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "CXL_QUERY_RESP:\n"));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  max_port_index - 0x%02x\n", max_port_index));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  caps - 0x%02x\n", caps.raw));
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "         cap_version=0x%01x, iv_gen_capable=0x%01x, key_gen_capable=0x%01x, k_set_stop_capable=0x%01x\n",
                                            caps.cxl_ide_cap_version,
                                            caps.iv_generation_capable,
                                            caps.ide_key_generation_capable,
                                            caps.k_set_stop_capable));

  if(!cxl_check_device_ide_reg_block(ide_reg_block, ide_reg_block_count)) {
    return false;
  }

  // check CXL IDE Capability at offset 00h
  TEEIO_ASSERT(ide_reg_block_count > 1);
  CXL_IDE_CAPABILITY ide_cap = {.raw = ide_reg_block[0]};
  CXL_PRIV_DATA *cxl_data = &group_context->lower_port.cxl_data;
  cxl_data->memcache.ide_cap.raw = ide_cap.raw;
  cxl_data->query_resp.bus_num = bus_num;
  cxl_data->query_resp.dev_func_num = dev_func_num;
  cxl_data->query_resp.segment = segment;
  cxl_data->query_resp.max_port_index = max_port_index;
  cxl_data->query_resp.caps = caps.raw;

  return true;
}

// link_ide test group

/**
 * This function works to setup link_ide
 *
 * 1. open cofiguration_space and find the ecap_offset
 * 2. map kcbar to user space
 * 3. initialize spdm_context and doe_context
 * 4. setup spdm_session
 */
static bool common_test_group_setup(void *test_context)
{
  bool ret = false;

  cxl_ide_test_group_context_t *context = (cxl_ide_test_group_context_t *)test_context;
  TEEIO_ASSERT(context->signature == GROUP_CONTEXT_SIGNATURE);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "test_group_setup start\n"));

  // first scan devices
  if(!cxl_scan_devices(test_context)) {
    TEEIO_ASSERT(false);
    return false;
  }

  // initialize lower_port
  ret = cxl_init_dev_port(context);
  if(!ret) {
    TEEIO_ASSERT(false);
    return false;
  }

  IDE_TEST_TOPOLOGY *top = context->top;
  TEEIO_ASSERT(top->connection == IDE_TEST_CONNECT_DIRECT || top->connection == IDE_TEST_CONNECT_SWITCH);

  ret = cxl_init_root_port(context);
  if (!ret) {
    TEEIO_ASSERT(false);
    return false;
  }

  // init spdm_context
  void *spdm_context = spdm_client_init();
  TEEIO_ASSERT(spdm_context != NULL);

  uint32_t session_id = 0;
  ret = spdm_connect(spdm_context, &session_id);
  if (!ret) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "spdm_connect failed.\n"));
    TEEIO_ASSERT(false);
    return false;
  }

  context->spdm_context = spdm_context;
  context->session_id = session_id;

  // cxl query is called in group_setup
  if(!cxl_ide_query(context)) {
    return false;
  }

  // stream id in cxl-ide is always 0 according to cxl-spec
  context->stream_id = 0;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "test_group_setup done\n"));

  return true;
}

static bool common_test_group_teardown(void *test_context)
{
  cxl_ide_test_group_context_t *context = (cxl_ide_test_group_context_t *)test_context;
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
