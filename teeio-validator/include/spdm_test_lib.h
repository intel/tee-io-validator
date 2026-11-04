/**
 *  Copyright Notice:
 *  Copyright 2025 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __SPDM_TEST_LIB_H__
#define __SPDM_TEST_LIB_H__

#include "ide_test.h"

// spdm_test_lib header file
#define TEEIO_SPDM_TEST_SCRATCH_BUFFER_SIZE 0x1000
typedef struct {
    void *spdm_context;
    /* test case specific scratch buffer between setup and case, avoid writable global variable */
    uint8_t test_scratch_buffer[TEEIO_SPDM_TEST_SCRATCH_BUFFER_SIZE];
    uint32_t test_scratch_buffer_size;
} teeio_spdm_test_context_t;

bool spdm_test_lib_register_test_suite_funcs(teeio_test_funcs_t* funcs);
void spdm_test_lib_clean();

void* spdm_test_get_spdm_context_from_test_context(void *test_context);

/**
 * Called by the test runner before a case's setup. It clears the SPDM traffic
 * trace so the setup of this case can be analysed on its own.
 */
void spdm_test_case_begin(void);

/**
 * Called by the test runner when an SPDM case's setup returned false. It records,
 * for the currently running case, why the case did not run. The case is still
 * reported as skipped.
 */
void spdm_test_case_setup_failed(int case_class, int case_id);

#endif