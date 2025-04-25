/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "helperlib.h"
#include "ide_test.h"
#include "spdm_test_common.h"
#include "library/common_test_utility_lib.h"
#include "library/spdm_responder_conformance_test_lib.h"

// SPDM supported config items
const char* m_spdm_test_configuration_name[] = {
  "default"
};

uint32_t m_spdm_config_bitmask = SPDM_TEST_CONFIGURATION_BITMASK;

typedef struct {
  int teeio_spdm_case_class;
  int spdm_responder_group_id;
} teeio_spdm_responder_spdm_group_id_map_t;

// SPDM_TEST_CASE_xxx maps to SPDM_RESPONDER_TEST_GROUP_xxx
// refer to "library/spdm_responder_conformance_test_lib.h"
teeio_spdm_responder_spdm_group_id_map_t m_teeio_spdm_responder_spdm_group_id_maps[SPDM_TEST_CASE_NUM] = {
  {SPDM_TEST_CASE_VERSION, SPDM_RESPONDER_TEST_GROUP_VERSION},
  {SPDM_TEST_CASE_CAPABILITIES, SPDM_RESPONDER_TEST_GROUP_CAPABILITIES},
  {SPDM_TEST_CASE_ALGORITHMS, SPDM_RESPONDER_TEST_GROUP_ALGORITHMS},
  {SPDM_TEST_CASE_CERTIFICATE, SPDM_RESPONDER_TEST_GROUP_CERTIFICATE},
  {SPDM_TEST_CASE_MEASUREMENTS, SPDM_RESPONDER_TEST_GROUP_MEASUREMENTS},
  {SPDM_TEST_CASE_KEY_EXCHANGE_RSP, SPDM_RESPONDER_TEST_GROUP_KEY_EXCHANGE_RSP},
  {SPDM_TEST_CASE_FINISH_RSP, SPDM_RESPONDER_TEST_GROUP_FINISH_RSP},
  {SPDM_TEST_CASE_END_SESSION_ACK, SPDM_RESPONDER_TEST_GROUP_END_SESSION_ACK}
};
  
ide_test_config_funcs_t m_spdm_test_config_funcs[SPDM_TEST_CONFIGURATION_TYPE_NUM] = {
  {
    // Default Config
    spdm_test_config_default_enable,
    spdm_test_config_default_disable,
    spdm_test_config_default_support,
    spdm_test_config_default_check
  }
};

ide_test_group_funcs_t m_spdm_test_group_funcs = {
  spdm_test_group_setup,
  spdm_test_group_teardown
};

ide_test_case_name_t m_spdm_test_case_names[] = {
  {"Version",           "1",                          SPDM_TEST_CASE_VERSION          },
  {"Capabilities",      "1,2,3,4,5,6",                SPDM_TEST_CASE_CAPABILITIES     },
  {"Algorithms",        "1,2,3,4,5,6,7",              SPDM_TEST_CASE_ALGORITHMS       },
  {"Certificate",       "1,2,3,4",                    SPDM_TEST_CASE_CERTIFICATE      },
  {"Measurements",      "1,2,3,4,5,6,7,8,9,10",       SPDM_TEST_CASE_MEASUREMENTS     },
  {"KeyExchangeRsp",    "1,2,3,4,5,6,7,8",            SPDM_TEST_CASE_KEY_EXCHANGE_RSP },
  {"FinishRsp",         "1,2,3,4,5,6,7,8,9,10,11",    SPDM_TEST_CASE_FINISH_RSP       },
  {"EndSessionAck",     "1,2,3,4",                    SPDM_TEST_CASE_END_SESSION_ACK  },
  {NULL,                NULL,                         SPDM_TEST_CASE_NUM              }
};

ide_test_case_funcs_t m_spdm_test_version_cases[MAX_SPDM_TEST_VERSION_CASE_ID] = {
  {spdm_test_version_1_setup, spdm_test_version_1_run, spdm_test_version_1_teardown, false}
};

ide_test_case_funcs_t m_spdm_test_capabilities_cases[MAX_SPDM_TEST_CAPABILITIES_CASE_ID] = {
  {spdm_test_capabilities_1_setup, spdm_test_capabilities_1_run, spdm_test_capabilities_1_teardown, false},
  {spdm_test_capabilities_2_setup, spdm_test_capabilities_2_run, spdm_test_capabilities_2_teardown, false},
  {spdm_test_capabilities_3_setup, spdm_test_capabilities_3_run, spdm_test_capabilities_3_teardown, false},
  {spdm_test_capabilities_4_setup, spdm_test_capabilities_4_run, spdm_test_capabilities_4_teardown, false},
  {spdm_test_capabilities_5_setup, spdm_test_capabilities_5_run, spdm_test_capabilities_5_teardown, false},
  {spdm_test_capabilities_6_setup, spdm_test_capabilities_6_run, spdm_test_capabilities_6_teardown, false}
};

ide_test_case_funcs_t m_spdm_test_algorithms_cases[MAX_SPDM_TEST_ALGORITHMS_CASE_ID] = {
  {spdm_test_algorithms_1_setup, spdm_test_algorithms_1_run, spdm_test_algorithms_1_teardown, false},
  {spdm_test_algorithms_2_setup, spdm_test_algorithms_2_run, spdm_test_algorithms_2_teardown, false},
  {spdm_test_algorithms_3_setup, spdm_test_algorithms_3_run, spdm_test_algorithms_3_teardown, false},
  {spdm_test_algorithms_4_setup, spdm_test_algorithms_4_run, spdm_test_algorithms_4_teardown, false},
  {spdm_test_algorithms_5_setup, spdm_test_algorithms_5_run, spdm_test_algorithms_5_teardown, false},
  {spdm_test_algorithms_6_setup, spdm_test_algorithms_6_run, spdm_test_algorithms_6_teardown, false},
  {spdm_test_algorithms_7_setup, spdm_test_algorithms_7_run, spdm_test_algorithms_7_teardown, false}
};

ide_test_case_funcs_t m_spdm_test_certificate_cases[MAX_SPDM_TEST_CERTIFICATE_CASE_ID] = {
  {spdm_test_certificate_1_setup, spdm_test_certificate_1_run, spdm_test_certificate_1_teardown, false},
  {spdm_test_certificate_2_setup, spdm_test_certificate_2_run, spdm_test_certificate_2_teardown, false},
  {spdm_test_certificate_3_setup, spdm_test_certificate_3_run, spdm_test_certificate_3_teardown, false},
  {spdm_test_certificate_4_setup, spdm_test_certificate_4_run, spdm_test_certificate_4_teardown, false}
};

ide_test_case_funcs_t m_spdm_test_measurements_cases[MAX_SPDM_TEST_MEASUREMENTS_CASE_ID] = {
  {spdm_test_measurements_1_setup, spdm_test_measurements_1_run, spdm_test_measurements_1_teardown, false},
  {spdm_test_measurements_2_setup, spdm_test_measurements_2_run, spdm_test_measurements_2_teardown, false},
  {spdm_test_measurements_3_setup, spdm_test_measurements_3_run, spdm_test_measurements_3_teardown, false},
  {spdm_test_measurements_4_setup, spdm_test_measurements_4_run, spdm_test_measurements_4_teardown, false},
  {spdm_test_measurements_5_setup, spdm_test_measurements_5_run, spdm_test_measurements_5_teardown, false},
  {spdm_test_measurements_6_setup, spdm_test_measurements_6_run, spdm_test_measurements_6_teardown, false},
  {spdm_test_measurements_7_setup, spdm_test_measurements_7_run, spdm_test_measurements_7_teardown, false},
  {spdm_test_measurements_8_setup, spdm_test_measurements_8_run, spdm_test_measurements_8_teardown, false},
  {spdm_test_measurements_9_setup, spdm_test_measurements_9_run, spdm_test_measurements_9_teardown, false},
  {spdm_test_measurements_10_setup, spdm_test_measurements_10_run, spdm_test_measurements_10_teardown, false}
};

ide_test_case_funcs_t m_spdm_test_key_exchange_cases[MAX_SPDM_TEST_KEY_EXCHANGE_CASE_ID] = {
  {spdm_test_key_exchange_1_setup, spdm_test_key_exchange_1_run, spdm_test_key_exchange_1_teardown, false},
  {spdm_test_key_exchange_2_setup, spdm_test_key_exchange_2_run, spdm_test_key_exchange_2_teardown, false},
  {spdm_test_key_exchange_3_setup, spdm_test_key_exchange_3_run, spdm_test_key_exchange_3_teardown, false},
  {spdm_test_key_exchange_4_setup, spdm_test_key_exchange_4_run, spdm_test_key_exchange_4_teardown, false},
  {spdm_test_key_exchange_5_setup, spdm_test_key_exchange_5_run, spdm_test_key_exchange_5_teardown, false},
  {spdm_test_key_exchange_6_setup, spdm_test_key_exchange_6_run, spdm_test_key_exchange_6_teardown, false},
  {spdm_test_key_exchange_7_setup, spdm_test_key_exchange_7_run, spdm_test_key_exchange_7_teardown, false},
  {spdm_test_key_exchange_8_setup, spdm_test_key_exchange_8_run, spdm_test_key_exchange_8_teardown, false}
};

ide_test_case_funcs_t m_spdm_test_finish_cases[MAX_SPDM_TEST_FINISH_CASE_ID] = {
  {spdm_test_finish_1_setup, spdm_test_finish_1_run, spdm_test_finish_1_teardown, false},
  {spdm_test_finish_2_setup, spdm_test_finish_2_run, spdm_test_finish_2_teardown, false},
  {spdm_test_finish_3_setup, spdm_test_finish_3_run, spdm_test_finish_3_teardown, false},
  {spdm_test_finish_4_setup, spdm_test_finish_4_run, spdm_test_finish_4_teardown, false},
  {spdm_test_finish_5_setup, spdm_test_finish_5_run, spdm_test_finish_5_teardown, false},
  {spdm_test_finish_6_setup, spdm_test_finish_6_run, spdm_test_finish_6_teardown, false},
  {spdm_test_finish_7_setup, spdm_test_finish_7_run, spdm_test_finish_7_teardown, false},
  {spdm_test_finish_8_setup, spdm_test_finish_8_run, spdm_test_finish_8_teardown, false},
  {spdm_test_finish_9_setup, spdm_test_finish_9_run, spdm_test_finish_9_teardown, false},
  {spdm_test_finish_10_setup, spdm_test_finish_10_run, spdm_test_finish_10_teardown, false},
  {spdm_test_finish_11_setup, spdm_test_finish_11_run, spdm_test_finish_11_teardown, false}
};

ide_test_case_funcs_t m_spdm_test_end_session_cases[MAX_SPDM_TEST_END_SESSION_CASE_ID] = {
  {spdm_test_end_session_1_setup, spdm_test_end_session_1_run, spdm_test_end_session_1_teardown, false},
  {spdm_test_end_session_2_setup, spdm_test_end_session_2_run, spdm_test_end_session_2_teardown, false},
  {spdm_test_end_session_3_setup, spdm_test_end_session_3_run, spdm_test_end_session_3_teardown, false},
  {spdm_test_end_session_4_setup, spdm_test_end_session_4_run, spdm_test_end_session_4_teardown, false}
};

TEEIO_TEST_CASES m_spdm_test_case_funcs[SPDM_TEST_CASE_NUM] = {
  {m_spdm_test_version_cases,       MAX_SPDM_TEST_VERSION_CASE_ID       },
  {m_spdm_test_capabilities_cases,  MAX_SPDM_TEST_CAPABILITIES_CASE_ID  },
  {m_spdm_test_algorithms_cases,    MAX_SPDM_TEST_ALGORITHMS_CASE_ID    },
  {m_spdm_test_certificate_cases,   MAX_SPDM_TEST_CERTIFICATE_CASE_ID   },
  {m_spdm_test_measurements_cases,  MAX_SPDM_TEST_MEASUREMENTS_CASE_ID  },
  {m_spdm_test_key_exchange_cases,  MAX_SPDM_TEST_KEY_EXCHANGE_CASE_ID  },
  {m_spdm_test_finish_cases,        MAX_SPDM_TEST_FINISH_CASE_ID        },
  {m_spdm_test_end_session_cases,   MAX_SPDM_TEST_END_SESSION_CASE_ID   },
};

static const char* get_test_configuration_name (int configuration_type)
{
  if(configuration_type > sizeof(m_spdm_test_configuration_name)/sizeof(const char*)) {
    return NULL;
  }

  return m_spdm_test_configuration_name[configuration_type];
}

static uint32_t get_test_configuration_bitmask (int top_tpye)
{
  return m_spdm_config_bitmask;
}

static ide_test_config_funcs_t* get_test_configuration_funcs (int top_type, int configuration_type)
{
  return &m_spdm_test_config_funcs[configuration_type];
}

static ide_test_group_funcs_t* get_test_group_funcs (int top_type)
{
  return &m_spdm_test_group_funcs;
}

static ide_test_case_funcs_t* get_test_case_funcs (int case_class, int case_id)
{
  TEEIO_ASSERT(case_class < SPDM_TEST_CASE_NUM);
  TEEIO_TEST_CASES* test_cases = &m_spdm_test_case_funcs[case_class];

  TEEIO_ASSERT(case_id < test_cases->cnt);
  return &test_cases->funcs[case_id];
}

static ide_test_case_name_t* get_test_case_name (int case_class)
{
  TEEIO_ASSERT(case_class < SPDM_TEST_CASE_NUM + 1);
  return &m_spdm_test_case_names[case_class];
}

static void* alloc_spdm_test_group_context(void)
{
  spdm_test_group_context_t* context = (spdm_test_group_context_t*)malloc(sizeof(spdm_test_group_context_t));
  TEEIO_ASSERT(context);
  memset(context, 0, sizeof(spdm_test_group_context_t));
  context->common.signature = GROUP_CONTEXT_SIGNATURE;

  return context;
}

static bool spdm_test_check_configuration_bitmap(uint32_t* bitmap)
{
  // default config is always set
  *bitmap |= BIT_MASK(SPDM_TEST_CONFIGURATION_TYPE_DEFAULT);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "spdm_test configuration bitmap=0x%08x\n", *bitmap));

  return true;
}

bool spdm_test_lib_register_test_suite_funcs(teeio_test_funcs_t* funcs)
{
  TEEIO_ASSERT(funcs);

  funcs->get_case_funcs_func = get_test_case_funcs;
  funcs->get_case_name_func = get_test_case_name;
  funcs->get_configuration_bitmask_func = get_test_configuration_bitmask;
  funcs->get_configuration_funcs_func = get_test_configuration_funcs;
  funcs->get_configuration_name_func = get_test_configuration_name;
  funcs->get_group_funcs_func = get_test_group_funcs;
  funcs->alloc_test_group_context_func = alloc_spdm_test_group_context;
  funcs->check_configuration_bitmap_func = spdm_test_check_configuration_bitmap;

  return true;
}

void common_test_record_test_assertion (
  common_test_group_id group_id,
  common_test_case_id case_id,
  common_test_assertion_id assertion_id,
  common_test_result_t test_result,
  const char *message_format,
  ...)
{
  char buffer[MAX_LINE_LENGTH] = {0};
  va_list marker;

  int case_class = SPDM_TEST_CASE_NUM;
  for(int i = 0; i < SPDM_TEST_CASE_NUM; i++) {
    if (m_teeio_spdm_responder_spdm_group_id_maps[i].spdm_responder_group_id == group_id) {
      case_class = m_teeio_spdm_responder_spdm_group_id_maps[i].teeio_spdm_case_class;
      break;
    }
  }

  TEEIO_ASSERT(case_class < SPDM_TEST_CASE_NUM);

  if (message_format != NULL) {
      va_start(marker, message_format);

      vsnprintf(buffer, sizeof(buffer), message_format, marker);

      va_end(marker);
  }

  teeio_record_assertion_result(
    case_class, case_id, assertion_id,
    IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
    (teeio_test_result_t)test_result, 
    "%s", buffer);
}

void common_test_run_test_suite (
  void *test_context,
  const common_test_suite_t *test_suite,
  const common_test_suite_config_t *test_suite_config)
{
  TEEIO_ASSERT(false);
}

void common_test_record_test_message(const char *message_format, ...)
{
  char buffer[MAX_LINE_LENGTH] = {0};
  va_list marker;

  if (message_format != NULL) {
      va_start(marker, message_format);

      vsnprintf(buffer, sizeof(buffer), message_format, marker);

      va_end(marker);
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%s\n", buffer));
}

void* spdm_test_get_spdm_context_from_test_context(void *test_context)
{
  TEEIO_ASSERT(test_context);

  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  spdm_test_group_context_t *group_context = (spdm_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);
  TEEIO_ASSERT(group_context->spdm_doe.spdm_context);

  return group_context->spdm_doe.spdm_context;
}
