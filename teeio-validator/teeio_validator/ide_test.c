/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 */

#include "teeio_validator.h"

#include <stdlib.h>
#include <ctype.h>
#include "helperlib.h"
#include "ide_test.h"
#include "pcie_ide_test_lib.h"
#include "test_factory.h"

extern const char *TEEIO_TEST_CATEGORY_NAMES[];

void append_config_item(ide_run_test_config_item_t **head, ide_run_test_config_item_t* new)
{
  TEEIO_ASSERT(head != NULL && new != NULL);
  if (*head == NULL)  {
    *head = new;
  } else {
    ide_run_test_config_item_t *walk = *head;
    while (walk->next) {
      walk = walk->next;
    }
    walk->next = new;
  }
}

const char *m_ide_test_case_name[] = {
    "Query",
    "KeyProg",
    "KSetGo",
    "KSetStop",
    "SpdmSession",
    "Test",
    NULL};

const char *m_ide_test_configuration_name[] = {
  "Default",
  "Switch",
  "PartialHeaderEncryption",
  "PCRC",
  "Aggregation",
  "SelectiveIDEForConfiguration",
  "TeeLimitedStream",
  NULL
};

const char *m_ide_test_topology_name[] = {
  "SelectiveIDE",
  "LinkIDE",
  "SelectiveAndLinkIDE"
};

const char* m_test_case_result_str[] = {
  "skip", "pass", "fail"
};

const char* m_test_config_result_str[] = {
  "na", "pass", "fail"
};

ide_run_test_suite_t *alloc_run_test_suite(IDE_TEST_SUITE *suite, IDE_TEST_CONFIG *test_config)
{
  ide_run_test_suite_t *rts = (ide_run_test_suite_t *)malloc(sizeof(ide_run_test_suite_t));
  TEEIO_ASSERT(rts != NULL);
  memset(rts, 0, sizeof(ide_run_test_suite_t));
  sprintf(rts->name, "TestSuite_%d", suite->id);

  ide_common_test_suite_context_t *context = (ide_common_test_suite_context_t *)malloc(sizeof(ide_common_test_suite_context_t));
  TEEIO_ASSERT(context);
  memset(context, 0, sizeof(ide_common_test_suite_context_t));

  context->signature = SUITE_CONTEXT_SIGNATURE;
  context->test_suite_id = suite->id;
  context->test_config = test_config;
  context->test_category = suite->test_category;

  rts->test_context = context;

  return rts;
}

bool alloc_run_test_config_item(ide_run_test_config_t *rtc, int config_type, IDE_TEST_TOPOLOGY_TYPE top_type, TEEIO_TEST_CATEGORY test_category)
{
  TEEIO_ASSERT(top_type < IDE_TEST_TOPOLOGY_TYPE_NUM);
  TEEIO_ASSERT(config_type < IDE_TEST_CONFIGURATION_TYPE_NUM);

  ide_test_config_funcs_t *config_func = test_factory_get_test_config_funcs(test_category, top_type, config_type);

  ide_run_test_config_item_t *config_item = (ide_run_test_config_item_t*)malloc(sizeof(ide_run_test_config_item_t));
  TEEIO_ASSERT(config_item != NULL);
  memset(config_item, 0, sizeof(ide_run_test_config_item_t));
  config_item->type = config_type;

  // assign the functions
  config_item->check_func = config_func->check;
  config_item->disable_func = config_func->disable;
  config_item->enable_func = config_func->enable;
  config_item->support_func = config_func->support;

  append_config_item(&rtc->config_item, config_item);

  return true;
}

/**
 * allocate run_test_configs based on configuration indicated by @config_id
 * run_test_configs is attached in a run_test_suite.
*/
bool alloc_run_test_config(ide_run_test_suite_t *rts, IDE_TEST_CONFIG *test_config, int top_id, int config_id)
{
  IDE_TEST_CONFIGURATION *configuration = get_configuration_by_id(test_config, config_id);
  TEEIO_ASSERT(configuration);
  TEEIO_ASSERT(configuration->enabled);
  TEEIO_ASSERT(configuration->bit_map != 0);
  TEEIO_ASSERT(configuration->type < IDE_TEST_TOPOLOGY_TYPE_NUM);

  IDE_TEST_TOPOLOGY *top = get_topology_by_id(test_config, top_id);
  TEEIO_ASSERT(top);
  TEEIO_ASSERT(top->enabled);
  TEEIO_ASSERT(top->type == configuration->type);

  ide_run_test_config_t *run_test_config = (ide_run_test_config_t *)malloc(sizeof(ide_run_test_config_t));
  TEEIO_ASSERT(run_test_config);
  memset(run_test_config, 0, sizeof(ide_run_test_config_t));

  // assign test_config_context
  ide_common_test_config_context_t *context = (ide_common_test_config_context_t *)malloc(sizeof(ide_common_test_config_context_t));
  TEEIO_ASSERT(context);
  memset(context, 0, sizeof(ide_common_test_config_context_t));
  context->group_context = NULL;  // this is assigned in run-time
  context->suite_context = rts->test_context;
  context->signature = CONFIG_CONTEXT_SIGNATURE;
  context->top_type = top->type;
  run_test_config->test_context = context;

  TEEIO_TEST_CATEGORY test_category = context->suite_context->test_category;

  // config_item
  int config_type_num = 0;
  uint32_t config_bitmask = test_factory_get_config_bitmask(&config_type_num, top->type, test_category);
  TEEIO_ASSERT(config_bitmask != 0);

  uint32_t config_bits = configuration->bit_map & config_bitmask;
  char name_buf[MAX_NAME_LENGTH] = {0};
  int offset = 0;
  for(int i = 0; i < config_type_num; i++) {
    if(config_bits & BIT_MASK(i)) {
      alloc_run_test_config_item(run_test_config, i, top->type, test_category);
      TEEIO_ASSERT(offset + strlen(m_ide_test_configuration_name[i]) + 1 < MAX_NAME_LENGTH);
      sprintf(name_buf + offset, "%s+", m_ide_test_configuration_name[i]);
      offset = strlen(name_buf);
    }
  }

  TEEIO_ASSERT(offset > 1);
  // strip the last '+'
  name_buf[offset - 1] = '\0';
  sprintf(run_test_config->name, "%s", name_buf);

  // insert into run_test_suite
  ide_run_test_config_t *itr = rts->test_config;
  if (itr == NULL)
  {
    rts->test_config = run_test_config;
  }
  else
  {
    while (itr->next)
    {
      itr = itr->next;
    }
    itr->next = run_test_config;
  }

  return true;
}

ide_common_test_switch_internal_conn_context_t* alloc_switch_internal_conn_context(IDE_TEST_CONFIG* test_config, IDE_TEST_TOPOLOGY* top, IDE_SWITCH_INTERNAL_CONNECTION *conn)
{
  ide_common_test_switch_internal_conn_context_t* conn_context = NULL;
  ide_common_test_switch_internal_conn_context_t* conn_header = NULL;

  while(conn != NULL) {
    conn_context = (ide_common_test_switch_internal_conn_context_t*)malloc(sizeof(ide_common_test_switch_internal_conn_context_t));
    TEEIO_ASSERT(conn_context);
    memset(conn_context, 0, sizeof(ide_common_test_switch_internal_conn_context_t));

    IDE_SWITCH* sw = get_switch_by_id(test_config, conn->switch_id);
    TEEIO_ASSERT(sw);
    IDE_PORT* ups_port = get_port_from_switch_by_id(sw, conn->ups_port);
    IDE_PORT* dps_port = get_port_from_switch_by_id(sw, conn->dps_port);
    TEEIO_ASSERT(ups_port && dps_port);

    conn_context->switch_id = conn->switch_id;
    conn_context->ups.port = ups_port;
    conn_context->dps.port = dps_port;
    
    if(conn_header == NULL) {
      conn_header = conn_context;
    } else {
      ide_common_test_switch_internal_conn_context_t* itr = conn_header;
      while(itr->next) {
        itr = itr->next;
      }
      itr->next = conn_context;
    }

    conn = conn->next;
  }

  return conn_header;
}

// alloc run_test_group data.
// After the call of this funciton, the data is inserted into the run_test_suite
ide_run_test_group_t *alloc_run_test_group(ide_run_test_suite_t *rts, IDE_TEST_CONFIG *test_config, int top_id, int case_id)
{
  ide_run_test_group_t *run_test_group = (ide_run_test_group_t *)malloc(sizeof(ide_run_test_group_t));
  TEEIO_ASSERT(run_test_group);
  memset(run_test_group, 0, sizeof(ide_run_test_group_t));

  ide_common_test_suite_context_t* suite_context = (ide_common_test_suite_context_t *)rts->test_context;
  TEEIO_ASSERT(suite_context->signature == SUITE_CONTEXT_SIGNATURE);

  IDE_TEST_TOPOLOGY *top = get_topology_by_id(test_config, top_id);
  TEEIO_ASSERT(top != NULL);
  TEEIO_ASSERT(top->type < IDE_TEST_TOPOLOGY_TYPE_NUM);

  IDE_PORT *root_port = get_port_by_id(test_config, top->root_port);
  IDE_PORT *upper_port = get_port_by_id(test_config, top->upper_port);
  IDE_PORT *lower_port = get_port_by_id(test_config, top->lower_port);
  TEEIO_ASSERT(root_port != NULL);
  TEEIO_ASSERT(upper_port != NULL);
  TEEIO_ASSERT(lower_port != NULL);

  sprintf(run_test_group->name, "%s", m_ide_test_topology_name[top->type]);

  ide_test_group_funcs_t *group_funcs = test_factory_get_test_group_funcs(suite_context->test_category, top->type);
  run_test_group->setup_func = group_funcs->setup;
  run_test_group->teardown_func = group_funcs->teardown;

  ide_common_test_group_context_t *context = (ide_common_test_group_context_t *)malloc(sizeof(ide_common_test_group_context_t));
  TEEIO_ASSERT(context);
  memset(context, 0, sizeof(ide_common_test_group_context_t));

  context->signature = GROUP_CONTEXT_SIGNATURE;
  context->suite_context = rts->test_context;
  context->top = top;
  context->upper_port.port = upper_port;
  context->lower_port.port = lower_port;
  context->root_port.port = root_port;

  // via switch
  if(top->connection == IDE_TEST_CONNECT_SWITCH || top->connection == IDE_TEST_CONNECT_P2P) {
    context->sw_conn1 = alloc_switch_internal_conn_context(test_config, top, top->sw_conn1);
  }
  if(top->connection == IDE_TEST_CONNECT_P2P) {
    context->sw_conn2 = alloc_switch_internal_conn_context(test_config, top, top->sw_conn2);
  }

  run_test_group->test_context = context;

  // insert into run_test_suite
  ide_run_test_group_t *itr = rts->test_group;
  if (itr == NULL)
  {
    rts->test_group = run_test_group;
  }
  else
  {
    while (itr->next)
    {
      itr = itr->next;
    }
    itr->next = run_test_group;
  }

  return run_test_group;
}

/**
 * allocate run_test_case. After that it is insert into @run_test_group
*/
bool alloc_run_test_case(ide_run_test_group_t *run_test_group, IDE_COMMON_TEST_CASE test_case, uint32_t case_id, TEEIO_TEST_CATEGORY test_category)
{
  TEEIO_ASSERT(test_case < IDE_COMMON_TEST_CASE_NUM);
  TEEIO_ASSERT(case_id <= MAX_CASE_ID);

  ide_run_test_case_t *run_test_case = (ide_run_test_case_t *)malloc(sizeof(ide_run_test_case_t));
  TEEIO_ASSERT(run_test_case != NULL);
  memset(run_test_case, 0, sizeof(ide_run_test_case_t));

  strncpy(run_test_case->class, m_ide_test_case_name[(int)test_case], MAX_NAME_LENGTH);
  sprintf(run_test_case->name, "%s.%d", m_ide_test_case_name[(int)test_case], case_id);
  // run_test_case->action = IDE_COMMON_TEST_ACTION_RUN;

  ide_test_case_funcs_t* case_funcs = test_factory_get_test_case_funcs(test_category, test_case, case_id);
  run_test_case->run_func = case_funcs->run;
  run_test_case->setup_func = case_funcs->setup;
  run_test_case->teardown_func = case_funcs->teardown;
  run_test_case->complete_ide_stream = case_funcs->complete_ide_stream;

  ide_common_test_case_context_t* context = (ide_common_test_case_context_t *)malloc(sizeof(ide_common_test_case_context_t));
  TEEIO_ASSERT(context);
  memset(context, 0, sizeof(ide_common_test_case_context_t));

  context->group_context = run_test_group->test_context;
  context->signature = CASE_CONTEXT_SIGNATURE;
  run_test_case->test_context = context;

  ide_run_test_case_t *itr = run_test_group->test_case;
  if (itr == NULL)
  {
    run_test_group->test_case = run_test_case;
  }
  else
  {
    while (itr->next)
    {
      itr = itr->next;
    }
    itr->next = run_test_case;
  }

  return true;
}

bool alloc_run_test_cases(
    ide_run_test_suite_t *run_test_suite,
    IDE_TEST_CONFIG *test_config,
    IDE_TEST_SUITE *suite,
    IDE_TEST_TOPOLOGY *top)
{
  ide_run_test_group_t *run_test_group = NULL;
  ide_common_test_suite_context_t* suite_context = run_test_suite->test_context;
  TEEIO_ASSERT(suite_context->signature == SUITE_CONTEXT_SIGNATURE);

  for(int i = 0; i < IDE_COMMON_TEST_CASE_NUM; i++) {
    IDE_TEST_CASE *tc = suite->test_cases.cases + i;
    if(tc->cases_cnt == 0) {
      continue;
    }

    run_test_group = alloc_run_test_group(run_test_suite, test_config, top->id, i);
    TEEIO_ASSERT(run_test_group);

    for (int j = 0; j < tc->cases_cnt; j++)
    {
      alloc_run_test_case(run_test_group, i, tc->cases_id[j], suite_context->test_category);
    }
  }

  return true;
}

/**
 * Prepare the test data. It returns a ide_run_test_suite_t chain.
 */
ide_run_test_suite_t *prepare_tests_data(IDE_TEST_CONFIG *test_config)
{
  IDE_TEST_SUITES *suites = &test_config->test_suites;
  ide_run_test_suite_t *run_test_suite = NULL;
  ide_run_test_suite_t *run_test_suite_header = run_test_suite;

  for (int i = 0; i < MAX_TEST_SUITE_NUM; i++)
  {
    IDE_TEST_SUITE *suite = suites->test_suites + i;
    if (suite->id == 0 || !suite->enabled)
    {
      continue;
    }

    IDE_TEST_TOPOLOGY *top = get_topology_by_id(test_config, suite->topology_id);
    if (top == NULL)
    {
      continue;
    }

    run_test_suite = alloc_run_test_suite(suite, test_config);
    TEEIO_ASSERT(run_test_suite);

    bool ret = alloc_run_test_config(run_test_suite, test_config, suite->topology_id, suite->configuration_id);
    TEEIO_ASSERT(ret);

    ret = alloc_run_test_cases(run_test_suite, test_config, suite, top);
    TEEIO_ASSERT(ret);

    // insert into run_test_suite chain
    ide_run_test_suite_t *itr = run_test_suite_header;
    if (itr == NULL)
    {
      run_test_suite_header = run_test_suite;
    }
    else
    {
      while (itr->next != NULL)
      {
        itr = itr->next;
      }
      itr->next = run_test_suite;
    }
  }

  return run_test_suite_header;
}

bool do_run_test_case(ide_run_test_case_t *test_case, ide_run_test_case_result_t *case_result)
{
  bool ret = true;
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_case->test_context;
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_common_test_group_context_t *group_context = (ide_common_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  TEEIO_PRINT(("     Run %s\n", test_case->name));

  if(case_context->action == IDE_COMMON_TEST_ACTION_SKIP)
  {
    // set the test result
    case_context->result = IDE_COMMON_TEST_CASE_RESULT_SKIPPED;
    TEEIO_PRINT(("       %s skipped.\n", test_case->name));
    return true;
  }

  if(test_case->setup_func != NULL) {
    ret = test_case->setup_func(test_case->test_context);
    if(!ret) {
      case_context->result = IDE_COMMON_TEST_CASE_RESULT_FAILED;
      TEEIO_PRINT(("       %s setup failed. So skipped.\n", test_case->name));
      goto TestCaseDone;
    }
  }

  // again check if the case to be skipped after setup
  if(case_context->action == IDE_COMMON_TEST_ACTION_SKIP)
  {
    // set the test result
    case_context->result = IDE_COMMON_TEST_CASE_RESULT_SKIPPED;
    TEEIO_PRINT(("       %s skipped after setup.\n", test_case->name));
    goto TestCaseDone;
  }

  if(ret && test_case->run_func != NULL) {
    ret = test_case->run_func(test_case->test_context);
  }

TestCaseDone:
  case_result->case_result = case_context->result;

  if(test_case->teardown_func != NULL) {
    ret = test_case->teardown_func(test_case->test_context);
    TEEIO_ASSERT(ret);
  }

  return ret;
}

ide_run_test_case_result_t *alloc_run_test_case_result(ide_run_test_group_result_t* group_result, const char* case_name, const char* case_class)
{
  ide_run_test_case_result_t *case_result = (ide_run_test_case_result_t *)malloc(sizeof(ide_run_test_case_result_t));
  TEEIO_ASSERT(case_result);
  memset(case_result, 0, sizeof(ide_run_test_case_result_t));

  strncpy(case_result->name, case_name, MAX_NAME_LENGTH);
  strncpy(case_result->class, case_class, MAX_NAME_LENGTH);

  ide_run_test_case_result_t* itr = group_result->case_result;
  if(itr == NULL) {
    group_result->case_result = case_result;
  } else {
    while(itr->next) {
      itr = itr->next;
    }
    itr->next = case_result;
  }

  return case_result;
}

static bool do_run_test_config_support(ide_run_test_config_t *run_test_config, IDE_TEST_TOPOLOGY_TYPE top_type, TEEIO_TEST_CATEGORY test_category)
{
  bool ret = false;

  ide_run_test_config_item_t* config_item = run_test_config->config_item;
  do {
    TEEIO_ASSERT(config_item->support_func);
    ret = config_item->support_func(run_test_config->test_context);

    config_item = config_item->next;
  } while(ret && config_item);

  return ret;
}

static bool do_run_test_config_enable(ide_run_test_config_t *run_test_config, IDE_TEST_TOPOLOGY_TYPE top_type, TEEIO_TEST_CATEGORY test_category)
{
  bool ret = false;

  ide_run_test_config_item_t* config_item = run_test_config->config_item;
  do {
    TEEIO_ASSERT(config_item->enable_func);
    ret = config_item->enable_func(run_test_config->test_context);

    config_item = config_item->next;
  } while(ret && config_item);

  return ret;
}

static bool do_run_test_config_check(ide_run_test_config_t *run_test_config, IDE_TEST_TOPOLOGY_TYPE top_type, TEEIO_TEST_CATEGORY test_category)
{
  bool ret = false;

  ide_run_test_config_item_t* config_item = run_test_config->config_item;
  do {
    TEEIO_ASSERT(config_item->check_func);
    ret = config_item->check_func(run_test_config->test_context);

    config_item = config_item->next;
  } while(ret && config_item);

  return ret;
}

/**
 * run_test_group
*/
bool do_run_test_group(ide_run_test_group_t *run_test_group, ide_run_test_config_t *run_test_config, ide_run_test_group_result_t* group_result, TEEIO_TEST_CATEGORY test_category)
{
  bool ret = true;
  bool run_test_group_failed = false;
  bool run_test_config_failed = false;
  ide_common_test_group_context_t *group_context = (ide_common_test_group_context_t *)run_test_group->test_context;
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)run_test_config->test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);
  config_context->group_context = group_context;

  IDE_TEST_TOPOLOGY_TYPE top_type = group_context->top->type;

  if(run_test_group->test_case == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "test_case is not set in test_group.\n"));
    return true;
  }

  TEEIO_PRINT(("   Run TestGroup (%s %s %s)\n", run_test_group->name, run_test_config->name, run_test_group->test_case->class));

  // call run_test_group's setup function
  if (run_test_group->setup_func != NULL)
  {
    ret = run_test_group->setup_func(group_context);
    if(!ret) {
      config_context->test_result = IDE_COMMON_TEST_CONFIG_RESULT_FAILED;
      TEEIO_PRINT(("       %s failed at test_group->setup(). Skip the TestConfig.\n", run_test_config->name));
      run_test_group_failed = true;
    }
  }
  else
  {
    run_test_group_failed = true;
  }

  if(run_test_group_failed) {
    run_test_config_failed = true;
  } else {
    // check if the test_config is supported.
    // if not supported, this test_group is done.
    if(!do_run_test_config_support(run_test_config, top_type, test_category)) {
      config_context->test_result = IDE_COMMON_TEST_CONFIG_RESULT_NA;
      TEEIO_PRINT(("       %s is not supported. Skip the TestConfig.\n", run_test_config->name));
      run_test_config_failed = true;
    }
  }

  ide_run_test_case_t *test_case = run_test_group->test_case;
  while (test_case != NULL)
  {
    // alloc case_result
    ide_run_test_case_result_t *case_result = alloc_run_test_case_result(group_result, test_case->name, test_case->class);
    if(run_test_group_failed || run_test_config_failed) {
      case_result->case_result = IDE_COMMON_TEST_CASE_RESULT_FAILED;
      case_result->config_result = IDE_COMMON_TEST_CONFIG_RESULT_NA;
      goto OneCaseDone;
    }

    // call test_config's enable function
    ret = do_run_test_config_enable(run_test_config, top_type, test_category);
    TEEIO_ASSERT(ret);

    // run the test_case
    do_run_test_case(test_case, case_result);

    if(test_case->complete_ide_stream) {
      // check config
      ret = do_run_test_config_check(run_test_config, top_type, test_category);
      case_result->config_result = config_context->test_result;
      TEEIO_ASSERT(ret);
    }

OneCaseDone:
    // next case
    test_case = test_case->next;
  }

  // call run_test_group's teardown function
  if (run_test_group->teardown_func != NULL && !run_test_group_failed)
  {
    ret = run_test_group->teardown_func(group_context);
    TEEIO_ASSERT(ret);
  }

  // config_context is reused between different run_group_test
  // So its group_context must be cleared here.
  config_context->group_context = NULL;

  TEEIO_PRINT(("\n"));

  return true;
}

ide_run_test_group_result_t* alloc_run_test_group_result(ide_run_test_group_t *run_test_group, ide_run_test_config_t *run_test_config)
{
  ide_run_test_group_result_t* group_result = (ide_run_test_group_result_t *)malloc(sizeof(ide_run_test_group_result_t));
  TEEIO_ASSERT(group_result);
  memset(group_result, 0, sizeof(ide_run_test_group_result_t));
  strncpy(group_result->name, run_test_group->name, MAX_NAME_LENGTH);

  ide_run_test_group_result_t* itr = run_test_config->group_result;
  if(itr == NULL) {
    run_test_config->group_result = group_result;
  } else {
    while(itr->next) {
      itr = itr->next;
    }
    itr->next = group_result;
  }

  return group_result;
}

/**
 * run test suite
 */
bool do_run_test_suite(ide_run_test_suite_t *run_test_suite)
{
    ide_run_test_group_t *run_test_group = run_test_suite->test_group;
    ide_run_test_config_t *run_test_config = run_test_suite->test_config;
    run_test_config->group_result = NULL;

    ide_common_test_suite_context_t* suite_context = run_test_suite->test_context;
    TEEIO_ASSERT(suite_context->signature == SUITE_CONTEXT_SIGNATURE);

    TEEIO_PRINT((" Run %s\n", run_test_suite->name));

    while(run_test_config != NULL) {

      while (run_test_group != NULL)
      {
        // alloc group_result
        ide_run_test_group_result_t* group_result = alloc_run_test_group_result(run_test_group, run_test_config);
        TEEIO_ASSERT(group_result);

        do_run_test_group(run_test_group, run_test_config, group_result, suite_context->test_category);

        run_test_group = run_test_group->next;
      }
      run_test_config = run_test_config->next;
    }

    TEEIO_PRINT(("\n"));

    return true;
}

bool test_group_cases_statics(ide_run_test_group_result_t *group_result, int *passed, int *failed, int *skipped)
{
  *passed = 0;
  *failed = 0;
  *skipped = 0;

  if(group_result == NULL) {
    return false;
  }
  ide_run_test_case_result_t *case_result = group_result->case_result;

  while(case_result) {
    if(case_result->case_result == IDE_COMMON_TEST_CASE_RESULT_SKIPPED) {
      *skipped += 1;
    } else if(case_result->case_result == IDE_COMMON_TEST_CASE_RESULT_SUCCESS) {
      *passed += 1;
    } else if(case_result->case_result == IDE_COMMON_TEST_CASE_RESULT_FAILED) {
      *failed += 1;
    } else {
      TEEIO_ASSERT(false);
    }

    case_result = case_result->next;
  }

  return true;
}

bool test_suite_cases_statics(ide_run_test_suite_t *run_test_suite, int *passed, int *failed, int *skipped)
{
  if(run_test_suite == NULL) {
    return false;
  }
  *passed = 0;
  *failed = 0;
  *skipped = 0;

  int p,f,s;

  ide_run_test_config_t *run_test_config = run_test_suite->test_config;
  while(run_test_config) {

    ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)run_test_config->test_context;
    TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

    ide_run_test_group_result_t * group_result = run_test_config->group_result;

    while(group_result) {

      test_group_cases_statics(group_result, &p, &f, &s);
      *passed += p;
      *failed += f;
      *skipped += s;

      group_result = group_result->next;
    }

    run_test_config = run_test_config->next;
  }

  return true;
}

bool print_test_results(ide_run_test_suite_t *run_test_suite)
{
  ide_run_test_suite_t* test_suite = run_test_suite;
  TEEIO_PRINT(("\n"));
  TEEIO_PRINT((" Print test results.\n"));

  int passed, failed, skipped;

  while(test_suite) {
    test_suite_cases_statics(test_suite, &passed, &failed, &skipped);
    TEEIO_PRINT((" %s - pass: %d, fail: %d, skip: %d\n", test_suite->name, passed, failed, skipped));

    ide_run_test_config_t *run_test_config = test_suite->test_config;
    while(run_test_config) {

      ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)run_test_config->test_context;
      TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);

      TEEIO_PRINT(("   TestConfiguration (%s)\n", run_test_config->name));

      ide_run_test_group_result_t * group_result = run_test_config->group_result;
      while(group_result) {

        ide_run_test_case_result_t *case_result = group_result->case_result;
        test_group_cases_statics(group_result, &passed, &failed, &skipped);
        TEEIO_PRINT(("     TestGroup (%s %s) - pass: %d, fail: %d, skip: %d\n", group_result->name, case_result->class, passed, failed, skipped));

        while(case_result) {
          TEEIO_PRINT(("       %s: case - %s; ide_stream_secure - %s\n", case_result->name, m_test_case_result_str[case_result->case_result], m_test_config_result_str[case_result->config_result]));

          case_result = case_result->next;
        }

        group_result = group_result->next;
        TEEIO_PRINT(("\n"));
      }

      run_test_config = run_test_config->next;
      TEEIO_PRINT(("\n"));
    }

    TEEIO_PRINT((" ---------------------------------------------\n"));
    test_suite = test_suite->next;
  }

  return true;
}

bool clean_test_cases(ide_run_test_case_t *test_case)
{
  if(test_case == NULL) {
    return true;
  }

  ide_run_test_case_t *ptr_case = NULL;

  while(test_case) {
    if(test_case->test_context) {
      free(test_case->test_context);
    }
    ptr_case = test_case;
    test_case = test_case->next;
    free(ptr_case);
  }
  return true;
}

bool clean_test_groups(ide_run_test_group_t *group)
{
  if(group == NULL) {
    return true;
  }

  ide_run_test_group_t *ptr_group = NULL;
  while(group) {
    clean_test_cases(group->test_case);
    if(group->test_context) {
      free(group->test_context);
    }

    ptr_group = group;
    group = group->next;
    free(ptr_group);
  }

  return true;
}

bool clean_test_case_result(ide_run_test_case_result_t *case_result)
{
  ide_run_test_case_result_t *ptr = NULL;
  while (case_result)
  {
    ptr = case_result;
    case_result = case_result->next;
    free(ptr);
  }  

  return true;
}

bool clean_test_group_results(ide_run_test_group_result_t *group_result)
{
  if(group_result == NULL) {
    return true;
  }
  ide_run_test_group_result_t *ptr = NULL;

  while (group_result)
  {
    clean_test_case_result(group_result->case_result);

    ptr = group_result;
    group_result = group_result->next;
    free(ptr);
  }
  

  return true;
}

bool clean_test_config_items(ide_run_test_config_item_t *config_item)
{
  if(config_item == NULL) {
    return true;
  }
  ide_run_test_config_item_t *ptr = NULL;

  while (config_item) {
    ptr = config_item;
    config_item = config_item->next;
    free(ptr);
  }

  return true;
}

bool clean_test_configs(ide_run_test_config_t *config)
{
  if(config == NULL) {
    return true;
  }

  ide_run_test_config_t *ptr_config = NULL;
  while(config){
    clean_test_group_results(config->group_result);
    clean_test_config_items(config->config_item);
    if(config->test_context) {
      free(config->test_context);
    }
    ptr_config = config;
    config = config->next;
    free(ptr_config);
  }

  return true;
}

// clean the tests data
bool clean_tests_data(ide_run_test_suite_t* test_suite)
{
  ide_run_test_suite_t* suite = test_suite;
  if(suite == NULL) {
    return true;
  }

  ide_run_test_suite_t* ptr_suite = NULL;

  while(suite) {
    clean_test_groups(suite->test_group);
    clean_test_configs(suite->test_config);
    if(suite->test_context) {
      free(suite->test_context);
    }
    ptr_suite = suite;
    suite = suite->next;
    free(ptr_suite);
  }  

  return true;
}

/**
 * Run tests based on test_config
*/
bool run(IDE_TEST_CONFIG *test_config)
{
  ide_run_test_suite_t *run_test_suite = prepare_tests_data(test_config);
  ide_run_test_suite_t *itr = run_test_suite;

  while(itr != NULL) {
    do_run_test_suite(itr);
    itr = itr->next;
  }

  print_test_results(run_test_suite);

  clean_tests_data(run_test_suite);

  return true;
}