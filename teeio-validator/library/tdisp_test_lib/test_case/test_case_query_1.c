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
	"tdisp_query send_receive_data",
	"sizeof(TdispMessage) == sizeof(TDISP_VERSION)",
	"TdispMessage.TDISPVersion == 0x10",
	"TdispMessage.MessageType == TDISP_VERSION",
	"TdispMessage.INTERFACE_ID == GET_TDISP_VERSION.INTERFACE_ID",
	"TdispMessage.VERSION_NUM_COUNT == 1",
	"TdispMessage.VERSION_NUM_ENTRY[0] == 0x10"
};


// Query Case 1.1
bool tdisp_test_query_1_setup (void *test_context)
{
	return true;
}

bool tdisp_test_query_1_run (void *test_context)
{
	assert_context (test_context);

	ide_common_test_case_context_t *case_context =
		(ide_common_test_case_context_t*) test_context;

	ide_run_test_case_t *test_case = case_context->test_case;

	TEEIO_ASSERT (test_case);
	int case_class = test_case->class_id;
	int case_id = test_case->case_id;

	teeio_test_result_t assertion_result = TEEIO_TEST_RESULT_NOT_TESTED;

	pci_tdisp_version_response_mine_t response;
	size_t response_size = sizeof (response);

	bool res = tdisp_test_get_version (test_context, g_tdisp_interface_id.function_id, &response,
		&response_size);

	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[0]);

	res = (response_size == sizeof (pci_tdisp_version_response_mine_t));
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 1, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[1]);

	res = (response.header.version == PCI_TDISP_MESSAGE_VERSION_10);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 2, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[2]);

	res = (response.header.message_type == PCI_TDISP_VERSION);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 3, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[3]);

	res = (response.header.interface_id.function_id == g_tdisp_interface_id.function_id);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 4, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[4]);

	res = (response.version_num_count == 1);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 5, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[5]);

	res = (response.version_num_entry[0] == PCI_TDISP_MESSAGE_VERSION_10);
	assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
	teeio_record_assertion_result (case_class, case_id, 6, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
		assertion_result, mAssertion[6]);

	return true;
}

bool tdisp_test_query_1_teardown (void *test_context)
{
	return true;
}
