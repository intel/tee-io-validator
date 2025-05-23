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
#include "cxl_ide_test_lib.h"
#include "cxl_tsp_test_lib.h"
#include "tdisp_test_lib.h"
#include "spdm_test_lib.h"

const char *m_ide_test_topology_name[] = {
  "SelectiveIDE",
  "LinkIDE",
  "SelectiveAndLinkIDE"
};

const char* m_test_case_result_str[] = {
  "skipped", "pass", "fail"
};

const char* m_test_config_result_str[] = {
  "skipped", "pass", "fail"
};

const char* m_assertion_result_str[] = {
  "skipped", "pass", "fail"
};

const char* m_config_item_operation_str[] = {
  "support", "enable", "disable", "check"
};

ide_run_test_config_result_t* g_current_config_result = NULL;
ide_run_test_group_result_t* g_current_group_result = NULL;
ide_run_test_case_result_t* g_current_case_result = NULL;

extern const char *TEEIO_TEST_CATEGORY_NAMES[];
teeio_test_funcs_t m_teeio_test_funcs[TEEIO_TEST_CATEGORY_MAX] = {
  // PCIE-IDE
  { 0 },
  // CXL-IDE
  { 0 },
  // CXL-TSP
  { 0 },
  // TDISP
  { 0 },
  // SPDM
  { 0 },
};

void teeio_init_test_funcs()
{
  pcie_ide_test_lib_register_test_suite_funcs(&m_teeio_test_funcs[TEEIO_TEST_CATEGORY_PCIE_IDE]);
  cxl_ide_test_lib_register_test_suite_funcs(&m_teeio_test_funcs[TEEIO_TEST_CATEGORY_CXL_IDE]);
  cxl_tsp_test_lib_register_test_suite_funcs(&m_teeio_test_funcs[TEEIO_TEST_CATEGORY_CXL_TSP]);
  tdisp_test_lib_register_test_suite_funcs(&m_teeio_test_funcs[TEEIO_TEST_CATEGORY_TDISP]);
  spdm_test_lib_register_test_suite_funcs(&m_teeio_test_funcs[TEEIO_TEST_CATEGORY_SPDM]);
}

void teeio_clean_test_libs()
{
  spdm_test_lib_clean();
}

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

const char* get_test_configuration_name(int configuration_type, TEEIO_TEST_CATEGORY test_category)
{
  teeio_test_funcs_t* test_funcs = &m_teeio_test_funcs[test_category];
  if(test_funcs->get_configuration_name_func == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s is not supported yet.\n", TEEIO_TEST_CATEGORY_NAMES[(int)test_category]));
    return NULL;
  }

  return test_funcs->get_configuration_name_func(configuration_type);
}

bool parse_test_configuration_priv_names(const char* key, const char* value, TEEIO_TEST_CATEGORY test_category, IDE_TEST_CONFIGURATION* config)
{
  teeio_test_funcs_t* test_funcs = &m_teeio_test_funcs[test_category];
  if(test_funcs->parse_configuration_priv_name_func == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%s is not supported in %s yet.\n", __func__, TEEIO_TEST_CATEGORY_NAMES[(int)test_category]));
    return true;
  }

  return test_funcs->parse_configuration_priv_name_func(key, value, config);
}

const char** get_test_configuration_priv_names(TEEIO_TEST_CATEGORY test_category)
{
  teeio_test_funcs_t* test_funcs = &m_teeio_test_funcs[test_category];
  if(test_funcs->get_configuration_priv_names_func == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%s is not supported in %s yet.\n", __func__, TEEIO_TEST_CATEGORY_NAMES[(int)test_category]));
    return NULL;
  }

  return test_funcs->get_configuration_priv_names_func();
}

bool teeio_check_configuration_bitmap(uint32_t* bitmap, TEEIO_TEST_CATEGORY test_category)
{
  teeio_test_funcs_t* test_funcs = &m_teeio_test_funcs[test_category];
  if(test_funcs->check_configuration_bitmap_func == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s is not supported yet.\n", TEEIO_TEST_CATEGORY_NAMES[(int)test_category]));
    return false;
  }

  return test_funcs->check_configuration_bitmap_func(bitmap);
}

ide_test_case_name_t* get_test_case_name(int case_class, TEEIO_TEST_CATEGORY test_category)
{
  teeio_test_funcs_t* test_funcs = &m_teeio_test_funcs[test_category];
  if(test_funcs->get_case_name_func == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s is not supported yet.\n", TEEIO_TEST_CATEGORY_NAMES[(int)test_category]));
    return NULL;
  }

  return test_funcs->get_case_name_func(case_class);
}

ide_test_case_name_t* get_test_case_from_string(const char* test_case_name, int* index, TEEIO_TEST_CATEGORY test_category)
{
  if(test_case_name == NULL) {
    return NULL;
  }

  teeio_test_funcs_t* test_funcs = &m_teeio_test_funcs[test_category];
  if(test_funcs->get_case_name_func == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s is not supported yet.\n", TEEIO_TEST_CATEGORY_NAMES[(int)test_category]));
    return NULL;
  }

  char buf1[MAX_LINE_LENGTH] = {0};
  char buf2[MAX_LINE_LENGTH] = {0};
  strncpy(buf1, test_case_name, MAX_LINE_LENGTH);

  int pos = find_char_in_str(buf1, '.');
  if(pos == -1) {
    return NULL;
  }

  buf1[pos] = '\0';
  char* ptr1 = buf1 + pos + 1;

  ide_test_case_name_t* test_case;
  for(int i = 0; ; i++) {
    test_case = test_funcs->get_case_name_func(i);
    if(test_case->class == NULL) {
      return NULL;
    }
    if(strcmp(buf1, test_case->class) == 0) {
      break;
    }
  }

  bool hit = false;
  strncpy(buf2, test_case->names, MAX_LINE_LENGTH);
  char *ptr2 = buf2;
  int j = 0;

  pos = find_char_in_str(ptr2, ',');

  do {
    if(pos != -1) {
      ptr2[pos] = '\0';
    }
    if(strcmp(ptr1, ptr2) == 0) {
      hit = true;
      break;
    }

    if(pos == -1) {
      break;
    }

    ptr2 += (pos + 1);
    pos = find_char_in_str(ptr2, ',');
    j++;
  } while(true);

  if(index != NULL) {
    *index = j;
  }

  return hit ? test_case : NULL;
}

bool is_valid_test_case(const char* test_case_name, TEEIO_TEST_CATEGORY test_category)
{
  return get_test_case_from_string(test_case_name, NULL, test_category) != NULL;
}

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

  teeio_test_funcs_t* test_funcs = &m_teeio_test_funcs[test_category];
  if(test_funcs->get_configuration_funcs_func == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s is not supported yet.\n", TEEIO_TEST_CATEGORY_NAMES[(int)test_category]));
    return NULL;
  }

  ide_test_config_funcs_t *config_func = test_funcs->get_configuration_funcs_func(top_type, config_type);
  TEEIO_ASSERT(config_func);

  ide_run_test_config_item_t *config_item = (ide_run_test_config_item_t*)malloc(sizeof(ide_run_test_config_item_t));
  TEEIO_ASSERT(config_item != NULL);
  memset(config_item, 0, sizeof(ide_run_test_config_item_t));
  config_item->type = config_type;
  config_item->test_category = test_category;

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

  run_test_config->config_id = config_id;

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
  teeio_test_funcs_t* test_funcs = &m_teeio_test_funcs[test_category];
  if(test_funcs->get_configuration_bitmask_func == NULL || test_funcs->get_configuration_name_func == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s is not supported yet.\n", TEEIO_TEST_CATEGORY_NAMES[(int)test_category]));
    return NULL;
  }

  uint32_t config_bitmask = test_funcs->get_configuration_bitmask_func(configuration->type);
  uint32_t config_bits = configuration->bit_map & config_bitmask;
  char name_buf[MAX_NAME_LENGTH] = {0};
  const char* configuration_name = NULL;
  int offset = 0;
  for(int i = 0; i < 32; i++) {
    if(config_bits & BIT_MASK(i)) {
      alloc_run_test_config_item(run_test_config, i, top->type, test_category);
      configuration_name = test_funcs->get_configuration_name_func(i);
      TEEIO_ASSERT(offset + strlen(configuration_name) + 1 < MAX_NAME_LENGTH);
      sprintf(name_buf + offset, "%s+", configuration_name);
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
ide_run_test_group_t *alloc_run_test_group(TEEIO_TEST_CATEGORY test_category, ide_run_test_suite_t *rts, IDE_TEST_CONFIG *test_config, int top_id, int case_class)
{
  ide_run_test_group_t *run_test_group = (ide_run_test_group_t *)malloc(sizeof(ide_run_test_group_t));
  TEEIO_ASSERT(run_test_group);
  memset(run_test_group, 0, sizeof(ide_run_test_group_t));

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

  teeio_test_funcs_t* test_funcs = &m_teeio_test_funcs[test_category];
  if(test_funcs->get_group_funcs_func == NULL || test_funcs->alloc_test_group_context_func == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s is not supported yet.\n", TEEIO_TEST_CATEGORY_NAMES[(int)test_category]));
    return NULL;
  }

  ide_test_group_funcs_t *group_funcs = test_funcs->get_group_funcs_func(top->type);
  run_test_group->setup_func = group_funcs->setup;
  run_test_group->teardown_func = group_funcs->teardown;

  teeio_common_test_group_context_t *context = (teeio_common_test_group_context_t *)test_funcs->alloc_test_group_context_func();
  TEEIO_ASSERT(context);
  context->suite_context = rts->test_context;
  context->top = top;
  context->upper_port.port = upper_port;
  context->lower_port.port = lower_port;
  context->root_port.port = root_port;
  context->case_class = case_class;

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
bool alloc_run_test_case(TEEIO_TEST_CATEGORY test_category, ide_run_test_group_t *run_test_group, IDE_COMMON_TEST_CASE case_class, uint32_t case_id)
{
  TEEIO_ASSERT(case_class < MAX_TEST_CASE_NUM);
  TEEIO_ASSERT(case_id <= MAX_CASE_ID);

  ide_run_test_case_t *run_test_case = (ide_run_test_case_t *)malloc(sizeof(ide_run_test_case_t));
  TEEIO_ASSERT(run_test_case != NULL);
  memset(run_test_case, 0, sizeof(ide_run_test_case_t));

  teeio_test_funcs_t* test_funcs = &m_teeio_test_funcs[test_category];
  if(test_funcs->get_case_name_func == NULL || test_funcs->get_case_funcs_func == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s is not supported yet.\n", TEEIO_TEST_CATEGORY_NAMES[test_category]));
    return NULL;
  }

  ide_test_case_name_t* test_case = test_funcs->get_case_name_func(case_class);
  strncpy(run_test_case->class, test_case->class, MAX_NAME_LENGTH);
  sprintf(run_test_case->name, "%s.%d", test_case->class, case_id);
  run_test_case->class_id = case_class;
  run_test_case->case_id = case_id;

  ide_test_case_funcs_t* case_funcs = test_funcs->get_case_funcs_func(case_class, case_id - 1);
  run_test_case->run_func = case_funcs->run;
  run_test_case->setup_func = case_funcs->setup;
  run_test_case->teardown_func = case_funcs->teardown;
  run_test_case->config_check_required = case_funcs->config_check_required;

  ide_common_test_case_context_t* context = (ide_common_test_case_context_t *)malloc(sizeof(ide_common_test_case_context_t));
  TEEIO_ASSERT(context);
  memset(context, 0, sizeof(ide_common_test_case_context_t));

  context->group_context = run_test_group->test_context;
  context->test_case = run_test_case;
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

  for(int i = 0; i < MAX_TEST_CASE_NUM; i++) {
    IDE_TEST_CASE *tc = suite->test_cases.cases + i;
    if(tc->cases_cnt == 0) {
      continue;
    }

    run_test_group = alloc_run_test_group(suite->test_category, run_test_suite, test_config, top->id, i);
    TEEIO_ASSERT(run_test_group);

    for (int j = 0; j < tc->cases_cnt; j++)
    {
      alloc_run_test_case(suite->test_category, run_test_group, i, tc->cases_id[j]);
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

ide_run_test_case_result_t *alloc_run_test_case_result(ide_run_test_group_result_t* group_result, ide_run_test_case_t *test_case)
{
  ide_run_test_case_result_t *case_result = (ide_run_test_case_result_t *)malloc(sizeof(ide_run_test_case_result_t));
  TEEIO_ASSERT(case_result);
  memset(case_result, 0, sizeof(ide_run_test_case_result_t));

  strncpy(case_result->name, test_case->name, MAX_NAME_LENGTH);
  strncpy(case_result->class, test_case->class, MAX_NAME_LENGTH);

  case_result->case_id = test_case->case_id;
  case_result->class_id = test_case->class_id;

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

static bool do_run_test_config_disable(ide_run_test_config_t *run_test_config, IDE_TEST_TOPOLOGY_TYPE top_type, TEEIO_TEST_CATEGORY test_category)
{
  bool ret = false;

  ide_run_test_config_item_t* config_item = run_test_config->config_item;
  do {
    TEEIO_ASSERT(config_item->disable_func);
    ret = config_item->disable_func(run_test_config->test_context);

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

bool do_run_test_case(ide_run_test_case_t *test_case, ide_run_test_config_t *run_test_config, TEEIO_TEST_CATEGORY test_category, IDE_TEST_TOPOLOGY_TYPE top_type)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_case->test_context;
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);
  teeio_spdm_test_context_t m_spdm_test_context = {0};

  void *context = case_context;
  if(test_category == TEEIO_TEST_CATEGORY_SPDM) {
    m_spdm_test_context.spdm_context = spdm_test_get_spdm_context_from_test_context(case_context);
    context = &m_spdm_test_context;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Run %s\n", test_case->name));

  if(case_context->action == IDE_COMMON_TEST_ACTION_SKIP)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%s skipped.\n", test_case->name));
    return true;
  }

  // check if the test_config is supported.
  if(!do_run_test_config_support(run_test_config, top_type, test_category)) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%s is not supported.\n", run_test_config->name));
    return true;
  }

  // call test_config's enable function
  if(!do_run_test_config_enable(run_test_config, top_type, test_category)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "run_test_config_enable failed. %s skipped.\n", test_case->name));
    return true;
  }

  if(test_case->setup_func != NULL) {
    if(!test_case->setup_func(context)) {
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%s setup failed. So skipped.\n", test_case->name));
      case_context->action = IDE_COMMON_TEST_ACTION_SKIP;
      goto TestCaseDone;
    }
  }

  // again check if the case to be skipped after setup
  if(case_context->action == IDE_COMMON_TEST_ACTION_SKIP)
  {
    // set the test result
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%s skipped after setup.\n", test_case->name));
    goto TestCaseDone;
  }

  if(test_case->run_func != NULL) {
    test_case->run_func(context);
  }

  if(test_case->config_check_required) {
    // check config
    do_run_test_config_check(run_test_config, top_type, test_category);
  }

TestCaseDone:

  if(test_case->teardown_func != NULL) {
    test_case->teardown_func(context);
  }

  do_run_test_config_disable(run_test_config, top_type, test_category);

  return true;
}

/**
 * run_test_group
*/
bool do_run_test_group(ide_run_test_group_t *run_test_group, ide_run_test_config_t *run_test_config, ide_run_test_group_result_t* group_result, TEEIO_TEST_CATEGORY test_category)
{
  teeio_common_test_group_context_t *group_context = (teeio_common_test_group_context_t *)run_test_group->test_context;
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);
  group_context->config_id = run_test_config->config_id;

  ide_common_test_config_context_t *config_context = (ide_common_test_config_context_t *)run_test_config->test_context;
  TEEIO_ASSERT(config_context->signature == CONFIG_CONTEXT_SIGNATURE);
  config_context->group_context = group_context;

  IDE_TEST_TOPOLOGY_TYPE top_type = group_context->top->type;

  if(run_test_group->test_case == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "test_case is not set in test_group.\n"));
    return true;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Run TestGroup (%s %s %s)\n", run_test_group->name, run_test_config->name, run_test_group->test_case->class));

  // call run_test_group's setup function
  bool group_setup_result = true;
  TEEIO_ASSERT(run_test_group->setup_func);
  if(!run_test_group->setup_func(group_context)) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%s failed at test_group->setup().\n", run_test_config->name));
    group_setup_result = false;
  }

  ide_run_test_case_t *test_case = run_test_group->test_case;
  while (test_case != NULL)
  {
    // alloc case_result
    g_current_case_result = alloc_run_test_case_result(group_result, test_case);

    // run the test_case
    if(group_setup_result) {
      do_run_test_case(test_case, run_test_config, test_category, top_type);
    }

    // next case
    test_case = test_case->next;
    g_current_case_result = NULL;
  }

  // call run_test_group's teardown function
  if(group_setup_result) {
    run_test_group->teardown_func(group_context);
  }

  // config_context is reused between different run_group_test
  // So its group_context must be cleared here.
  config_context->group_context = NULL;

  return true;
}

ide_run_test_group_result_t* alloc_run_test_group_result(ide_run_test_group_t *run_test_group, ide_run_test_config_result_t* config_result)
{
  ide_run_test_group_result_t* group_result = (ide_run_test_group_result_t *)malloc(sizeof(ide_run_test_group_result_t));
  TEEIO_ASSERT(group_result);
  memset(group_result, 0, sizeof(ide_run_test_group_result_t));
  strncpy(group_result->name, run_test_group->name, MAX_NAME_LENGTH);

  ide_run_test_group_result_t* itr = config_result->group_result;
  if(itr == NULL) {
    config_result->group_result = group_result;
  } else {
    while(itr->next) {
      itr = itr->next;
    }
    itr->next = group_result;
  }

  return group_result;
}

bool append_config_result(ide_run_test_suite_t* run_test_suite, ide_run_test_config_result_t* config_result)
{

  ide_common_test_suite_context_t* suite_context = (ide_common_test_suite_context_t*)run_test_suite->test_context;
  TEEIO_ASSERT(suite_context->signature == SUITE_CONTEXT_SIGNATURE);

  if(suite_context->result == NULL) {
    suite_context->result = config_result;
    return true;
  }

  ide_run_test_config_result_t* itr = suite_context->result;
  while(itr->next) {
    itr = itr->next;
  }
  itr->next = config_result;

  return true;
}

ide_run_test_config_result_t* alloc_run_test_config_result(ide_run_test_suite_t* run_test_suite, ide_run_test_config_t* run_test_config)
{
  ide_run_test_config_result_t* config_result = (ide_run_test_config_result_t*)malloc(sizeof(ide_run_test_config_result_t));
  TEEIO_ASSERT(config_result);
  memset(config_result, 0, sizeof(ide_run_test_config_result_t));

  config_result->config_id = run_test_config->config_id;
  strncpy(config_result->name, run_test_config->name, MAX_NAME_LENGTH);

  append_config_result(run_test_suite, config_result);

  return config_result;
}

/**
 * run test suite
 */
bool do_run_test_suite(ide_run_test_suite_t *run_test_suite)
{
    ide_run_test_group_t *run_test_group = run_test_suite->test_group;
    ide_run_test_config_t *run_test_config = run_test_suite->test_config;

    ide_common_test_suite_context_t* suite_context = run_test_suite->test_context;
    TEEIO_ASSERT(suite_context->signature == SUITE_CONTEXT_SIGNATURE);

    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Run %s\n", run_test_suite->name));

    while(run_test_config != NULL) {
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Run Configuration_%d\n", run_test_config->config_id));
      g_current_config_result = alloc_run_test_config_result(run_test_suite, run_test_config);

      while (run_test_group != NULL)
      {
        // alloc group_result
        g_current_group_result = alloc_run_test_group_result(run_test_group, g_current_config_result);
        TEEIO_ASSERT(g_current_group_result);

        do_run_test_group(run_test_group, run_test_config, g_current_group_result, suite_context->test_category);

        run_test_group = run_test_group->next;
        g_current_group_result = NULL;
      }

      run_test_config = run_test_config->next;
      g_current_config_result = NULL;
    }

    TEEIO_PRINT(("\n"));

    return true;
}

bool test_suite_cases_statics(ide_run_test_suite_t *run_test_suite, int *passed, int *failed)
{
  if(run_test_suite == NULL) {
    return false;
  }
  *passed = 0;
  *failed = 0;

  ide_common_test_suite_context_t* suite_context = (ide_common_test_suite_context_t*)run_test_suite->test_context;
  TEEIO_ASSERT(suite_context->signature == SUITE_CONTEXT_SIGNATURE);
  ide_run_test_config_result_t* run_test_config_result = suite_context->result;
  while(run_test_config_result) {
    *passed += run_test_config_result->total_passed;
    *failed += run_test_config_result->total_failed;
    run_test_config_result = run_test_config_result->next;
  }

  return true;
}

bool print_test_case_assertion_result(ide_run_test_case_assertion_result_t* assertion_result)
{
  while(assertion_result) {
    if(assertion_result->type == IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST) {
      TEEIO_PRINT(("           Assertion%d.%d.%d: - %s %s\n", 
        assertion_result->class_id + 1,
        assertion_result->case_id,
        assertion_result->assertion_id,
        m_assertion_result_str[assertion_result->result],
        assertion_result->extra_data));
    } else if(assertion_result->type == IDE_COMMON_TEST_CASE_ASSERTION_TYPE_SEPARATOR) {
      TEEIO_PRINT(("       %s\n", assertion_result->extra_data));
    }

    assertion_result = assertion_result->next;
  }

  return true;
}

bool print_test_config_item_result(TEEIO_TEST_CATEGORY test_category, ide_run_test_config_item_result_t* config_item_result, int config_id, bool phase1)
{
  if(config_item_result == NULL) {
    return true;
  }

  teeio_test_funcs_t* test_funcs = &m_teeio_test_funcs[test_category];
  if(test_funcs->get_configuration_name_func == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "%s is not supported yet.\n", TEEIO_TEST_CATEGORY_NAMES[(int)test_category]));
    return false;
  }

  ide_run_test_config_item_result_t *itr = config_item_result;
  char buffer1[MAX_LINE_LENGTH/4] = {0};
  char buffer2[MAX_LINE_LENGTH] = {0};
  int i, pos, start, end;

  TEEIO_PRINT(("       Configuration_%d:\n", config_id));
  while(itr) {
    pos = 0;
    memset(buffer2, 0, sizeof(buffer2));
    start = phase1 ? 0 : TEEIO_TEST_CONFIG_FUNC_MAX/2;
    end = phase1 ? TEEIO_TEST_CONFIG_FUNC_MAX/2 : TEEIO_TEST_CONFIG_FUNC_MAX;

    for(i = start; i < end; i++) {
      sprintf(buffer1, "%s: %s", m_config_item_operation_str[i], m_test_config_result_str[itr->results[i]]);
      sprintf(buffer2 + pos, "%-18s ", buffer1);
      pos = strlen(buffer2);
    }
    TEEIO_PRINT(("          %+10s: - %s\n", test_funcs->get_configuration_name_func(itr->config_item_id), buffer2));

    itr = itr->next;
  }

  return true;
}

bool print_test_results(ide_run_test_suite_t *run_test_suite, bool detail)
{
  ide_run_test_suite_t* test_suite = run_test_suite;
  TEEIO_PRINT(("\n"));
  if(detail) {
    TEEIO_PRINT((" Print detailed results.\n"));
  } else {
    TEEIO_PRINT((" Print summary results.\n"));
  }

  int passed, failed;
  teeio_test_result_t test_result = TEEIO_TEST_RESULT_NOT_TESTED;
  ide_common_test_suite_context_t *suite_context = NULL;
  ide_run_test_config_result_t* run_test_config_result = NULL;

  while(test_suite) {
    suite_context = (ide_common_test_suite_context_t *)test_suite->test_context;
    TEEIO_ASSERT(suite_context->signature == SUITE_CONTEXT_SIGNATURE);

    test_suite_cases_statics(test_suite, &passed, &failed);
    if(detail) {
      TEEIO_PRINT((" %s (%s)\n", test_suite->name, TEEIO_TEST_CATEGORY_NAMES[suite_context->test_category]));
    } else {
      TEEIO_PRINT((" %s (%s) - pass: %d, fail: %d\n", test_suite->name, TEEIO_TEST_CATEGORY_NAMES[suite_context->test_category], passed, failed));
    }

    run_test_config_result = suite_context->result;
    while(run_test_config_result) {

      TEEIO_PRINT(("   Configuration_%d (%s)\n", run_test_config_result->config_id, run_test_config_result->name));

      ide_run_test_group_result_t * group_result = run_test_config_result->group_result;
      while(group_result) {

        ide_run_test_case_result_t *case_result = group_result->case_result;
        const char* case_class = case_result->class;

        if(detail) {
          TEEIO_PRINT(("     TestGroup (%s) - setup %s %s\n", 
                      case_class,
                      m_test_case_result_str[group_result->func_results[TEEIO_TEST_GROUP_FUNC_SETUP].result],
                      group_result->func_results[TEEIO_TEST_GROUP_FUNC_SETUP].extra_data));
        } else {
          TEEIO_PRINT(("     TestGroup (%s) - pass: %d, fail: %d\n",
                      case_class,
                      group_result->total_passed, group_result->total_failed));
        }

        while(case_result) {
          if(case_result->total_failed == 0 && case_result->total_passed == 0) {
            test_result = TEEIO_TEST_RESULT_NOT_TESTED;
          } else if(case_result->total_failed == 0 && case_result->total_passed > 0) {
            test_result = TEEIO_TEST_RESULT_PASS;
          } else {
            test_result = TEEIO_TEST_RESULT_FAILED;
          }

          if(detail) {
            TEEIO_PRINT(("       TestCase %s: %s\n", case_result->name, m_test_case_result_str[test_result]));
            print_test_case_assertion_result(case_result->assertion_result);
            TEEIO_PRINT(("\n"));
          } else {
            TEEIO_PRINT(("       TestCase %s: %s (pass: %d, fail: %d)\n", case_result->name, m_test_case_result_str[test_result], case_result->total_passed, case_result->total_failed));
          }
          case_result = case_result->next;
        }
  
        if(detail) {
          TEEIO_PRINT(("     TestGroup (%s) - teardown %s\n",
                    case_class, m_test_case_result_str[group_result->func_results[TEEIO_TEST_GROUP_FUNC_TEARDOWN].result]));
        }

        group_result = group_result->next;
        TEEIO_PRINT(("\n"));
      }

      run_test_config_result = run_test_config_result->next;
      TEEIO_PRINT(("\n"));
    }

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

bool clean_test_case_assertion_result(ide_run_test_case_assertion_result_t *result)
{
  ide_run_test_case_assertion_result_t *ptr = NULL;
  while(result) {
    ptr = result;
    result = result->next;
    free(ptr);
  }

  return true;
}

bool clean_test_config_item_result(ide_run_test_config_item_result_t *result)
{
  ide_run_test_config_item_result_t *ptr = NULL;
  while(result) {
    ptr = result;
    result = result->next;
    free(ptr);
  }

  return true;
}

bool clean_test_case_result(ide_run_test_case_result_t *case_result)
{
  ide_run_test_case_result_t *ptr = NULL;
  while (case_result)
  {
    clean_test_case_assertion_result(case_result->assertion_result);
    clean_test_config_item_result(case_result->config_item_result);

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

bool clean_suite_context(void *context)
{
  if(context == NULL) {
    return true;
  }

  ide_common_test_suite_context_t* suite_context = (ide_common_test_suite_context_t*)context;
  TEEIO_ASSERT(suite_context->signature == SUITE_CONTEXT_SIGNATURE);

  ide_run_test_config_result_t* ptr = NULL;
  ide_run_test_config_result_t* config_result = suite_context->result;

  while(config_result) {

    clean_test_group_results(config_result->group_result);

    ptr = config_result->next;
    free(config_result);
    config_result = ptr;
  }

  free(context);
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
    clean_suite_context(suite->test_context);
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

  print_test_results(run_test_suite, true);
  print_test_results(run_test_suite, false);

  clean_tests_data(run_test_suite);

  return true;
}