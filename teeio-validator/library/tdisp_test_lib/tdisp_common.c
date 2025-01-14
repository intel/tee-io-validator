/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "tdisp_common.h"
#include "tdisp_test_lib.h"
#include "teeio_debug.h"

// PCIE-IDE supported config items
const char *m_tdisp_test_configuration_name[] = {
	"default",
	NULL
};

// TODO: Add link_ide
ide_test_config_funcs_t m_tdisp_config_func = {
	// selective_ide
	// Default Config
	tdisp_test_config_default_enable_common,
	tdisp_test_config_default_disable_common,
	tdisp_test_config_default_support_common,
	tdisp_test_config_default_check_common
};

ide_test_group_funcs_t m_tdisp_group_func = {
	// selective_ide
	tdisp_test_group_setup_sel, tdisp_test_group_teardown_sel
};

ide_test_case_name_t m_tdisp_test_case_names[] = {
	{"Query", "1,2", TDISP_TEST_CASE_QUERY},
	{"LockInterface", "1,2,3,4", TDISP_TEST_CASE_LOCK_INTERFACE},
	{"DeviceReport", "1,2,3,4,5", TDISP_TEST_CASE_DEVICE_REPORT},
	{"DeviceState", "1,2,3,4", TDISP_TEST_CASE_DEVICE_STATE},
	{"StartInterface", "1,2,3,4", TDISP_TEST_CASE_START_INTERFACE},
	{"StopInterface", "1,2,3", TDISP_TEST_CASE_STOP_INTERFACE},
	{NULL, NULL, TDISP_TEST_CASE_NUM}
};

ide_test_case_funcs_t m_tdisp_test_query_cases[MAX_TDISP_QUERY_CASE_ID] = {
	{
		tdisp_test_query_1_setup, tdisp_test_query_1_run,
		tdisp_test_query_1_teardown, true
	},
	{
		tdisp_test_query_2_setup, tdisp_test_query_2_run,
		tdisp_test_query_2_teardown, true
	},
};

ide_test_case_funcs_t
	m_tdisp_test_lock_interface_cases[MAX_LOCK_INTERFACE_RESPONSE_CASE_ID] = {
	{
		tdisp_test_lock_interface_1_setup, tdisp_test_lock_interface_1_run,
		tdisp_test_lock_interface_1_teardown, true
	},
	{
		tdisp_test_lock_interface_2_setup, tdisp_test_lock_interface_2_run,
		tdisp_test_lock_interface_2_teardown, true
	},
	{
		tdisp_test_lock_interface_3_setup, tdisp_test_lock_interface_3_run,
		tdisp_test_lock_interface_3_teardown, true
	},
	{
		tdisp_test_lock_interface_4_setup, tdisp_test_lock_interface_4_run,
		tdisp_test_lock_interface_4_teardown, true
	},
};

ide_test_case_funcs_t
	m_tdisp_test_interface_report_cases[MAX_DEVICE_INTERFACE_REPORT_CASE_ID] = {
	{
		tdisp_test_interface_report_1_setup, tdisp_test_interface_report_1_run,
		tdisp_test_interface_report_1_teardown, true
	},
	{
		tdisp_test_interface_report_2_setup, tdisp_test_interface_report_2_run,
		tdisp_test_interface_report_2_teardown, true
	},
	{
		tdisp_test_interface_report_3_setup, tdisp_test_interface_report_3_run,
		tdisp_test_interface_report_3_teardown, true
	},
	{
		tdisp_test_interface_report_4_setup, tdisp_test_interface_report_4_run,
		tdisp_test_interface_report_4_teardown, true
	},
	{
		tdisp_test_interface_report_5_setup, tdisp_test_interface_report_5_run,
		tdisp_test_interface_report_5_teardown, true
	},
};

ide_test_case_funcs_t
	m_tdisp_test_interface_state_cases[MAX_DEVICE_INTERFACE_STATE_CASE_ID] = {
	{
		tdisp_test_interface_state_1_setup, tdisp_test_interface_state_1_run,
		tdisp_test_interface_state_1_teardown, true
	},
	{
		tdisp_test_interface_state_2_setup, tdisp_test_interface_state_2_run,
		tdisp_test_interface_state_2_teardown, true
	},
	{
		tdisp_test_interface_state_3_setup, tdisp_test_interface_state_3_run,
		tdisp_test_interface_state_3_teardown, true
	},
	{
		tdisp_test_interface_state_4_setup, tdisp_test_interface_state_4_run,
		tdisp_test_interface_state_4_teardown, true
	},
};

ide_test_case_funcs_t
	m_tdisp_test_start_interface_cases[MAX_START_INTERFACE_RESPONSE_CASE_ID] = {
	{
		tdisp_test_start_interface_1_setup, tdisp_test_start_interface_1_run,
		tdisp_test_start_interface_1_teardown, true
	},
	{
		tdisp_test_start_interface_2_setup, tdisp_test_start_interface_2_run,
		tdisp_test_start_interface_2_teardown, true
	},
	{
		tdisp_test_start_interface_3_setup, tdisp_test_start_interface_3_run,
		tdisp_test_start_interface_3_teardown, true
	},
	{
		tdisp_test_start_interface_4_setup, tdisp_test_start_interface_4_run,
		tdisp_test_start_interface_4_teardown, true
	}
};

ide_test_case_funcs_t
	m_tdisp_test_stop_interface_cases[MAX_STOP_INTERFACE_RESPONSE_CASE_ID] = {
	{
		tdisp_test_stop_interface_1_setup, tdisp_test_stop_interface_1_run,
		tdisp_test_stop_interface_1_teardown, true
	},
	{
		tdisp_test_stop_interface_2_setup, tdisp_test_stop_interface_2_run,
		tdisp_test_stop_interface_2_teardown, true
	},
	{
		tdisp_test_stop_interface_3_setup, tdisp_test_stop_interface_3_run,
		tdisp_test_stop_interface_3_teardown, true
	},
};

TEEIO_TEST_CASES m_tdisp_test_case_funcs[TDISP_TEST_CASE_NUM] = {
	{m_tdisp_test_query_cases, MAX_TDISP_QUERY_CASE_ID},
	{m_tdisp_test_lock_interface_cases, MAX_LOCK_INTERFACE_RESPONSE_CASE_ID},
	{m_tdisp_test_interface_report_cases, MAX_DEVICE_INTERFACE_REPORT_CASE_ID},
	{m_tdisp_test_interface_state_cases, MAX_DEVICE_INTERFACE_STATE_CASE_ID},
	{m_tdisp_test_start_interface_cases, MAX_START_INTERFACE_RESPONSE_CASE_ID},
	{m_tdisp_test_stop_interface_cases, MAX_STOP_INTERFACE_RESPONSE_CASE_ID}
};


static const char* get_test_configuration_name (int configuration_type)
{
	if (configuration_type >
		sizeof (m_tdisp_test_configuration_name) / sizeof (const char*)) {
		return NULL;
	}

	return m_tdisp_test_configuration_name[configuration_type];
}

static uint32_t get_test_configuration_bitmask (int top_tpye)
{
	TEEIO_ASSERT (top_tpye == IDE_TEST_TOPOLOGY_TYPE_SEL_IDE);

	return (uint32_t) SELECTIVE_IDE_CONFIGURATION_BITMASK;
}

static ide_test_config_funcs_t* get_test_configuration_funcs (int top_type, int configuration_type)
{
	TEEIO_ASSERT (top_type == IDE_TEST_TOPOLOGY_TYPE_SEL_IDE &&
		configuration_type == IDE_TEST_CONFIGURATION_TYPE_DEFAULT);

	return &m_tdisp_config_func;
}

static ide_test_group_funcs_t* get_test_group_funcs (int top_type)
{
	TEEIO_ASSERT (top_type == IDE_TEST_TOPOLOGY_TYPE_SEL_IDE);

	return &m_tdisp_group_func;
}

static ide_test_case_funcs_t* get_test_case_funcs (int case_class, int case_id)
{
	TEEIO_ASSERT (case_class < TDISP_TEST_CASE_NUM);
	TEEIO_TEST_CASES *test_cases = &m_tdisp_test_case_funcs[case_class];

	TEEIO_ASSERT (case_id < test_cases->cnt);

	return &test_cases->funcs[case_id];
}

static ide_test_case_name_t* get_test_case_name (int case_class)
{
	TEEIO_ASSERT (case_class < TDISP_TEST_CASE_NUM + 1);

	return &m_tdisp_test_case_names[case_class];
}

static void* alloc_tdisp_test_group_context (void)
{
	pcie_ide_test_group_context_t *context =
		(pcie_ide_test_group_context_t*) malloc (sizeof (pcie_ide_test_group_context_t));

	TEEIO_ASSERT (context);
	memset (context, 0, sizeof (pcie_ide_test_group_context_t));
	context->common.signature = GROUP_CONTEXT_SIGNATURE;

	return context;
}

static bool tdisp_check_configuration_bitmap (uint32_t *bitmap)
{
	// default config is always set
	*bitmap |= BIT_MASK (IDE_TEST_CONFIGURATION_TYPE_DEFAULT);

	TEEIO_DEBUG ((TEEIO_DEBUG_INFO, "tdisp configuration bitmap=0x%08x\n", *bitmap));

	return true;
}

bool tdisp_test_lib_register_test_suite_funcs (teeio_test_funcs_t *funcs)
{
	TEEIO_ASSERT (funcs);

	funcs->get_case_funcs_func = get_test_case_funcs;
	funcs->get_case_name_func = get_test_case_name;
	funcs->get_configuration_bitmask_func = get_test_configuration_bitmask;
	funcs->get_configuration_funcs_func = get_test_configuration_funcs;
	funcs->get_configuration_name_func = get_test_configuration_name;
	funcs->get_group_funcs_func = get_test_group_funcs;
	funcs->alloc_test_group_context_func = alloc_tdisp_test_group_context;
	funcs->check_configuration_bitmap_func = tdisp_check_configuration_bitmap;

	return true;
}
