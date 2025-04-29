/**
 *  Copyright Notice:
 *  Copyright 2025 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>
#include "ide_test.h"
 
// test with default config
bool spdm_test_config_default_enable(void *test_context)
{
    return true;
}

bool spdm_test_config_default_disable(void *test_context)
{
    return true;
}

bool spdm_test_config_default_support(void *test_context)
{
    return true;
}

bool spdm_test_config_default_check(void *test_context)
{
    return true;
}
 