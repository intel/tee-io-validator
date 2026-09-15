/* Copyright 2026 Intel. SPDX-License-Identifier: BSD-3-Clause
 * Exercise the actual parser, entry exit policy, and saved-result evaluator.
 * Replace ONLY run() at the entry point: never enumerate or access PCI devices.
 * Synthetic assertion records here test policy, not protocol correctness.
 */
#include "teeio_validator.h"
#include "ide_test.h"
#define run host_run
#define main validator_entry
#include "../../teeio_validator/teeio_validator.c"
#undef main
#undef run
#include "../../teeio_validator/ide_test.c"
#include <assert.h>

bool host_run(IDE_TEST_CONFIG *config, bool *control_passed)
{
  const char *mode = getenv("TEEIO_HOST_MODE");
  ide_run_test_case_assertion_result_t primary = {0}, extra = {0};
  ide_run_test_case_result_t test = {0}, second = {0};
  ide_run_test_group_result_t group = {0};
  ide_run_test_config_result_t result = {0};
  ide_common_test_suite_context_t suite_context = {0};
  ide_run_test_suite_t suite = {0};

  puts("HOST: parsed catalog; PCI execution replaced (not a DUT pass)");
  if (mode == NULL) { *control_passed = false; return false; }
  primary.type = IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST;
  primary.class_id = SPDM_TEST_CASE_FAULT; primary.case_id = 1; primary.assertion_id = 1;
  primary.result = TEEIO_TEST_RESULT_PASS;
  test.class_id = SPDM_TEST_CASE_FAULT; test.case_id = 1;
  test.total_passed = 1; test.assertion_result = &primary;
  group.case_result = &test;
  group.func_results[TEEIO_TEST_GROUP_FUNC_SETUP].result = TEEIO_TEST_RESULT_PASS;
  group.func_results[TEEIO_TEST_GROUP_FUNC_TEARDOWN].result = TEEIO_TEST_RESULT_PASS;
  result.group_result = &group;
  suite_context.test_category = TEEIO_TEST_CATEGORY_SPDM;
  suite_context.result = &result;
  suite.test_context = &suite_context;
  if (!strcmp(mode, "skipped")) { test.total_passed = 0; test.assertion_result = NULL; }
  if (!strcmp(mode, "no-primary")) primary.assertion_id = 15;
  if (!strcmp(mode, "failed") || !strcmp(mode, "fired-failed")) {
    extra = primary; extra.assertion_id = 15; extra.result = TEEIO_TEST_RESULT_FAILED;
    primary.next = &extra; test.total_failed = 1;
  }
  if (!strcmp(mode, "not-tested")) primary.result = TEEIO_TEST_RESULT_NOT_TESTED;
  if (!strcmp(mode, "teardown"))
    group.func_results[TEEIO_TEST_GROUP_FUNC_TEARDOWN].result = TEEIO_TEST_RESULT_FAILED;
  if (!strcmp(mode, "wrong-class")) primary.class_id++;
  if (!strcmp(mode, "missing-driver")) test.class_id++;
  if (!strcmp(mode, "non-spdm")) suite_context.test_category = TEEIO_TEST_CATEGORY_PCIE_IDE;
  if (!strcmp(mode, "two-controls")) { second = test; test.next = &second; }
  if (!strcmp(mode, "extra-rule")) {
    config->fault_injection.rules[1] = config->fault_injection.rules[0];
    config->fault_injection.rule_count = 2;
  }
  if (!strcmp(mode, "fired-failed") || !strcmp(mode, "fired-pass")) {
    uint8_t frame[168] = {0};
    frame[2] = TEEIO_FAULT_DOE_TYPE_PLAIN_SECURED_SPDM;
    frame[4] = sizeof(frame) / 4;
    frame[8] = SPDM_MESSAGE_VERSION_14; frame[9] = SPDM_FINISH;
    assert(teeio_fault_apply(frame, sizeof(frame), sizeof(frame)).disposition ==
           TEEIO_FAULT_DISPOSITION_MUTATE);
    assert(teeio_fault_fire_count() == 1);
  }
  return fault_driver_results_passed(!strcmp(mode, "empty") ? NULL : &suite,
                                    control_passed);
}

int main(int argc, char **argv)
{
  return validator_entry(argc, argv);
}