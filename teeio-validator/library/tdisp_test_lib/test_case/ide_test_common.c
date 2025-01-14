/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include "ide_test.h"
#include "tdisp_test_internal.h"
#include "teeio_debug.h"

bool setup_ide_stream (void *doe_context, void *spdm_context, uint32_t *session_id,
	uint8_t *kcbar_addr, uint8_t stream_id, uint8_t ks, ide_key_set_t *k_set,
	uint8_t rp_stream_index, uint8_t port_index, IDE_TEST_TOPOLOGY_TYPE top_type,
	ide_common_test_port_context_t *upper_port, ide_common_test_port_context_t *lower_port,
	bool skip_ksetgo);

bool tdisp_test_set_ide_stream (void *test_context, uint8_t ks)
{
	assert_context (test_context);

	ide_common_test_case_context_t *case_context =
		(ide_common_test_case_context_t*) test_context;
	pcie_ide_test_group_context_t *group_context = case_context->group_context;

	// Setup parameters
	void *doe_context = group_context->spdm_doe.doe_context;
	void *spdm_context = group_context->spdm_doe.spdm_context;
	uint32_t *session_id = &group_context->spdm_doe.session_id;
	uint8_t *kcbar_addr = group_context->common.upper_port.mapped_kcbar_addr;
	uint8_t stream_id = 0;	// Default
	ide_key_set_t *k_set = &group_context->k_set;
	uint8_t rp_stream_index = group_context->rp_stream_index;
	uint8_t port_index = 0;
	IDE_TEST_TOPOLOGY_TYPE top_type = IDE_TEST_TOPOLOGY_TYPE_SEL_IDE;
	ide_common_test_port_context_t *upper_port = &group_context->common.upper_port;
	ide_common_test_port_context_t *lower_port = &group_context->common.lower_port;

	return setup_ide_stream (doe_context, spdm_context, session_id, kcbar_addr, stream_id, ks,
		k_set, rp_stream_index, port_index, top_type, upper_port, lower_port, false);
}
