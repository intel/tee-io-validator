/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __CXL_IDE_TEST_LIB_H__
#define __CXL_IDE_TEST_LIB_H__

#include "ide_test.h"

// cxl_ide_test_lib header file

bool cxl_ide_test_lib_register_test_suite_funcs(teeio_test_funcs_t* funcs);

// setup cxl ide stream
bool cxl_setup_ide_stream(void *doe_context, void *spdm_context,
                          uint32_t *session_id, uint8_t *kcbar_addr,
                          uint8_t stream_id, uint8_t port_index,
                          ide_common_test_port_context_t *upper_port,
                          ide_common_test_port_context_t *lower_port,
                          bool skip_ksetgo, uint32_t config_bitmap,
                          CXL_IDE_MODE ide_mode, bool key_refresh);

// stop the CXL.cachemem IDE stream and check that the ports are in Insecure State
bool cxl_teardown_ide_stream_common(void *doe_context, void *spdm_context,
                                    uint32_t *session_id, uint8_t *kcbar_addr,
                                    uint8_t stream_id, uint8_t port_index,
                                    ide_common_test_port_context_t *upper_port,
                                    ide_common_test_port_context_t *lower_port,
                                    bool *insecure);

// Query the port index of dev_port with device function and bus number
bool cxl_ide_query_dev_port_index(spdm_doe_context_t *spdm_doe, IDE_PORT *dev_port, uint8_t *matched_port_index);

// CXL_QUERY the device at port_index and store the response in cxl_data
bool cxl_ide_query_dev(spdm_doe_context_t *spdm_doe, uint8_t port_index, CXL_PRIV_DATA *cxl_data);

#endif
