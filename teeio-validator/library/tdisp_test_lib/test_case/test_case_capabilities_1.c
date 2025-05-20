/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include "ide_test.h"
#include "tdisp_test_internal.h"
#include "teeio_debug.h"
#include "hal/library/memlib.h"

extern pci_tdisp_interface_id_t g_tdisp_interface_id;

static const char *mAssertion[] = {
	"tdisp_capabilities send_receive_data",
	"sizeof(TdispMessage) == sizeof(TDISP_CAPABILITIES)",
	"TdispMessage.TDISPVersion == 0x10",
	"TdispMessage.MessageType == TDISP_CAPABILITIES",
	"TdispMessage.INTERFACE_ID == GET_TDISP_VERSION.INTERFACE_ID",
	"TdispMessage.DSM_CAPS == 0",
	"(TdispMessage.REQ_MSGS_SUPPORTED[0..2] == ExpectedSupportedBits) && (TdispMessage.REQ_MSGS_SUPPORTED[2..16] == 0x0)",
	"(TdispMessage.LOCK_INTERFACE_FLAGS_SUPPORTED & 0xFFE0) == 0",
	"TdispMessage.DEV_ADDR_WIDTH >= 52",
	"TdispMessage.NUM_REQ_THIS > 0",
	"TdispMessage.NUM_REQ_ALL > 0"
};

static const uint8_t tdisp_supported_messages[] = {
	TDISP_REQUEST_GET_VERSION,
	TDISP_REQUEST_GET_CAPABILITIES,
	TDISP_REQUEST_LOCK_INTERFACE,
	TDISP_REQUEST_GET_DEVICE_INTERFACE_REPORT,
	TDISP_REQUEST_GET_DEVICE_INTERFACE_STATE,
	TDISP_REQUEST_START_INTERFACE,
	TDISP_REQUEST_STOP_INTERFACE,
};


// Capabilities Case 1
bool tdisp_test_capabilities_1_setup (void *test_context)
{
	pci_tdisp_version_response_mine_t response;
	size_t response_size = sizeof (response);

	if (!tdisp_test_get_version (test_context, g_tdisp_interface_id.function_id, &response,
		&response_size)) {
		TEEIO_DEBUG ((TEEIO_DEBUG_ERROR, "tdisp_test_capabilities_1_setup failed.\n"));

		return false;
	}

	return true;
}

void tdisp_test_capabilities_1_run (void *test_context)
{
	assert_context (test_context);

	ide_common_test_case_context_t *case_context =
		(ide_common_test_case_context_t*) test_context;

	ide_run_test_case_t *test_case = case_context->test_case;

	TEEIO_ASSERT (test_case);
	int case_class = test_case->class_id;
	int case_id = test_case->case_id;

	teeio_test_result_t assertion_result = TEEIO_TEST_RESULT_NOT_TESTED;

	pci_tdisp_capabilities_response_t response;
	size_t response_size = sizeof (response);

	bool res = tdisp_test_get_capabilities (test_context, g_tdisp_interface_id.function_id,
		&response, &response_size);

	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[0]);

	res = (response_size == sizeof (pci_tdisp_capabilities_response_t));
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 1, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[1]);

	res = (response.header.version == PCI_TDISP_MESSAGE_VERSION_10);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 2, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[2]);

	res = (response.header.message_type == PCI_TDISP_CAPABILITIES);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 3, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[3]);

	res = (response.header.interface_id.function_id == g_tdisp_interface_id.function_id);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 4, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[4]);

	res = (response.rsp_caps.dsm_caps == 0);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 5, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[5]);

	// supported bits
	int len =
		sizeof (tdisp_supported_messages) / sizeof (tdisp_supported_messages[0]);
	uint16_t expected_supported_bits = 0;

	for (int i = 0; i < len; ++i) {
		expected_supported_bits |= 1 <<
			(tdisp_supported_messages[i] - TDISP_REQUEST_SUPPORT_MSG_OFFSET);
	}

	READ_DATA16 data16 = {
		.byte0 = response.rsp_caps.req_msg_supported[0],
		.byte1 = response.rsp_caps.req_msg_supported[1]
	};
	uint16_t first_two_bytes = data16.raw;
	uint8_t the_rest = 0;

	for (int i = 2; i < 16; ++i) {
		the_rest |= response.rsp_caps.req_msg_supported[i];
	}

	res = ((first_two_bytes == expected_supported_bits) && (the_rest == 0));
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 6, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[6]);

	res = ((response.rsp_caps.lock_interface_flags_supported & 0xFFE0) == 0);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 7, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[7]);

	res = (response.rsp_caps.dev_addr_width >= 52);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 8, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[8]);

	res = (response.rsp_caps.num_req_this > 0);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 9, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[9]);

	res = (response.rsp_caps.num_req_all > 0);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 10,
		IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, mAssertion[10]);
}

void tdisp_test_capabilities_1_teardown (void *test_context)
{
}
