/**
 *  Copyright Notice:
 *  Copyright 2025 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "helperlib.h"
#include "ide_test.h"
#include "hal/library/memlib.h"
#include "library/common_test_utility_lib.h"
#include "library/spdm_responder_conformance_test_lib.h"
#include "spdm_test_common.h"
#include "spdm_test_lib.h"
#include "teeio_spdmlib.h"

/**
 * SPDM version applicability of each conformance case.
 *
 * Source of truth: the "SPDM Version:" line of each case in the doc folder of
 * spdm-emu/SPDM-Responder-Validator at the pinned revision.
 * max_version == 0 means open-ended ("x.y+"). The last column marks cases
 * that deliberately send requests with a non-negotiated version.
 *
 * This table is only used to annotate failed-case output. It is deliberately
 * NOT used to re-classify a result: several cases intentionally send requests
 * with a version other than the negotiated one, so the connection version at
 * failure time cannot by itself decide whether a failure is genuine.
 */
#define OPEN_ENDED 0

static const spdm_test_case_version_info_t m_spdm_case_version_info[] = {
  // 1. VERSION
  {SPDM_RESPONDER_TEST_GROUP_VERSION, SPDM_RESPONDER_TEST_CASE_VERSION_SUCCESS_10, SPDM_MESSAGE_VERSION_10, OPEN_ENDED, false},

  // 2. CAPABILITIES
  {SPDM_RESPONDER_TEST_GROUP_CAPABILITIES, SPDM_RESPONDER_TEST_CASE_CAPABILITIES_SUCCESS_10,                       SPDM_MESSAGE_VERSION_10, SPDM_MESSAGE_VERSION_10, false},
  {SPDM_RESPONDER_TEST_GROUP_CAPABILITIES, SPDM_RESPONDER_TEST_CASE_CAPABILITIES_VERSION_MISMATCH,                 SPDM_MESSAGE_VERSION_10, OPEN_ENDED, true},
  {SPDM_RESPONDER_TEST_GROUP_CAPABILITIES, SPDM_RESPONDER_TEST_CASE_CAPABILITIES_SUCCESS_11,                       SPDM_MESSAGE_VERSION_11, SPDM_MESSAGE_VERSION_11, false},
  {SPDM_RESPONDER_TEST_GROUP_CAPABILITIES, SPDM_RESPONDER_TEST_CASE_CAPABILITIES_INVALID_REQUEST,                  SPDM_MESSAGE_VERSION_11, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_CAPABILITIES, SPDM_RESPONDER_TEST_CASE_CAPABILITIES_SUCCESS_12,                       SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_12, false},
  {SPDM_RESPONDER_TEST_GROUP_CAPABILITIES, SPDM_RESPONDER_TEST_CASE_CAPABILITIES_UNEXPECTED_REQUEST_NON_IDENTICAL, SPDM_MESSAGE_VERSION_10, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_CAPABILITIES, SPDM_RESPONDER_TEST_CASE_CAPABILITIES_SUCCESS_13,                       SPDM_MESSAGE_VERSION_13, SPDM_MESSAGE_VERSION_13, false},

  // 3. ALGORITHMS
  {SPDM_RESPONDER_TEST_GROUP_ALGORITHMS, SPDM_RESPONDER_TEST_CASE_ALGORITHMS_SUCCESS_10,                       SPDM_MESSAGE_VERSION_10, SPDM_MESSAGE_VERSION_10, false},
  {SPDM_RESPONDER_TEST_GROUP_ALGORITHMS, SPDM_RESPONDER_TEST_CASE_ALGORITHMS_VERSION_MISMATCH,                 SPDM_MESSAGE_VERSION_10, OPEN_ENDED, true},
  {SPDM_RESPONDER_TEST_GROUP_ALGORITHMS, SPDM_RESPONDER_TEST_CASE_ALGORITHMS_INVALID_REQUEST,                  SPDM_MESSAGE_VERSION_10, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_ALGORITHMS, SPDM_RESPONDER_TEST_CASE_ALGORITHMS_SUCCESS_11,                       SPDM_MESSAGE_VERSION_11, SPDM_MESSAGE_VERSION_11, false},
  {SPDM_RESPONDER_TEST_GROUP_ALGORITHMS, SPDM_RESPONDER_TEST_CASE_ALGORITHMS_SUCCESS_12,                       SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_12, false},
  {SPDM_RESPONDER_TEST_GROUP_ALGORITHMS, SPDM_RESPONDER_TEST_CASE_ALGORITHMS_UNEXPECTED_REQUEST_NON_IDENTICAL, SPDM_MESSAGE_VERSION_10, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_ALGORITHMS, SPDM_RESPONDER_TEST_CASE_ALGORITHMS_SUCCESS_13,                       SPDM_MESSAGE_VERSION_13, SPDM_MESSAGE_VERSION_13, false},

  // 4. DIGESTS
  {SPDM_RESPONDER_TEST_GROUP_DIGESTS, SPDM_RESPONDER_TEST_CASE_DIGESTS_SUCCESS_10,       SPDM_MESSAGE_VERSION_10, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_DIGESTS, SPDM_RESPONDER_TEST_CASE_DIGESTS_VERSION_MISMATCH, SPDM_MESSAGE_VERSION_10, OPEN_ENDED, true},

  // 5. CERTIFICATE
  {SPDM_RESPONDER_TEST_GROUP_CERTIFICATE, SPDM_RESPONDER_TEST_CASE_CERTIFICATE_SUCCESS_10,            SPDM_MESSAGE_VERSION_10, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_CERTIFICATE, SPDM_RESPONDER_TEST_CASE_CERTIFICATE_VERSION_MISMATCH,      SPDM_MESSAGE_VERSION_10, OPEN_ENDED, true},
  {SPDM_RESPONDER_TEST_GROUP_CERTIFICATE, SPDM_RESPONDER_TEST_CASE_CERTIFICATE_INVALID_REQUEST,       SPDM_MESSAGE_VERSION_10, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_CERTIFICATE, SPDM_RESPONDER_TEST_CASE_CERTIFICATE_SPDM_X509_CERTIFICATE, SPDM_MESSAGE_VERSION_10, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_CERTIFICATE, SPDM_RESPONDER_TEST_CASE_CERTIFICATE_SIZE_REQ,              SPDM_MESSAGE_VERSION_13, OPEN_ENDED, false},

  // 6. CHALLENGE_AUTH
  {SPDM_RESPONDER_TEST_GROUP_CHALLENGE_AUTH, SPDM_RESPONDER_TEST_CASE_CHALLENGE_AUTH_SUCCESS_10_A1B1C1, SPDM_MESSAGE_VERSION_10, SPDM_MESSAGE_VERSION_11, false},
  {SPDM_RESPONDER_TEST_GROUP_CHALLENGE_AUTH, SPDM_RESPONDER_TEST_CASE_CHALLENGE_AUTH_SUCCESS_10_A1B2C1, SPDM_MESSAGE_VERSION_10, SPDM_MESSAGE_VERSION_11, false},
  {SPDM_RESPONDER_TEST_GROUP_CHALLENGE_AUTH, SPDM_RESPONDER_TEST_CASE_CHALLENGE_AUTH_SUCCESS_10_A1B3C1, SPDM_MESSAGE_VERSION_10, SPDM_MESSAGE_VERSION_11, false},
  {SPDM_RESPONDER_TEST_GROUP_CHALLENGE_AUTH, SPDM_RESPONDER_TEST_CASE_CHALLENGE_AUTH_VERSION_MISMATCH,  SPDM_MESSAGE_VERSION_10, OPEN_ENDED, true},
  {SPDM_RESPONDER_TEST_GROUP_CHALLENGE_AUTH, SPDM_RESPONDER_TEST_CASE_CHALLENGE_AUTH_INVALID_REQUEST,   SPDM_MESSAGE_VERSION_10, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_CHALLENGE_AUTH, SPDM_RESPONDER_TEST_CASE_CHALLENGE_AUTH_SUCCESS_12_A1B1C1, SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_13, false},
  {SPDM_RESPONDER_TEST_GROUP_CHALLENGE_AUTH, SPDM_RESPONDER_TEST_CASE_CHALLENGE_AUTH_SUCCESS_12_A1B2C1, SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_13, false},
  {SPDM_RESPONDER_TEST_GROUP_CHALLENGE_AUTH, SPDM_RESPONDER_TEST_CASE_CHALLENGE_AUTH_SUCCESS_12_A1B3C1, SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_13, false},
  {SPDM_RESPONDER_TEST_GROUP_CHALLENGE_AUTH, SPDM_RESPONDER_TEST_CASE_CHALLENGE_AUTH_SUCCESS_12_A1B4C1, SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_13, false},
  {SPDM_RESPONDER_TEST_GROUP_CHALLENGE_AUTH, SPDM_RESPONDER_TEST_CASE_CHALLENGE_AUTH_SUCCESS_12_A2B1C1, SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_13, false},
  {SPDM_RESPONDER_TEST_GROUP_CHALLENGE_AUTH, SPDM_RESPONDER_TEST_CASE_CHALLENGE_AUTH_SUCCESS_12_A2B2C1, SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_13, false},
  {SPDM_RESPONDER_TEST_GROUP_CHALLENGE_AUTH, SPDM_RESPONDER_TEST_CASE_CHALLENGE_AUTH_SUCCESS_12_A2B3C1, SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_13, false},
  {SPDM_RESPONDER_TEST_GROUP_CHALLENGE_AUTH, SPDM_RESPONDER_TEST_CASE_CHALLENGE_AUTH_SUCCESS_12_A2B4C1, SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_13, false},

  // 7. MEASUREMENTS
  {SPDM_RESPONDER_TEST_GROUP_MEASUREMENTS, SPDM_RESPONDER_TEST_CASE_MEASUREMENTS_SUCCESS_10,                           SPDM_MESSAGE_VERSION_10, SPDM_MESSAGE_VERSION_10, false},
  {SPDM_RESPONDER_TEST_GROUP_MEASUREMENTS, SPDM_RESPONDER_TEST_CASE_MEASUREMENTS_VERSION_MISMATCH,                     SPDM_MESSAGE_VERSION_10, OPEN_ENDED, true},
  {SPDM_RESPONDER_TEST_GROUP_MEASUREMENTS, SPDM_RESPONDER_TEST_CASE_MEASUREMENTS_INVALID_REQUEST,                      SPDM_MESSAGE_VERSION_10, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_MEASUREMENTS, SPDM_RESPONDER_TEST_CASE_MEASUREMENTS_SPDM_MEASUREMENT_BLOCK,               SPDM_MESSAGE_VERSION_10, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_MEASUREMENTS, SPDM_RESPONDER_TEST_CASE_MEASUREMENTS_SUCCESS_11,                           SPDM_MESSAGE_VERSION_11, SPDM_MESSAGE_VERSION_11, false},
  {SPDM_RESPONDER_TEST_GROUP_MEASUREMENTS, SPDM_RESPONDER_TEST_CASE_MEASUREMENTS_SUCCESS_11_IN_DHE_SESSION,            SPDM_MESSAGE_VERSION_11, SPDM_MESSAGE_VERSION_11, false},
  {SPDM_RESPONDER_TEST_GROUP_MEASUREMENTS, SPDM_RESPONDER_TEST_CASE_MEASUREMENTS_UNEXPECTED_REQUEST_IN_DHE_SESSION_HS, SPDM_MESSAGE_VERSION_11, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_MEASUREMENTS, SPDM_RESPONDER_TEST_CASE_MEASUREMENTS_SUCCESS_12,                           SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_12, false},
  {SPDM_RESPONDER_TEST_GROUP_MEASUREMENTS, SPDM_RESPONDER_TEST_CASE_MEASUREMENTS_SUCCESS_12_IN_DHE_SESSION,            SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_12, false},

  // 8. KEY_EXCHANGE_RSP
  {SPDM_RESPONDER_TEST_GROUP_KEY_EXCHANGE_RSP, SPDM_RESPONDER_TEST_CASE_KEY_EXCHANGE_RSP_SUCCESS_11,                    SPDM_MESSAGE_VERSION_11, SPDM_MESSAGE_VERSION_11, false},
  {SPDM_RESPONDER_TEST_GROUP_KEY_EXCHANGE_RSP, SPDM_RESPONDER_TEST_CASE_KEY_EXCHANGE_RSP_SUCCESS_11_HS_CLEAR,           SPDM_MESSAGE_VERSION_11, SPDM_MESSAGE_VERSION_11, false},
  {SPDM_RESPONDER_TEST_GROUP_KEY_EXCHANGE_RSP, SPDM_RESPONDER_TEST_CASE_KEY_EXCHANGE_RSP_VERSION_MISMATCH,              SPDM_MESSAGE_VERSION_11, OPEN_ENDED, true},
  {SPDM_RESPONDER_TEST_GROUP_KEY_EXCHANGE_RSP, SPDM_RESPONDER_TEST_CASE_KEY_EXCHANGE_RSP_UNEXPECTED_REQUEST_IN_SESSION, SPDM_MESSAGE_VERSION_11, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_KEY_EXCHANGE_RSP, SPDM_RESPONDER_TEST_CASE_KEY_EXCHANGE_RSP_INVALID_REQUEST,               SPDM_MESSAGE_VERSION_11, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_KEY_EXCHANGE_RSP, SPDM_RESPONDER_TEST_CASE_KEY_EXCHANGE_RSP_SUCCESS_12,                    SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_12, false},
  {SPDM_RESPONDER_TEST_GROUP_KEY_EXCHANGE_RSP, SPDM_RESPONDER_TEST_CASE_KEY_EXCHANGE_RSP_SUCCESS_12_HS_CLEAR,           SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_12, false},

  // 9. FINISH_RSP
  {SPDM_RESPONDER_TEST_GROUP_FINISH_RSP, SPDM_RESPONDER_TEST_CASE_FINISH_RSP_SUCCESS_11,                                 SPDM_MESSAGE_VERSION_11, SPDM_MESSAGE_VERSION_11, false},
  {SPDM_RESPONDER_TEST_GROUP_FINISH_RSP, SPDM_RESPONDER_TEST_CASE_FINISH_RSP_SUCCESS_11_HS_CLEAR,                        SPDM_MESSAGE_VERSION_11, SPDM_MESSAGE_VERSION_11, false},
  {SPDM_RESPONDER_TEST_GROUP_FINISH_RSP, SPDM_RESPONDER_TEST_CASE_FINISH_RSP_VERSION_MISMATCH,                           SPDM_MESSAGE_VERSION_11, OPEN_ENDED, true},
  {SPDM_RESPONDER_TEST_GROUP_FINISH_RSP, SPDM_RESPONDER_TEST_CASE_FINISH_RSP_UNEXPECTED_REQUEST_IN_SESSION,              SPDM_MESSAGE_VERSION_11, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_FINISH_RSP, SPDM_RESPONDER_TEST_CASE_FINISH_RSP_INVALID_REQUEST,                            SPDM_MESSAGE_VERSION_11, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_FINISH_RSP, SPDM_RESPONDER_TEST_CASE_FINISH_RSP_DECRYPT_ERROR_INVALID_VERIFY_DATA,          SPDM_MESSAGE_VERSION_11, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_FINISH_RSP, SPDM_RESPONDER_TEST_CASE_FINISH_RSP_DECRYPT_ERROR_INVALID_VERIFY_DATA_HS_CLEAR, SPDM_MESSAGE_VERSION_11, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_FINISH_RSP, SPDM_RESPONDER_TEST_CASE_FINISH_RSP_SUCCESS_12,                                 SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_12, false},
  {SPDM_RESPONDER_TEST_GROUP_FINISH_RSP, SPDM_RESPONDER_TEST_CASE_FINISH_RSP_SUCCESS_12_HS_CLEAR,                        SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_12, false},
  {SPDM_RESPONDER_TEST_GROUP_FINISH_RSP, SPDM_RESPONDER_TEST_CASE_FINISH_RSP_SESSION_REQUIRED,                           SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_12, false},

  // 12. HEARTBEAT_ACK
  {SPDM_RESPONDER_TEST_GROUP_HEARTBEAT_ACK, SPDM_RESPONDER_TEST_CASE_HEARTBEAT_ACK_SUCCESS_11_IN_DHE_SESSION,            SPDM_MESSAGE_VERSION_11, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_HEARTBEAT_ACK, SPDM_RESPONDER_TEST_CASE_HEARTBEAT_ACK_VERSION_MISMATCH_IN_DHE_SESSION,      SPDM_MESSAGE_VERSION_11, OPEN_ENDED, true},
  {SPDM_RESPONDER_TEST_GROUP_HEARTBEAT_ACK, SPDM_RESPONDER_TEST_CASE_HEARTBEAT_ACK_UNEXPECTED_REQUEST_IN_DHE_SESSION_HS, SPDM_MESSAGE_VERSION_11, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_HEARTBEAT_ACK, SPDM_RESPONDER_TEST_CASE_HEARTBEAT_ACK_SESSION_REQUIRED,                     SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_12, false},

  // 13. KEY_UPDATE_ACK
  {SPDM_RESPONDER_TEST_GROUP_KEY_UPDATE_ACK, SPDM_RESPONDER_TEST_CASE_KEY_UPDATE_ACK_SUCCESS_11_IN_DHE_SESSION,            SPDM_MESSAGE_VERSION_11, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_KEY_UPDATE_ACK, SPDM_RESPONDER_TEST_CASE_KEY_UPDATE_ACK_VERSION_MISMATCH_IN_DHE_SESSION,      SPDM_MESSAGE_VERSION_11, OPEN_ENDED, true},
  {SPDM_RESPONDER_TEST_GROUP_KEY_UPDATE_ACK, SPDM_RESPONDER_TEST_CASE_KEY_UPDATE_ACK_INVALID_REQUEST_IN_DHE_SESSION,       SPDM_MESSAGE_VERSION_11, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_KEY_UPDATE_ACK, SPDM_RESPONDER_TEST_CASE_KEY_UPDATE_ACK_UNEXPECTED_REQUEST_IN_DHE_SESSION_HS, SPDM_MESSAGE_VERSION_11, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_KEY_UPDATE_ACK, SPDM_RESPONDER_TEST_CASE_KEY_UPDATE_ACK_SESSION_REQUIRED,                     SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_12, false},

  // 16. END_SESSION_ACK
  {SPDM_RESPONDER_TEST_GROUP_END_SESSION_ACK, SPDM_RESPONDER_TEST_CASE_END_SESSION_ACK_SUCCESS_11_IN_DHE_SESSION,            SPDM_MESSAGE_VERSION_11, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_END_SESSION_ACK, SPDM_RESPONDER_TEST_CASE_END_SESSION_ACK_VERSION_MISMATCH_IN_DHE_SESSION,      SPDM_MESSAGE_VERSION_11, OPEN_ENDED, true},
  {SPDM_RESPONDER_TEST_GROUP_END_SESSION_ACK, SPDM_RESPONDER_TEST_CASE_END_SESSION_ACK_UNEXPECTED_REQUEST_IN_DHE_SESSION_HS, SPDM_MESSAGE_VERSION_11, OPEN_ENDED, false},
  {SPDM_RESPONDER_TEST_GROUP_END_SESSION_ACK, SPDM_RESPONDER_TEST_CASE_END_SESSION_ACK_SESSION_REQUIRED,                     SPDM_MESSAGE_VERSION_12, SPDM_MESSAGE_VERSION_12, false},
};

// Requester context of the SPDM group currently under test, plus the
// spdm_version requested in the [Main] section of the ini file.
static void *m_spdm_active_context = NULL;
static uint8_t m_spdm_ini_version = TEEIO_SPDM_VERSION_AUTO;

void spdm_test_set_active_context(void *spdm_context, uint8_t ini_spdm_version)
{
  m_spdm_active_context = spdm_context;
  m_spdm_ini_version = ini_spdm_version;
}

void spdm_test_clear_active_context(void)
{
  m_spdm_active_context = NULL;
  m_spdm_ini_version = TEEIO_SPDM_VERSION_AUTO;
}

const spdm_test_case_version_info_t* spdm_test_get_case_version_info(int group_id, int case_id)
{
  for(size_t i = 0; i < sizeof(m_spdm_case_version_info)/sizeof(m_spdm_case_version_info[0]); i++) {
    if(m_spdm_case_version_info[i].group_id == group_id && m_spdm_case_version_info[i].case_id == case_id) {
      return &m_spdm_case_version_info[i];
    }
  }
  return NULL;
}

void spdm_test_format_version(uint8_t version, char *buf, size_t size)
{
  if(version == 0) {
    snprintf(buf, size, "n/a");
  } else {
    snprintf(buf, size, "%u.%u", version >> 4, version & 0x0f);
  }
}

void spdm_test_format_version_range(const spdm_test_case_version_info_t *info, char *buf, size_t size)
{
  char min_str[8], max_str[8];

  if(info == NULL) {
    snprintf(buf, size, "unknown");
    return;
  }

  spdm_test_format_version(info->min_version, min_str, sizeof(min_str));
  if(info->max_version == OPEN_ENDED) {
    snprintf(buf, size, "%s+", min_str);
  } else if(info->max_version == info->min_version) {
    snprintf(buf, size, "%s only", min_str);
  } else {
    spdm_test_format_version(info->max_version, max_str, sizeof(max_str));
    snprintf(buf, size, "%s-%s", min_str, max_str);
  }
}

// Returns the version stored in libspdm's connection state, or 0 if none.
// Hand-crafted requests sent by some cases do not update this value.
static uint8_t get_connection_version(void)
{
  libspdm_data_parameter_t parameter;
  spdm_version_number_t spdm_version = 0;
  size_t data_size = sizeof(spdm_version);
  libspdm_return_t status;

  if(m_spdm_active_context == NULL) {
    return 0;
  }

  libspdm_zero_mem(&parameter, sizeof(parameter));
  parameter.location = LIBSPDM_DATA_LOCATION_CONNECTION;
  status = libspdm_get_data(m_spdm_active_context, LIBSPDM_DATA_SPDM_VERSION, &parameter, &spdm_version, &data_size);
  if(LIBSPDM_STATUS_IS_ERROR(status) || data_size != sizeof(spdm_version)) {
    return 0;
  }

  return (uint8_t)(spdm_version >> SPDM_VERSION_NUMBER_SHIFT_BIT);
}

/**
 * Version.1 assertion 5 enumerates every VersionNumberEntry the responder
 * advertises and FAILs any entry the bundled suite does not recognize. At the
 * pinned revision the suite knows 1.0..1.3 only, so a responder that also
 * advertises a newer version (e.g. 1.4) fails this assertion even though the
 * entry is legitimate. The message format is fixed by the pinned suite:
 *   "response version_number_entry - 0x%04x"
 */
bool spdm_test_detect_version_coverage_gap(
  int group_id, int case_id, int assertion_id,
  teeio_test_result_t result, const char *message,
  char *note, size_t note_size)
{
  const char *hex;
  char *end = NULL;
  unsigned long entry;
  uint8_t version;
  char ver_str[8], max_str[8];

  if(result != TEEIO_TEST_RESULT_FAILED ||
     group_id != SPDM_RESPONDER_TEST_GROUP_VERSION ||
     case_id != SPDM_RESPONDER_TEST_CASE_VERSION_SUCCESS_10 ||
     assertion_id != 5 ||
     message == NULL) {
    return false;
  }

  hex = strstr(message, "0x");
  if(hex == NULL) {
    return false;
  }
  entry = strtoul(hex, &end, 16);
  if(end == hex || entry > 0xFFFF) {
    return false;
  }

  version = (uint8_t)(entry >> SPDM_VERSION_NUMBER_SHIFT_BIT);
  if(version <= SPDM_TEST_SUITE_MAX_KNOWN_VERSION) {
    return false;
  }

  spdm_test_format_version(version, ver_str, sizeof(ver_str));
  spdm_test_format_version(SPDM_TEST_SUITE_MAX_KNOWN_VERSION, max_str, sizeof(max_str));
  snprintf(note, note_size,
           "responder advertises SPDM %s (entry 0x%04lx); the bundled SPDM-Responder-Validator "
           "only recognizes versions up to %s, so Assertion 1.1.5 fails for this entry regardless "
           "of device conformance. Re-check once the suite adds %s support.",
           ver_str, entry, max_str, ver_str);
  return true;
}

void spdm_test_annotate_failed_case(
  int group_id, int case_id, int case_class, int assertion_id,
  teeio_test_result_t result, const char *message)
{
  const spdm_test_case_version_info_t *info;
  char range_str[32], conn_str[8], ini_str[8], gap_note[MAX_LINE_LENGTH];
  uint8_t conn_version;

  if(result != TEEIO_TEST_RESULT_FAILED) {
    return;
  }

  // Annotate once per case, on its first failing assertion.
  if(!teeio_case_has_diagnostic_note(case_class, case_id)) {
    info = spdm_test_get_case_version_info(group_id, case_id);
    conn_version = get_connection_version();

    spdm_test_format_version_range(info, range_str, sizeof(range_str));
    spdm_test_format_version(conn_version, conn_str, sizeof(conn_str));
    if(m_spdm_ini_version == TEEIO_SPDM_VERSION_AUTO) {
      snprintf(ini_str, sizeof(ini_str), "auto");
    } else {
      spdm_test_format_version(m_spdm_ini_version, ini_str, sizeof(ini_str));
    }

    teeio_record_case_diagnostic_note(case_class, case_id,
      "case applies to SPDM %s%s; libspdm connection version at first failure: %s "
      "(may not reflect hand-crafted requests); ini spdm_version=%s "
      "(SPDM conformance cases negotiate their own version; see doc/spdm_test/spdm_test.md)",
      range_str,
      (info != NULL && info->sends_non_negotiated_version) ?
        "; case deliberately sends non-negotiated SPDM versions" : "",
      conn_str, ini_str);
  }

  if(spdm_test_detect_version_coverage_gap(group_id, case_id, assertion_id, result, message, gap_note, sizeof(gap_note))) {
    teeio_flag_suite_coverage_gap(case_class, case_id, "%s", gap_note);
  }
}

void spdm_test_case_begin(void)
{
  teeio_spdm_trace_reset();
}

static void format_advertised_versions(const teeio_spdm_trace_t *trace, char *buf, size_t size)
{
  size_t offset = 0;

  buf[0] = '\0';
  for(uint8_t i = 0; i < trace->version_count && offset < size; i++) {
    char version_str[8];
    spdm_test_format_version(trace->versions[i], version_str, sizeof(version_str));
    offset += (size_t)snprintf(buf + offset, size - offset, "%s%s", i == 0 ? "" : ", ", version_str);
  }
  if(trace->version_count == 0) {
    snprintf(buf, size, "none");
  }
}

static bool any_advertised_version_in_range(const teeio_spdm_trace_t *trace, const spdm_test_case_version_info_t *info)
{
  for(uint8_t i = 0; i < trace->version_count; i++) {
    uint8_t version = trace->versions[i];
    if(version >= info->min_version && (info->max_version == OPEN_ENDED || version <= info->max_version)) {
      return true;
    }
  }
  return false;
}

/**
 * Explain why an SPDM case's setup returned false. The reason is informational:
 * the case is still reported as skipped and no result is changed.
 *
 * The upstream setup functions only return a bool, so the reason is rebuilt from
 * the SPDM traffic seen by the DOE layer while the setup ran:
 *  1. The last response was an SPDM ERROR: the responder rejected a request.
 *  2. No version the responder advertises is in the case's applicability range:
 *     the case cannot run on this responder (expected skip).
 *  3. Otherwise the cause is not visible here. If the responder advertises a
 *     version newer than the bundled suite recognizes, flag a suite coverage gap,
 *     because the upstream setup rejects unrecognized versions.
 */
void spdm_test_case_setup_failed(int case_class, int case_id)
{
  teeio_spdm_trace_t trace;
  const spdm_test_case_version_info_t *info;
  char versions_str[64], range_str[32], max_known_str[8];
  bool has_newer_version = false;

  info = spdm_test_get_case_version_info(spdm_test_lib_get_group_id(case_class), case_id);
  teeio_spdm_trace_get(&trace);
  format_advertised_versions(&trace, versions_str, sizeof(versions_str));

  for(uint8_t i = 0; i < trace.version_count; i++) {
    if(trace.versions[i] > SPDM_TEST_SUITE_MAX_KNOWN_VERSION) {
      has_newer_version = true;
    }
  }

  if(trace.last_response_is_error) {
    teeio_record_case_skip_reason(case_class, case_id, TEEIO_CASE_SKIP_SETUP_FAILED,
      "setup failed: responder answered %s with ERROR(%s) (ErrorCode 0x%02x, ErrorData 0x%02x)",
      teeio_spdm_request_code_name(trace.error_request_code),
      teeio_spdm_error_code_name(trace.error_code), trace.error_code, trace.error_data);
    return;
  }

  if(trace.has_versions && info != NULL && !any_advertised_version_in_range(&trace, info)) {
    spdm_test_format_version_range(info, range_str, sizeof(range_str));
    teeio_record_case_skip_reason(case_class, case_id, TEEIO_CASE_SKIP_PRECONDITION,
      "setup precondition not met: case applies to SPDM %s but the responder advertises %s",
      range_str, versions_str);
    return;
  }

  teeio_record_case_skip_reason(case_class, case_id, TEEIO_CASE_SKIP_SETUP_FAILED,
    "setup failed without an SPDM ERROR response (communication failure, or a precondition the "
    "responder does not meet such as a required capability); rerun with libspdm_log=1 for the message trace");

  if(has_newer_version) {
    spdm_test_format_version(SPDM_TEST_SUITE_MAX_KNOWN_VERSION, max_known_str, sizeof(max_known_str));
    teeio_flag_suite_coverage_gap(case_class, case_id,
      "responder advertises SPDM %s; the bundled SPDM-Responder-Validator only recognizes versions up to %s, "
      "so this case's setup may have been rejected for that reason rather than by the device. "
      "Re-check once the suite supports the newer version.",
      versions_str, max_known_str);
  }
}
