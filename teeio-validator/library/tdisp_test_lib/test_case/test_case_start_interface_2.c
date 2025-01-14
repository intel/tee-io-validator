/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include "ide_test.h"
#include "tdisp_test_internal.h"
#include "teeio_debug.h"
#include "hal/library/memlib.h"
#include "industry_standard/pci_idekm.h"

extern pci_tdisp_interface_id_t g_tdisp_interface_id;

static const char *mAssertion[] = {
	"tdisp_query send_receive_data",
	"sizeof(TdispMessage) == sizeof(TDISP_ERROR)",
	"TdispMessage.TDISPVersion == 0x10",
	"TdispMessage.MessageType == TDISP_ERROR",
	"TdispMessage.INTERFACE_ID == START_INTERFACE_REQUEST.INTERFACE_ID",
	"TdispMessage.ERROR_CODE == INVALID_NONCE",
	"TdispMessage.TDI_STATE == CONFIG_LOCKED"
};
static pci_tdisp_lock_interface_response_t lock_interface_response = {0};
static bool setup_success = false;


bool tdisp_test_start_interface_2_setup (void *test_context)
{
	pci_tdisp_version_response_mine_t get_version_response;
	size_t response_size = sizeof (get_version_response);

	if (!tdisp_test_get_version (test_context, g_tdisp_interface_id.function_id,
		&get_version_response,
		&response_size) || (response_size != sizeof (pci_tdisp_version_response_mine_t))) {
		TEEIO_DEBUG ((TEEIO_DEBUG_ERROR,
			"tdisp_test_start_interface_2_setup get_version failed.\n"));

		return false;
	}

	pci_tdisp_capabilities_response_t get_capabilities_response;

	response_size = sizeof (get_capabilities_response);
	if (!tdisp_test_get_capabilities (test_context, g_tdisp_interface_id.function_id,
		&get_capabilities_response,
		&response_size) || (response_size != sizeof (pci_tdisp_capabilities_response_t))) {
		TEEIO_DEBUG ((TEEIO_DEBUG_ERROR,
			"tdisp_test_start_interface_2_setup get_capabilities failed.\n"));

		return false;
	}

	if (!tdisp_test_set_ide_stream (test_context, PCI_IDE_KM_KEY_SET_K0)) {
		TEEIO_DEBUG ((TEEIO_DEBUG_ERROR,
			"tdisp_test_start_interface_2_setup set_ide_stream failed.\n"));

		return false;
	}

	response_size = sizeof (lock_interface_response);
	if (!tdisp_test_lock_interface (test_context, g_tdisp_interface_id.function_id,
		get_capabilities_response.rsp_caps.lock_interface_flags_supported, &lock_interface_response,
		&response_size) || (response_size != sizeof (pci_tdisp_lock_interface_response_t))) {
		TEEIO_DEBUG ((TEEIO_DEBUG_ERROR,
			"tdisp_test_start_interface_2_setup lock_interface failed.\n"));

		return false;
	}

	pci_tdisp_device_interface_state_response_t get_state_response;

	response_size = sizeof (get_state_response);
	if (!tdisp_test_get_state (test_context, g_tdisp_interface_id.function_id, &get_state_response,
		&response_size) ||
		(response_size != sizeof (pci_tdisp_device_interface_state_response_t))) {
		TEEIO_DEBUG ((TEEIO_DEBUG_ERROR,
			"tdisp_test_start_interface_2_setup get_state failed.\n"));

		return false;
	}

	if (get_state_response.tdi_state != PCI_TDISP_INTERFACE_STATE_CONFIG_LOCKED) {
		// Skip the test
		ide_common_test_case_context_t *case_context =
			(ide_common_test_case_context_t*) test_context;

		case_context->action = IDE_COMMON_TEST_ACTION_SKIP;

		return setup_success = true;
	}

	return setup_success = true;
}

bool tdisp_test_start_interface_2_run (void *test_context)
{
	assert_context (test_context);

	ide_common_test_case_context_t *case_context =
		(ide_common_test_case_context_t*) test_context;

	ide_run_test_case_t *test_case = case_context->test_case;

	TEEIO_ASSERT (test_case);
	int case_class = test_case->class_id;
	int case_id = test_case->case_id;

	teeio_test_result_t assertion_result = TEEIO_TEST_RESULT_NOT_TESTED;

	pci_tdisp_error_response_t response;
	size_t response_size = sizeof (response);

	// Modity start_interface_nonce
	uint8_t start_interface_nonce[PCI_TDISP_START_INTERFACE_NONCE_SIZE] = {0};

	for (int i = 0; i < PCI_TDISP_START_INTERFACE_NONCE_SIZE; ++i) {
		start_interface_nonce[i] =
			~lock_interface_response.start_interface_nonce[i];
	}

	bool res = tdisp_test_start_interface (test_context, g_tdisp_interface_id.function_id,
		start_interface_nonce, &response, &response_size);

	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[0]);

	res = (response_size == sizeof (pci_tdisp_error_response_t));
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 1, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[1]);

	res = (response.header.version == PCI_TDISP_MESSAGE_VERSION_10);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 2, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[2]);

	res = (response.header.message_type == PCI_TDISP_ERROR);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 3, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[3]);

	res = (response.header.interface_id.function_id == g_tdisp_interface_id.function_id);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 4, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[4]);

	res = (response.error_code == PCI_TDISP_ERROR_CODE_INVALID_NONCE);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 5, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[5]);

	pci_tdisp_device_interface_state_response_t get_state_response;

	response_size = sizeof (get_state_response);
	if (!tdisp_test_get_state (test_context, g_tdisp_interface_id.function_id, &get_state_response,
		&response_size)) {
		TEEIO_DEBUG ((TEEIO_DEBUG_ERROR,
			"tdisp_test_start_interface_2_run get_state failed.\n"));

		return false;
	}

	res = (get_state_response.tdi_state == PCI_TDISP_INTERFACE_STATE_CONFIG_LOCKED);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 6, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[6]);

	return true;
}

bool tdisp_test_start_interface_2_teardown (void *test_context)
{
	if (setup_success == false) {
		return true;
	}

	pci_tdisp_stop_interface_response_t response;
	size_t response_size = sizeof (response);

	return tdisp_test_stop_interface (test_context, g_tdisp_interface_id.function_id, &response,
		&response_size);
}
