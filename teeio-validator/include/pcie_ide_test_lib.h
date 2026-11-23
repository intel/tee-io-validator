/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __PCIE_IDE_TEST_LIB_H__
#define __PCIE_IDE_TEST_LIB_H__

#include "ide_test.h"

// pcie_ide_test_lib header file

bool pcie_ide_test_lib_register_test_suite_funcs(teeio_test_funcs_t* funcs);

// check if an IDE stream status is expected_state (IDE_STREAM_STATUS_xxx) on RP and EP
bool is_ide_stream_in_state(uint8_t expected_state, TEST_IDE_TYPE ide_type,
                            int rp_cfg_space_fd, uint32_t rp_ecap_offset, uint8_t rp_ide_id,
                            int ep_cfg_space_fd, uint32_t ep_ecap_offset, uint8_t ep_ide_id
);

// setup ide stream
bool setup_ide_stream(void* doe_context, void* spdm_context,
                    uint32_t* session_id, uint8_t* kcbar_addr,
                    uint8_t stream_id, uint8_t ks,
                    ide_key_set_t* k_set, uint8_t rp_stream_index,
                    uint8_t port_index, IDE_TEST_TOPOLOGY_TYPE top_type,
                    ide_common_test_port_context_t* upper_port,
                    ide_common_test_port_context_t* lower_port,
                    bool skip_ksetgo);

// send K_SET_STOP for all sub-streams in both directions
bool pcie_ide_key_set_stop_all(void *doe_context, void *spdm_context, uint32_t *session_id,
                               uint8_t stream_id, uint8_t ks, uint8_t port_index);

// use ide_km to query the port_index matching with BDF of dev_port
bool pcie_ide_query_port_index(spdm_doe_context_t *spdm_doe, IDE_PORT *dev_port, uint8_t *matched_port_index);

#endif