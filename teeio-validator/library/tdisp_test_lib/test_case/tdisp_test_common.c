/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include "ide_test.h"
#include "tdisp_test_internal.h"
#include "teeio_debug.h"
#include "hal/library/memlib.h"

/* *INDENT-OFF* */
#include "library/spdm_transport_pcidoe_lib.h"	// Note: this header file has to be before pci_tdisp_requester_lib.h
#include "library/pci_tdisp_requester_lib.h"
/* *INDENT-ON* */

void assert_context (void *test_context)
{
	ide_common_test_case_context_t *case_context =
		(ide_common_test_case_context_t*) test_context;

	TEEIO_ASSERT (case_context);
	TEEIO_ASSERT (case_context->signature == CASE_CONTEXT_SIGNATURE);

	pcie_ide_test_group_context_t *group_context =
		case_context->group_context;

	TEEIO_ASSERT (group_context);
	TEEIO_ASSERT (group_context->common.signature == GROUP_CONTEXT_SIGNATURE);
	TEEIO_ASSERT (group_context->spdm_doe.spdm_context);
	TEEIO_ASSERT (group_context->spdm_doe.session_id);
}

bool tdisp_test_get_version (void *test_context, uint32_t function_id,
	pci_tdisp_version_response_mine_t *response, size_t *response_size)
{
	assert_context (test_context);

	ide_common_test_case_context_t *case_context =
		(ide_common_test_case_context_t*) test_context;
	pcie_ide_test_group_context_t *group_context =
		case_context->group_context;

	pci_tdisp_get_version_request_t request;
	size_t request_size;

	libspdm_zero_mem (&request, sizeof (request));
	request.header.version = PCI_TDISP_MESSAGE_VERSION_10;
	request.header.message_type = PCI_TDISP_GET_VERSION;
	request.header.interface_id.function_id = function_id;

	request_size = sizeof (request);
	libspdm_return_t status = pci_tdisp_send_receive_data (group_context->spdm_doe.spdm_context,
		&group_context->spdm_doe.session_id, &request, request_size, response, response_size);

	return LIBSPDM_STATUS_IS_ERROR (status) == false;
}

bool tdisp_test_get_capabilities (void *test_context, uint32_t function_id,
	pci_tdisp_capabilities_response_t *response, size_t *response_size)
{
	assert_context (test_context);

	ide_common_test_case_context_t *case_context =
		(ide_common_test_case_context_t*) test_context;
	pcie_ide_test_group_context_t *group_context =
		case_context->group_context;

	pci_tdisp_get_capabilities_request_t request;
	size_t request_size;

	pci_tdisp_requester_capabilities_t req_caps;

	req_caps.tsm_caps = 0;

	libspdm_zero_mem (&request, sizeof (request));
	request.header.version = PCI_TDISP_MESSAGE_VERSION_10;
	request.header.message_type = PCI_TDISP_GET_CAPABILITIES;
	request.header.interface_id.function_id = function_id;
	libspdm_copy_mem (&request.req_caps, sizeof (request.req_caps), &req_caps, sizeof (req_caps));

	request_size = sizeof (request);
	libspdm_return_t status = pci_tdisp_send_receive_data (group_context->spdm_doe.spdm_context,
		&group_context->spdm_doe.session_id, &request, request_size, response, response_size);

	return LIBSPDM_STATUS_IS_ERROR (status) == false;
}

bool tdisp_test_get_state (void *test_context, uint32_t function_id,
	pci_tdisp_device_interface_state_response_t *response, size_t *response_size)
{
	assert_context (test_context);

	ide_common_test_case_context_t *case_context =
		(ide_common_test_case_context_t*) test_context;
	pcie_ide_test_group_context_t *group_context =
		case_context->group_context;

	pci_tdisp_get_device_interface_state_request_t request;
	size_t request_size;

	libspdm_zero_mem (&request, sizeof (request));
	request.header.version = PCI_TDISP_MESSAGE_VERSION_10;
	request.header.message_type = PCI_TDISP_GET_DEVICE_INTERFACE_STATE;
	request.header.interface_id.function_id = function_id;

	request_size = sizeof (request);
	libspdm_return_t status = pci_tdisp_send_receive_data (group_context->spdm_doe.spdm_context,
		&group_context->spdm_doe.session_id, &request, request_size, response, response_size);

	return LIBSPDM_STATUS_IS_ERROR (status) == false;
}

bool tdisp_test_lock_interface (void *test_context, uint32_t function_id,
	uint16_t lock_interface_flags_supported, void *response, size_t *response_size)
{
	assert_context (test_context);

	ide_common_test_case_context_t *case_context =
		(ide_common_test_case_context_t*) test_context;
	pcie_ide_test_group_context_t *group_context =
		case_context->group_context;

	pci_tdisp_lock_interface_request_t request;
	size_t request_size;

	pci_tdisp_lock_interface_param_t lock_interface_param;

	libspdm_zero_mem (&lock_interface_param, sizeof (lock_interface_param));
	lock_interface_param.flags = lock_interface_flags_supported;
	lock_interface_param.default_stream_id = 0;
	lock_interface_param.mmio_reporting_offset = TDISP_MMIO_REPORTING_OFFSET;
	lock_interface_param.bind_p2p_address_mask = 0;

	libspdm_zero_mem (&request, sizeof (request));
	request.header.version = PCI_TDISP_MESSAGE_VERSION_10;
	request.header.message_type = PCI_TDISP_LOCK_INTERFACE_REQ;
	request.header.interface_id.function_id = function_id;
	request.lock_interface_param = lock_interface_param;

	request_size = sizeof (request);
	libspdm_return_t status = pci_tdisp_send_receive_data (group_context->spdm_doe.spdm_context,
		&group_context->spdm_doe.session_id, &request, request_size, response, response_size);

	return LIBSPDM_STATUS_IS_ERROR (status) == false;
}

bool tdisp_test_start_interface (void *test_context, uint32_t function_id,
	const uint8_t *start_interface_nonce, void *response, size_t *response_size)
{
	assert_context (test_context);

	ide_common_test_case_context_t *case_context =
		(ide_common_test_case_context_t*) test_context;
	pcie_ide_test_group_context_t *group_context =
		case_context->group_context;

	pci_tdisp_start_interface_request_t request;
	size_t request_size;

	libspdm_zero_mem (&request, sizeof (request));
	request.header.version = PCI_TDISP_MESSAGE_VERSION_10;
	request.header.message_type = PCI_TDISP_START_INTERFACE_REQ;
	request.header.interface_id.function_id = function_id;
	libspdm_copy_mem (request.start_interface_nonce, sizeof (request.start_interface_nonce),
		start_interface_nonce, PCI_TDISP_START_INTERFACE_NONCE_SIZE);

	request_size = sizeof (request);
	libspdm_return_t status = pci_tdisp_send_receive_data (group_context->spdm_doe.spdm_context,
		&group_context->spdm_doe.session_id, &request, request_size, response, response_size);

	libspdm_zero_mem (&request.start_interface_nonce, sizeof (request.start_interface_nonce));

	return LIBSPDM_STATUS_IS_ERROR (status) == false;
}

bool tdisp_test_stop_interface (void *test_context, uint32_t function_id,
	pci_tdisp_stop_interface_response_t *response, size_t *response_size)
{
	assert_context (test_context);

	ide_common_test_case_context_t *case_context =
		(ide_common_test_case_context_t*) test_context;
	pcie_ide_test_group_context_t *group_context =
		case_context->group_context;

	pci_tdisp_stop_interface_request_t request;
	size_t request_size;

	libspdm_zero_mem (&request, sizeof (request));
	request.header.version = PCI_TDISP_MESSAGE_VERSION_10;
	request.header.message_type = PCI_TDISP_STOP_INTERFACE_REQ;
	request.header.interface_id.function_id = function_id;

	request_size = sizeof (request);
	libspdm_return_t status = pci_tdisp_send_receive_data (group_context->spdm_doe.spdm_context,
		&group_context->spdm_doe.session_id, &request, request_size, response, response_size);

	return LIBSPDM_STATUS_IS_ERROR (status) == false;
}

static const char *mAssertion[] = {
	"tdisp_query send_receive_data",
	"sizeof(TdispMessage) >= sizeof(DEVICE_INTERFACE_REPORT, REPORT_BYTES)",
	"TdispMessage.TDISPVersion == 0x10",
	"TdispMessage.MessageType == DEVICE_INTERFACE_REPORT",
	"TdispMessage.INTERFACE_ID == DEVICE_INTERFACE_REPORT.INTERFACE_ID",
	"TdispMessage.PORTION_LENGTH > 0"
};


bool tdisp_test_get_interface_report (void *test_context, uint32_t function_id,
	uint8_t *interface_report, uint32_t *interface_report_size)
{
	assert_context (test_context);

	ide_common_test_case_context_t *case_context =
		(ide_common_test_case_context_t*) test_context;
	pcie_ide_test_group_context_t *group_context =
		case_context->group_context;

	ide_run_test_case_t *test_case = case_context->test_case;

	TEEIO_ASSERT (test_case);
	int case_class = test_case->class_id;
	int case_id = test_case->case_id;

	teeio_test_result_t assertion_result = TEEIO_TEST_RESULT_NOT_TESTED;

	libspdm_return_t status;
	pci_tdisp_get_device_interface_report_request_t request;
	size_t request_size;
	pci_tdisp_device_interface_report_response_mine_t response;
	size_t response_size;
	uint16_t offset;
	uint16_t remainder_length;
	uint32_t total_report_length;
	bool res;

	offset = 0;
	remainder_length = 0;
	total_report_length = 0;
	do {
		libspdm_zero_mem (&request, sizeof (request));
		request.header.version = PCI_TDISP_MESSAGE_VERSION_10;
		request.header.message_type = PCI_TDISP_GET_DEVICE_INTERFACE_REPORT;
		request.header.interface_id.function_id = function_id;
		request.offset = offset;
		request.length = LIBTDISP_INTERFACE_REPORT_PORTION_LEN;
		if (request.offset != 0) {
			request.length = LIBSPDM_MIN (remainder_length, LIBTDISP_INTERFACE_REPORT_PORTION_LEN);
		}

		request_size = sizeof (request);
		response_size = sizeof (response);
		status = pci_tdisp_send_receive_data (group_context->spdm_doe.spdm_context,
			&group_context->spdm_doe.session_id, &request, request_size, &response, &response_size);

		res = !LIBSPDM_STATUS_IS_ERROR (status);
		assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
		teeio_record_assertion_result (case_class, case_id, 0,
			IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, mAssertion[0]);

		res = (response_size >= sizeof (pci_tdisp_device_interface_report_response_t)) &&
			(response.portion_length <= request.length) && (response_size ==
				sizeof (pci_tdisp_device_interface_report_response_t) + response.portion_length);
		assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
		teeio_record_assertion_result (case_class, case_id, 1,
			IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, mAssertion[1]);

		res = (response.header.version == PCI_TDISP_MESSAGE_VERSION_10);
		assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
		teeio_record_assertion_result (case_class, case_id, 2,
			IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, mAssertion[2]);

		res = (response.header.message_type == PCI_TDISP_DEVICE_INTERFACE_REPORT);
		assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
		teeio_record_assertion_result (case_class, case_id, 3,
			IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, mAssertion[3]);

		res = (response.header.interface_id.function_id == function_id);
		assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
		teeio_record_assertion_result (case_class, case_id, 4,
			IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, mAssertion[4]);

		res = (response.portion_length != 0);
		assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
		teeio_record_assertion_result (case_class, case_id, 5,
			IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, mAssertion[5]);

		if (offset == 0) {
			total_report_length = response.portion_length + response.remainder_length;
			if (total_report_length > *interface_report_size) {
				*interface_report_size = total_report_length;
				TEEIO_DEBUG ((TEEIO_DEBUG_ERROR,
					"Not enough buffer size for interface report.\n"));

				return false;
			}
		}
		else {
			if (total_report_length !=
				(uint32_t) (offset + response.portion_length + response.remainder_length)) {
				teeio_record_assertion_result (case_class, case_id, 1,
					IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED,
					mAssertion[1]);

				return false;
			}
		}
		libspdm_copy_mem (interface_report + offset, *interface_report_size - offset,
			response.report, response.portion_length);
		offset = offset + response.portion_length;
		remainder_length = response.remainder_length;
	} while (remainder_length != 0);

	*interface_report_size = total_report_length;

	return true;
}
