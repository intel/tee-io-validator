/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __IDE_TEST_GROUP_H__
#define __IDE_TEST_GROUP_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/param.h>

// selective_ide test group
bool test_group_setup_sel(void *test_context);
bool test_group_teardown_sel(void *test_context);

// link_ide test group
bool test_group_setup_link(void *test_context);
bool test_group_teardown_link(void *test_context);

// selective_and_link_ide test group
bool test_group_setup_sel_link(void *test_context);
bool test_group_teardown_sel_link(void *test_context);

#endif