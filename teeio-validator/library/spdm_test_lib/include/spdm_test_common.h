/**
 *  Copyright Notice:
 *  Copyright 2025 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __SPDM_TEST_COMMON_H__
#define __SPDM_TEST_COMMON_H__

void spdm_test_lib_init_test_cases();
int spdm_test_lib_get_group_id(int case_class);
extern TEEIO_TEST_CASES m_spdm_test_case_funcs[];
extern ide_test_case_name_t m_spdm_test_case_names[];

// SPDM Test Config
// default config
bool spdm_test_config_default_enable(void *test_context);
bool spdm_test_config_default_disable(void *test_context);
bool spdm_test_config_default_support(void *test_context);
bool spdm_test_config_default_check(void *test_context);

//
// SPDM Test Group
//
bool spdm_test_group_setup(void *test_context);
bool spdm_test_group_teardown(void *test_context);

//
// SPDM version applicability (informational annotation of failed cases)
//

// Highest responder-advertised SPDM version recognized by the bundled
// SPDM-Responder-Validator at the pinned revision. See the version switch in
// spdm_responder_test_1_version.c and SPDM_TEST_VERSION_MASK_V13 in
// spdm_responder_test.h.
#define SPDM_TEST_SUITE_MAX_KNOWN_VERSION SPDM_MESSAGE_VERSION_13

typedef struct {
  int group_id;         // SPDM_RESPONDER_TEST_GROUP_xxx
  int case_id;          // SPDM_RESPONDER_TEST_CASE_xxx
  uint8_t min_version;  // SPDM_MESSAGE_VERSION_xx
  uint8_t max_version;  // 0 = open-ended ("x.y+")
  // The case deliberately sends requests with a non-negotiated SPDM version
  // (the *_VERSION_MISMATCH cases), so a version on the wire that differs from
  // the connection version is expected and is not itself a sign of a problem.
  bool sends_non_negotiated_version;
} spdm_test_case_version_info_t;

void spdm_test_set_active_context(void *spdm_context, uint8_t ini_spdm_version);
void spdm_test_clear_active_context(void);

const spdm_test_case_version_info_t* spdm_test_get_case_version_info(int group_id, int case_id);
void spdm_test_format_version(uint8_t version, char *buf, size_t size);
void spdm_test_format_version_range(const spdm_test_case_version_info_t *info, char *buf, size_t size);

bool spdm_test_detect_version_coverage_gap(
  int group_id, int case_id, int assertion_id,
  teeio_test_result_t result, const char *message,
  char *note, size_t note_size);

void spdm_test_annotate_failed_case(
  int group_id, int case_id, int case_class, int assertion_id,
  teeio_test_result_t result, const char *message);

#endif
