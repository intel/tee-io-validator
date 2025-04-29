/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "ide_test.h"
#include "teeio_debug.h"

#include "library/spdm_requester_lib.h"
#include "hal/library/memlib.h"
#include "helperlib.h"
#include "cxl_ide_lib.h"
#include "cxl_ide_test_common.h"
#include "cxl_ide_test_internal.h"

static uint8_t mSubStream[0x100] = {0};

char* dump_binary_to_str(uint8_t data, char* str, int str_size) {
  TEEIO_ASSERT(str_size >= 6);
  sprintf(str, "0b");
  for (int i = 3; i >= 0; i--) {
      sprintf(str + 2 + 3 - i, "%d", (data >> i) & 1);
  }

  return str;
}


static int construct_test_sub_stream()
{
  int sub_stream_cnt = 0;

  for(uint8_t sub_stream = 0; sub_stream < 0b10000; sub_stream++) {
    if(sub_stream == 0b1000) {
      continue;
    }

    if(is_power_of_two(sub_stream) || sub_stream == 0b0000 || sub_stream == 0b0111 || sub_stream == 0b1001 || sub_stream == 0b1111) {
      mSubStream[sub_stream_cnt++] = sub_stream;
    }
  }

  char binary_str[8] = {0};
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "construct test sub_stream list. Total %d sub_stream.\n", sub_stream_cnt));
  for(int i = 0; i < sub_stream_cnt; i++) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "  mSubStream[%d] = %s\n", i, dump_binary_to_str(mSubStream[i], binary_str, 8)));
  }

  return sub_stream_cnt;
}

bool cxl_ide_test_key_prog_5_setup(void *test_context)
{
  // Cxl.Query has been called in test_group.setup()
  return true;
}

void cxl_ide_test_key_prog_5_run(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_run_test_case_t* test_case = case_context->test_case;
  TEEIO_ASSERT(test_case);
  int case_class = test_case->class_id;
  int case_id = test_case->case_id;

  cxl_ide_test_group_context_t *group_context = (cxl_ide_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  IDE_TEST_CONFIGURATION *configuration = get_configuration_by_id(group_context->common.suite_context->test_config, group_context->common.config_id);
  TEEIO_ASSERT(configuration);

  ide_common_test_port_context_t *lower_port = &group_context->common.lower_port;

  int max_port_index = lower_port->cxl_data.query_resp.max_port_index;
  char case_info[MAX_LINE_LENGTH] = {0};
  int sub_stream_cnt = construct_test_sub_stream();

  for(int port_index = 0; port_index <= max_port_index; port_index++) {
    for(int i = 0; i < sub_stream_cnt; i++) {
      uint8_t sub_stream = mSubStream[i];
      sprintf(case_info, "  port_index = 0x%02x sub_stream=0x%02x", port_index, sub_stream);
      test_cxl_ide_key_prog(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context,
                            &group_context->spdm_doe.session_id, 0, (sub_stream << 4) & 0xf0,
                            port_index, false, case_info,
                            case_class, case_id, test_cxl_ide_key_prog_invalid_params);
    }
  }
}

void cxl_ide_test_key_prog_5_teardown(void *test_context)
{
}
