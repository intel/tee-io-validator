/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include "helperlib.h"
#include "industry_standard/pci_tdisp.h"
#include "library/pci_tdisp_common_lib.h"

#define TDISP_REQUEST_SUPPORT_MSG_OFFSET            80
#define TDISP_REQUEST_GET_VERSION                   81
#define TDISP_REQUEST_GET_CAPABILITIES              82
#define TDISP_REQUEST_LOCK_INTERFACE                83
#define TDISP_REQUEST_GET_DEVICE_INTERFACE_REPORT   84
#define TDISP_REQUEST_GET_DEVICE_INTERFACE_STATE    85
#define TDISP_REQUEST_START_INTERFACE               86
#define TDISP_REQUEST_STOP_INTERFACE                87
#define TDISP_MMIO_REPORTING_OFFSET					0xD0000000

#pragma pack(1)
typedef struct {
	pci_tdisp_header_t header;
	uint8_t version_num_count;
	pci_tdisp_version_number_t version_num_entry[1];
} pci_tdisp_version_response_mine_t;
#pragma pack()

#pragma pack(1)
typedef struct {
	pci_tdisp_header_t header;
	uint16_t portion_length;
	uint16_t remainder_length;
	uint8_t report[LIBTDISP_INTERFACE_REPORT_PORTION_LEN];
} pci_tdisp_device_interface_report_response_mine_t;
#pragma pack()

typedef union {
	struct {
		uint8_t byte0;
		uint8_t byte1;
	};

	uint16_t raw;
} READ_DATA16;

typedef union {
	struct {
		uint8_t byte0;
		uint8_t byte1;
		uint8_t byte2;
		uint8_t byte3;
	};

	uint32_t raw;
} READ_DATA32;

void assert_context (void *test_context);

bool tdisp_test_set_ide_stream (void *test_context, uint8_t ks);

bool tdisp_test_get_version (void *test_context, uint32_t function_id,
	pci_tdisp_version_response_mine_t *response, size_t *response_size);

bool tdisp_test_get_capabilities (void *test_context, uint32_t function_id,
	pci_tdisp_capabilities_response_t *response, size_t *response_size);

bool tdisp_test_get_state (void *test_context, uint32_t function_id,
	pci_tdisp_device_interface_state_response_t *response, size_t *response_size);

bool tdisp_test_lock_interface (void *test_context, uint32_t function_id,
	uint16_t lock_interface_flags_supported, void *response, size_t *response_size);

bool tdisp_test_start_interface (void *test_context, uint32_t function_id,
	const uint8_t *start_interface_nonce, void *response, size_t *response_size);

bool tdisp_test_stop_interface (void *test_context, uint32_t function_id,
	pci_tdisp_stop_interface_response_t *response, size_t *response_size);

bool tdisp_test_get_interface_report (void *test_context, uint32_t function_id,
	uint8_t *interface_report, uint32_t *interface_report_size);
