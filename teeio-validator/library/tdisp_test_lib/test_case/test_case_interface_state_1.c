/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include "ide_test.h"
#include "tdisp_test_internal.h"
#include "teeio_debug.h"

extern pci_tdisp_interface_id_t g_tdisp_interface_id;

static const char *mAssertion[] = {
	"tdisp_query send_receive_data",
	"sizeof(TdispMessage) == sizeof(DEVICE_INTERFACE_STATE)",
	"TdispMessage.TDISPVersion == 0x10",
	"TdispMessage.MessageType == DEVICE_INTERFACE_STATE",
	"TdispMessage.INTERFACE_ID == GET_DEVICE_INTERFACE_STATE.INTERFACE_ID",
	"TdispMessage.TDI_STATE == CONFIG_UNLOCKED"
};
static bool setup_success = false;


bool tdisp_test_interface_state_1_setup (void *test_context)
{
	pci_tdisp_version_response_mine_t get_version_response;
	size_t response_size = sizeof (get_version_response);

	if (!tdisp_test_get_version (test_context, g_tdisp_interface_id.function_id,
		&get_version_response,
		&response_size) || (response_size != sizeof (pci_tdisp_version_response_mine_t))) {
		TEEIO_DEBUG ((TEEIO_DEBUG_ERROR,
			"tdisp_test_interface_state_1_setup get_version failed.\n"));

		return false;
	}

	pci_tdisp_capabilities_response_t get_capabilities_response;

	response_size = sizeof (get_capabilities_response);
	if (!tdisp_test_get_capabilities (test_context, g_tdisp_interface_id.function_id,
		&get_capabilities_response,
		&response_size) || (response_size != sizeof (pci_tdisp_capabilities_response_t))) {
		TEEIO_DEBUG ((TEEIO_DEBUG_ERROR,
			"tdisp_test_interface_state_1_setup get_capabilities failed.\n"));

		return false;
	}

	return setup_success = true;
}

void tdisp_test_interface_state_1_run (void *test_context)
{
	assert_context (test_context);

	ide_common_test_case_context_t *case_context =
		(ide_common_test_case_context_t*) test_context;

	ide_run_test_case_t *test_case = case_context->test_case;

	TEEIO_ASSERT (test_case);
	int case_class = test_case->class_id;
	int case_id = test_case->case_id;

	teeio_test_result_t assertion_result = TEEIO_TEST_RESULT_NOT_TESTED;

	pci_tdisp_device_interface_state_response_t response;
	size_t response_size = sizeof (response);

	bool res = tdisp_test_get_state (test_context, g_tdisp_interface_id.function_id, &response,
		&response_size);

	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[0]);

	res = (response_size == sizeof (pci_tdisp_device_interface_state_response_t));
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 1, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[1]);

	res = (response.header.version == PCI_TDISP_MESSAGE_VERSION_10);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 2, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[2]);

	res = (response.header.message_type == PCI_TDISP_DEVICE_INTERFACE_STATE);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 3, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[3]);

	res = (response.header.interface_id.function_id == g_tdisp_interface_id.function_id);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 4, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[4]);

	res = (response.tdi_state == PCI_TDISP_INTERFACE_STATE_CONFIG_UNLOCKED);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 5, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[5]);

	return;
}

void tdisp_test_interface_state_1_teardown (void *test_context)
{
	if (setup_success == false) {
		return;
	}

	pci_tdisp_stop_interface_response_t response;
	size_t response_size = sizeof (response);

	tdisp_test_stop_interface (test_context, g_tdisp_interface_id.function_id, &response,
		&response_size);
}
