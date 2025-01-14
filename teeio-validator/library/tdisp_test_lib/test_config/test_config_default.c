/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdbool.h>

bool test_config_enable_common (void *test_context);
bool test_config_check_common (void *test_context, const char *assertion_msg);
bool test_config_support_common (void *test_context);

bool tdisp_test_config_default_enable_common (void *test_context)
{
	return test_config_enable_common (test_context);
}

bool tdisp_test_config_default_disable_common (void *test_context)
{
	return true;
}

bool tdisp_test_config_default_support_common (void *test_context)
{
	return test_config_support_common (test_context);
}

bool tdisp_test_config_default_check_common (void *test_context)
{
	return test_config_check_common (test_context, "Check Common Assertion");
}
