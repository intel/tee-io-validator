/**
 *  Copyright Notice:
 *  Copyright 2026 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>

#include "hal/base.h"
#include "hal/library/debuglib.h"

#include "ide_test.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "multi_ide_test_internal.h"

static multi_ide_test_group_context_t *get_group_context(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  multi_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  return group_context;
}

// record one assertion per stream which checks that the stream is active
static void check_streams_active(multi_ide_test_group_context_t *group_context, int case_class, int case_id,
                                 int assertion_id, const char *when)
{
  for(int i = 0; i < group_context->streams_cnt; i++) {
    multi_ide_stream_t *stream = &group_context->streams[i];
    bool active = multi_ide_is_stream_active(group_context, stream);
    teeio_record_assertion_result(case_class, case_id, assertion_id + i,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  active ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "%s:%d stream is %s %s.", IDE_TEST_STREAM_TYPE_NAMES[stream->type],
                                  stream->stream_id, active ? "active" : "not active", when);
  }
}

// Set up all streams in the order of the streams list
bool multi_ide_test_multi_stream_1_setup(void *test_context)
{
  return multi_ide_setup_all_streams(get_group_context(test_context));
}

// Check that all streams are active, wait for traffic and check again
void multi_ide_test_multi_stream_1_run(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  multi_ide_test_group_context_t *group_context = get_group_context(test_context);
  int case_class = case_context->test_case->class_id;
  int case_id = case_context->test_case->case_id;

  check_streams_active(group_context, case_class, case_id, 1, "after setup");

  for(int i = 0; i < group_context->streams_cnt; i++) {
    multi_ide_dump_stream(group_context, &group_context->streams[i]);
  }

  TEEIO_PRINT(("All IDE streams are set up. Run traffic and press any key to continue.\n"));
  getchar();

  check_streams_active(group_context, case_class, case_id, group_context->streams_cnt + 1, "after traffic");
}

// Tear down all streams in the order of the streams list
void multi_ide_test_multi_stream_1_teardown(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  multi_ide_test_group_context_t *group_context = get_group_context(test_context);
  int assertion_id = 2 * group_context->streams_cnt + 1;

  for(int i = 0; i < group_context->streams_cnt; i++) {
    multi_ide_stream_t *stream = &group_context->streams[i];
    bool active = stream->state == MULTI_IDE_STREAM_STATE_ACTIVE;
    bool insecure = multi_ide_teardown_stream(group_context, stream);

    if(active && case_context->action != IDE_COMMON_TEST_ACTION_SKIP) {
      teeio_record_assertion_result(case_context->test_case->class_id, case_context->test_case->case_id, assertion_id + i,
                                    IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                    insecure ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                    "%s:%d stream is %s after teardown.", IDE_TEST_STREAM_TYPE_NAMES[stream->type],
                                    stream->stream_id, insecure ? "in Insecure State" : "not in Insecure State");
    }
  }
}

// Set up all streams in the order of the streams list. At least 2 streams are needed.
bool multi_ide_test_multi_stream_2_setup(void *test_context)
{
  multi_ide_test_group_context_t *group_context = get_group_context(test_context);

  if(group_context->streams_cnt < 2) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "MultiStream.2 needs at least 2 streams in the streams list.\n"));
    return false;
  }

  return multi_ide_setup_all_streams(group_context);
}

// Tear down the streams one by one in list order and check that the other streams stay active
void multi_ide_test_multi_stream_2_run(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  multi_ide_test_group_context_t *group_context = get_group_context(test_context);
  int case_class = case_context->test_case->class_id;
  int case_id = case_context->test_case->case_id;
  int assertion_id = group_context->streams_cnt + 1;

  check_streams_active(group_context, case_class, case_id, 1, "after setup");

  for(int i = 0; i < group_context->streams_cnt; i++) {
    multi_ide_stream_t *stream = &group_context->streams[i];
    bool insecure = multi_ide_teardown_stream(group_context, stream);
    teeio_record_assertion_result(case_class, case_id, assertion_id++,
                                  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                  insecure ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                  "%s:%d stream is %s after teardown.", IDE_TEST_STREAM_TYPE_NAMES[stream->type],
                                  stream->stream_id, insecure ? "in Insecure State" : "not in Insecure State");

    if(i == 0) {
      TEEIO_PRINT(("%s:%d stream is torn down. Run traffic on the other streams and press any key to continue.\n",
                   IDE_TEST_STREAM_TYPE_NAMES[stream->type], stream->stream_id));
      getchar();
    }

    for(int j = i + 1; j < group_context->streams_cnt; j++) {
      multi_ide_stream_t *other = &group_context->streams[j];
      bool active = multi_ide_is_stream_active(group_context, other);
      teeio_record_assertion_result(case_class, case_id, assertion_id++,
                                    IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
                                    active ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED,
                                    "%s:%d stream is %s after %s:%d stream is torn down.",
                                    IDE_TEST_STREAM_TYPE_NAMES[other->type], other->stream_id,
                                    active ? "active" : "not active",
                                    IDE_TEST_STREAM_TYPE_NAMES[stream->type], stream->stream_id);
    }
  }
}

// Tear down the streams which are left if setup or run did not complete
void multi_ide_test_multi_stream_2_teardown(void *test_context)
{
  multi_ide_teardown_all_streams(get_group_context(test_context));
}
