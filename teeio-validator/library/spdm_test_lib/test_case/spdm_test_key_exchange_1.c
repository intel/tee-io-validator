/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>

#include "ide_test.h"
#include "spdm_test_common.h"
#include "library/common_test_utility_lib.h"

extern common_test_case_t m_spdm_test_group_key_exchange_rsp[];
static teeio_spdm_test_context_t m_spdm_test_context = {0};

bool spdm_test_key_exchange_1_setup(void *test_context)
{
  m_spdm_test_context.spdm_context = spdm_test_get_spdm_context_from_test_context(test_context);
  return m_spdm_test_group_key_exchange_rsp[0].case_setup_func(&m_spdm_test_context);
}

bool spdm_test_key_exchange_1_run(void *test_context)
{
  m_spdm_test_group_key_exchange_rsp[0].case_func(&m_spdm_test_context);
  return true;
}

bool spdm_test_key_exchange_1_teardown(void *test_context)
{
  m_spdm_test_group_key_exchange_rsp[0].case_teardown_func(&m_spdm_test_context);
  return true;
}
