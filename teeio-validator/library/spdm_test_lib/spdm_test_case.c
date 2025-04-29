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

extern TEEIO_DEBUG_LEVEL g_debug_level;

typedef struct {
  int teeio_spdm_case_class;
  int spdm_responder_group_id;
  common_test_case_t *spdm_responder_test_case;
} teeio_map_to_spdm_responder_test_group_t;

// refer to spdm_responder_conformance_test_lib
extern common_test_case_t m_spdm_test_group_version[];
extern common_test_case_t m_spdm_test_group_capabilities[];
extern common_test_case_t m_spdm_test_group_algorithms[];
extern common_test_case_t m_spdm_test_group_digests[];
extern common_test_case_t m_spdm_test_group_certificate[];
extern common_test_case_t m_spdm_test_group_challenge_auth[];
extern common_test_case_t m_spdm_test_group_measurements[];
extern common_test_case_t m_spdm_test_group_key_exchange_rsp[];
extern common_test_case_t m_spdm_test_group_finish_rsp[];
extern common_test_case_t m_spdm_test_group_heartbeat_ack[];
extern common_test_case_t m_spdm_test_group_key_update_ack[];
extern common_test_case_t m_spdm_test_group_end_session_ack[];

// SPDM_TEST_CASE_xxx maps to SPDM_RESPONDER_TEST_GROUP_xxx
// refer to "library/spdm_responder_conformance_test_lib.h"
teeio_map_to_spdm_responder_test_group_t m_teeio_map_spdm_responder_test_groups[SPDM_TEST_CASE_NUM] = {
  {SPDM_TEST_CASE_VERSION,          SPDM_RESPONDER_TEST_GROUP_VERSION,          m_spdm_test_group_version           },
  {SPDM_TEST_CASE_CAPABILITIES,     SPDM_RESPONDER_TEST_GROUP_CAPABILITIES,     m_spdm_test_group_capabilities      },
  {SPDM_TEST_CASE_ALGORITHMS,       SPDM_RESPONDER_TEST_GROUP_ALGORITHMS,       m_spdm_test_group_algorithms        },
  {SPDM_TEST_CASE_CERTIFICATE,      SPDM_RESPONDER_TEST_GROUP_CERTIFICATE,      m_spdm_test_group_certificate       },
  {SPDM_TEST_CASE_MEASUREMENTS,     SPDM_RESPONDER_TEST_GROUP_MEASUREMENTS,     m_spdm_test_group_measurements      },
  {SPDM_TEST_CASE_KEY_EXCHANGE_RSP, SPDM_RESPONDER_TEST_GROUP_KEY_EXCHANGE_RSP, m_spdm_test_group_key_exchange_rsp  },
  {SPDM_TEST_CASE_FINISH_RSP,       SPDM_RESPONDER_TEST_GROUP_FINISH_RSP,       m_spdm_test_group_finish_rsp        },
  {SPDM_TEST_CASE_END_SESSION_ACK,  SPDM_RESPONDER_TEST_GROUP_END_SESSION_ACK,  m_spdm_test_group_end_session_ack   },
};

// the second column will be populated in spdm_test_lib_init_test_cases()
ide_test_case_name_t m_spdm_test_case_names[] = {
  {"Version",           NULL,    SPDM_TEST_CASE_VERSION          },
  {"Capabilities",      NULL,    SPDM_TEST_CASE_CAPABILITIES     },
  {"Algorithms",        NULL,    SPDM_TEST_CASE_ALGORITHMS       },
  {"Certificate",       NULL,    SPDM_TEST_CASE_CERTIFICATE      },
  {"Measurements",      NULL,    SPDM_TEST_CASE_MEASUREMENTS     },
  {"KeyExchangeRsp",    NULL,    SPDM_TEST_CASE_KEY_EXCHANGE_RSP },
  {"FinishRsp",         NULL,    SPDM_TEST_CASE_FINISH_RSP       },
  {"EndSessionAck",     NULL,    SPDM_TEST_CASE_END_SESSION_ACK  },
  {NULL,                NULL,    SPDM_TEST_CASE_NUM              }
};

// It will be populated in spdm_test_lib_init_test_cases()
TEEIO_TEST_CASES m_spdm_test_case_funcs[SPDM_TEST_CASE_NUM] = {0};

static bool gen_teeio_spdm_test_case_from_responder_test_case(
  common_test_case_t *responder_spdm_test_case,
  char** case_ids, int* max_case_id,
  ide_test_case_funcs_t** teeio_test_case_funcs)
{
  char buf[MAX_LINE_LENGTH] = {0};
  int buf_offset = 0;
  int max_val = 0;
  int i = 0;

  common_test_case_t* ptr_responder = responder_spdm_test_case;
  while(ptr_responder->case_id != COMMON_TEST_ID_END) {
    i++;
    ptr_responder = responder_spdm_test_case + i;
  }

  if(i == 0) {
    TEEIO_ASSERT(false);
    return false;
  }

  ide_test_case_funcs_t* teeio_tc_funcs = (ide_test_case_funcs_t*)malloc(sizeof(ide_test_case_funcs_t) * i);
  memset(teeio_tc_funcs, 0, sizeof(ide_test_case_funcs_t) * i);

  i = 0;
  ptr_responder = responder_spdm_test_case;

  while(ptr_responder->case_id != COMMON_TEST_ID_END) {
    ide_test_case_funcs_t* ptr_teeio = teeio_tc_funcs + i;
    ptr_teeio->config_check_required = false;
    ptr_teeio->run = ptr_responder->case_func;
    ptr_teeio->setup = ptr_responder->case_setup_func;
    ptr_teeio->teardown = ptr_responder->case_teardown_func;

    if(ptr_responder->case_id > max_val) {
      max_val = ptr_responder->case_id;     
    }

    if(buf_offset + strlen(buf) + 4 + 1 > sizeof(buf)) {
      TEEIO_ASSERT(false);
      return false;
    }

    sprintf(buf + buf_offset, "%d,", ptr_responder->case_id);
    buf_offset = strlen(buf);

    i++;
    ptr_responder = responder_spdm_test_case + i;
  }

  buf[buf_offset - 1] = '\0'; // remove last comma

  *case_ids = (char*)malloc(strlen(buf) + 1);
  if(*case_ids == NULL) {
    TEEIO_ASSERT(false);
    return false;
  }
  strcpy(*case_ids, buf);
  *max_case_id = max_val;
  *teeio_test_case_funcs = teeio_tc_funcs;

  return true;
}

// Initialize m_spdm_test_case_funcs & m_spdm_test_case_names 
// based on spdm_responder_conformance_test_lib
void spdm_test_lib_init_test_cases()
{
  char *case_ids = NULL;
  int max_case_id = 0;

  // Walk thru m_teeio_map_spdm_responder_test_groups
  for(int i = 0; i < SPDM_TEST_CASE_NUM; i++) {
    TEEIO_TEST_CASES *teeio_test_cases = m_spdm_test_case_funcs + i;
    ide_test_case_name_t *teeio_test_case_name = m_spdm_test_case_names + i;
    teeio_map_to_spdm_responder_test_group_t *teeio_map_responder = m_teeio_map_spdm_responder_test_groups + i;

    if(gen_teeio_spdm_test_case_from_responder_test_case(
        teeio_map_responder->spdm_responder_test_case,
        &case_ids, &max_case_id,
        &teeio_test_cases->funcs)) {

      teeio_test_case_name->names = case_ids;
      teeio_test_cases->cnt = max_case_id;
    } else {
      // Error
      TEEIO_ASSERT(false);
    }
  }
}

int spdm_test_lib_get_case_class(int responder_group_id)
{
  int case_class = SPDM_TEST_CASE_NUM;
  for(int i = 0; i < SPDM_TEST_CASE_NUM; i++) {
    if (m_teeio_map_spdm_responder_test_groups[i].spdm_responder_group_id == responder_group_id) {
      case_class = m_teeio_map_spdm_responder_test_groups[i].teeio_spdm_case_class;
      break;
    }
  }

  return case_class;
}