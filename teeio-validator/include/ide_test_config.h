/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __IDE_TEST_CONFIG_H__
#define __IDE_TEST_CONFIG_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/param.h>

// default config
bool test_config_default_enable_common(void *test_context);
bool test_config_default_disable_common(void *test_context);
bool test_config_default_support_common(void *test_context);
bool test_config_default_check_common(void *test_context);

// test selective_and_link_ide with default config
bool test_config_default_enable_sel_link(void *test_context);
bool test_config_default_disable_sel_link(void *test_context);
bool test_config_default_support_sel_link(void *test_context);
bool test_config_default_check_sel_link(void *test_context);

// pcrc
bool test_config_pcrc_enable_sel(void *test_context);
bool test_config_pcrc_disable_sel(void *test_context);
bool test_config_pcrc_support_sel(void *test_context);
bool test_config_pcrc_check_sel(void *test_context);

bool test_config_pcrc_enable_link(void *test_context);
bool test_config_pcrc_disable_link(void *test_context);
bool test_config_pcrc_support_link(void *test_context);
bool test_config_pcrc_check_link(void *test_context);

bool test_config_pcrc_enable_sel_link(void *test_context);
bool test_config_pcrc_disable_sel_link(void *test_context);
bool test_config_pcrc_support_sel_link(void *test_context);
bool test_config_pcrc_check_sel_link(void *test_context);

// selective_ide for configuration
bool test_config_sel_ide_for_config_req_enable(void *test_context);
bool test_config_sel_ide_for_config_req_disable(void *test_context);
bool test_config_sel_ide_for_config_req_support(void *test_context);
bool test_config_sel_ide_for_config_req_check(void *test_context);

#endif
