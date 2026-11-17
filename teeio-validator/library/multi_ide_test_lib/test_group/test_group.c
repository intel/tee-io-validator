/**
 *  Copyright Notice:
 *  Copyright 2026 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hal/base.h"
#include "hal/library/debuglib.h"

#include "library/spdm_requester_lib.h"
#include "ide_test.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "teeio_spdmlib.h"
#include "pcie_ide_lib.h"
#include "pcie_ide_test_lib.h"
#include "cxl_ide_test_lib.h"
#include "multi_ide_test_internal.h"

static bool multi_ide_query_port_index(multi_ide_test_group_context_t *group_context)
{
  ide_common_test_port_context_t *lower_port = &group_context->common.lower_port;
  uint8_t pcie_port_index = 0;
  uint8_t cxl_port_index = 0;

  if(multi_ide_has_stream_type(group_context, false) &&
     !pcie_ide_query_port_index(&group_context->spdm_doe, lower_port->port, &pcie_port_index)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Query IDE_KM port_index failed.\n"));
    return false;
  }

  if(multi_ide_has_stream_type(group_context, true)) {
    if(!cxl_ide_query_dev_port_index(&group_context->spdm_doe, lower_port->port, &cxl_port_index)) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Query CXL_IDE_KM port_index failed.\n"));
      return false;
    }
    if(!cxl_ide_query_dev(&group_context->spdm_doe, cxl_port_index, &lower_port->cxl_data)) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "CXL Query failed.\n"));
      return false;
    }
  }

  for(int i = 0; i < group_context->streams_cnt; i++) {
    multi_ide_stream_t *stream = &group_context->streams[i];
    stream->port_index = stream->type == IDE_TEST_STREAM_TYPE_CXL_CM_IDE ? cxl_port_index : pcie_port_index;
  }

  return true;
}

// check that the rootport and the device have enough free resources for all PCIE/CXL.io streams
static bool multi_ide_check_pcie_resources(multi_ide_test_group_context_t *group_context)
{
  ide_common_test_port_context_t *upper_port = &group_context->common.upper_port;
  ide_common_test_port_context_t *lower_port = &group_context->common.lower_port;
  int pcie_streams_cnt = 0;
  uint8_t rp_stream_index = 0;
  uint8_t ide_id = 0;

  for(int i = 0; i < group_context->streams_cnt; i++) {
    multi_ide_stream_t *stream = &group_context->streams[i];
    if(stream->type == IDE_TEST_STREAM_TYPE_CXL_CM_IDE) {
      continue;
    }
    pcie_streams_cnt++;

    if(check_stream_id_used_in_rootport(stream->stream_id, upper_port)) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "stream_id (%d) is already used.\n", stream->stream_id));
      return false;
    }

    IDE_TEST_TOPOLOGY_TYPE top_type = multi_ide_get_stream_top_type(stream);
    if(!find_free_rp_stream_index_and_ide_id(upper_port, &rp_stream_index, &ide_id, top_type, true) ||
       !find_free_rp_stream_index_and_ide_id(lower_port, NULL, &ide_id, top_type, false)) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "No free register block for %s stream.\n", IDE_TEST_STREAM_TYPE_NAMES[stream->type]));
      return false;
    }
  }

  if(pcie_streams_cnt == 0) {
    return true;
  }

  int free_rp_streams = pcie_ide_get_free_rp_stream_num(upper_port);
  int free_slot_groups = pcie_ide_get_free_slot_group_num(upper_port);
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "PCIE/CXL.io streams=%d, free rootport stream_x=%d, free key/iv slot groups=%d\n",
               pcie_streams_cnt, free_rp_streams, free_slot_groups));
  if(free_rp_streams < pcie_streams_cnt || free_slot_groups < pcie_streams_cnt) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Not enough free stream_x or key/iv slots in rootport KCBar.\n"));
    return false;
  }

  return true;
}

/**
 * Open the ports and one SPDM session which is shared by all streams in Topology_x.
 * The streams themselves are set up in the test cases.
 */
bool multi_ide_test_group_setup(void *test_context)
{
  multi_ide_test_group_context_t *group_context = (multi_ide_test_group_context_t *)test_context;
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  IDE_TEST_TOPOLOGY *top = group_context->common.top;
  ide_common_test_port_context_t *upper_port = &group_context->common.upper_port;
  const char *err_msg = NULL;

  if(top->streams_cnt == 0 || top->connection != IDE_TEST_CONNECT_DIRECT) {
    teeio_record_group_result(TEEIO_TEST_GROUP_FUNC_SETUP, TEEIO_TEST_RESULT_FAILED, "streams is missing or connection is not direct.");
    return false;
  }

  group_context->streams_cnt = top->streams_cnt;
  for(int i = 0; i < top->streams_cnt; i++) {
    memset(&group_context->streams[i], 0, sizeof(multi_ide_stream_t));
    group_context->streams[i].type = top->streams[i].type;
    group_context->streams[i].stream_id = top->streams[i].stream_id;
    group_context->streams[i].state = MULTI_IDE_STREAM_STATE_IDLE;
  }

  if(!multi_ide_open_ports(group_context)) {
    err_msg = "Open ports failed.";
    goto GroupSetupFail;
  }

  for(int i = 0; i < group_context->streams_cnt; i++) {
    multi_ide_stream_t *stream = &group_context->streams[i];
    stream->kcbar = stream->type == IDE_TEST_STREAM_TYPE_CXL_CM_IDE ? upper_port->mapped_cxl_kcbar_addr : upper_port->mapped_kcbar_addr;
  }

  if(!teeio_spdm_session_open(&group_context->spdm_doe.spdm_context, &group_context->spdm_doe.session_id)) {
    err_msg = "Spdm connect failed.";
    goto GroupSetupFail;
  }

  if(!multi_ide_query_port_index(group_context)) {
    err_msg = "Query port_index failed.";
    goto GroupSetupFail;
  }

  if(!multi_ide_check_pcie_resources(group_context)) {
    err_msg = "Not enough free IDE resources.";
    goto GroupSetupFail;
  }

  teeio_record_group_result(TEEIO_TEST_GROUP_FUNC_SETUP, TEEIO_TEST_RESULT_PASS, "");
  return true;

GroupSetupFail:
  // group teardown is not called if group setup fails
  teeio_spdm_session_close(&group_context->spdm_doe.spdm_context, &group_context->spdm_doe.session_id);
  multi_ide_close_ports(group_context);
  teeio_record_group_result(TEEIO_TEST_GROUP_FUNC_SETUP, TEEIO_TEST_RESULT_FAILED, "%s", err_msg);
  return false;
}

bool multi_ide_test_group_teardown(void *test_context)
{
  multi_ide_test_group_context_t *group_context = (multi_ide_test_group_context_t *)test_context;
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  // streams are torn down in the test cases. This only catches streams which are left.
  multi_ide_teardown_all_streams(group_context);

  teeio_spdm_session_close(&group_context->spdm_doe.spdm_context, &group_context->spdm_doe.session_id);
  multi_ide_close_ports(group_context);

  teeio_record_group_result(TEEIO_TEST_GROUP_FUNC_TEARDOWN, TEEIO_TEST_RESULT_PASS, "");

  return true;
}
