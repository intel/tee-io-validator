/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef _TDISP_COMMON_H_
#define _TDISP_COMMON_H_

#include "stdbool.h"

//
// TDISP test cases
//

// Query Case 1.1
bool tdisp_test_query_1_setup (void *test_context);
bool tdisp_test_query_1_run (void *test_context);
bool tdisp_test_query_1_teardown (void *test_context);

// Query Case 1.2
bool tdisp_test_query_2_setup (void *test_context);
bool tdisp_test_query_2_run (void *test_context);
bool tdisp_test_query_2_teardown (void *test_context);

// LockInterface Case 2.1
bool tdisp_test_lock_interface_1_setup (void *test_context);
bool tdisp_test_lock_interface_1_run (void *test_context);
bool tdisp_test_lock_interface_1_teardown (void *test_context);

// LockInterface Case 2.2
bool tdisp_test_lock_interface_2_setup (void *test_context);
bool tdisp_test_lock_interface_2_run (void *test_context);
bool tdisp_test_lock_interface_2_teardown (void *test_context);

// LockInterface Case 2.3
bool tdisp_test_lock_interface_3_setup (void *test_context);
bool tdisp_test_lock_interface_3_run (void *test_context);
bool tdisp_test_lock_interface_3_teardown (void *test_context);

// LockInterface Case 2.4
bool tdisp_test_lock_interface_4_setup (void *test_context);
bool tdisp_test_lock_interface_4_run (void *test_context);
bool tdisp_test_lock_interface_4_teardown (void *test_context);

// DInterfaceReport Case 3.1
bool tdisp_test_interface_report_1_setup (void *test_context);
bool tdisp_test_interface_report_1_run (void *test_context);
bool tdisp_test_interface_report_1_teardown (void *test_context);

// DInterfaceReport Case 3.2
bool tdisp_test_interface_report_2_setup (void *test_context);
bool tdisp_test_interface_report_2_run (void *test_context);
bool tdisp_test_interface_report_2_teardown (void *test_context);

// DInterfaceReport Case 3.3
bool tdisp_test_interface_report_3_setup (void *test_context);
bool tdisp_test_interface_report_3_run (void *test_context);
bool tdisp_test_interface_report_3_teardown (void *test_context);

// DInterfaceReport Case 3.4
bool tdisp_test_interface_report_4_setup (void *test_context);
bool tdisp_test_interface_report_4_run (void *test_context);
bool tdisp_test_interface_report_4_teardown (void *test_context);

// DInterfaceReport Case 3.5
bool tdisp_test_interface_report_5_setup (void *test_context);
bool tdisp_test_interface_report_5_run (void *test_context);
bool tdisp_test_interface_report_5_teardown (void *test_context);

// DInterfaceState Case 4.1
bool tdisp_test_interface_state_1_setup (void *test_context);
bool tdisp_test_interface_state_1_run (void *test_context);
bool tdisp_test_interface_state_1_teardown (void *test_context);

// DInterfaceState Case 4.2
bool tdisp_test_interface_state_2_setup (void *test_context);
bool tdisp_test_interface_state_2_run (void *test_context);
bool tdisp_test_interface_state_2_teardown (void *test_context);

// DInterfaceState Case 4.3
bool tdisp_test_interface_state_3_setup (void *test_context);
bool tdisp_test_interface_state_3_run (void *test_context);
bool tdisp_test_interface_state_3_teardown (void *test_context);

// DInterfaceState Case 4.4
bool tdisp_test_interface_state_4_setup (void *test_context);
bool tdisp_test_interface_state_4_run (void *test_context);
bool tdisp_test_interface_state_4_teardown (void *test_context);

// StartInterface Case 5.1
bool tdisp_test_start_interface_1_setup (void *test_context);
bool tdisp_test_start_interface_1_run (void *test_context);
bool tdisp_test_start_interface_1_teardown (void *test_context);

// StartInterface Case 5.2
bool tdisp_test_start_interface_2_setup (void *test_context);
bool tdisp_test_start_interface_2_run (void *test_context);
bool tdisp_test_start_interface_2_teardown (void *test_context);

// StartInterface Case 5.3
bool tdisp_test_start_interface_3_setup (void *test_context);
bool tdisp_test_start_interface_3_run (void *test_context);
bool tdisp_test_start_interface_3_teardown (void *test_context);

// StartInterface Case 5.4
bool tdisp_test_start_interface_4_setup (void *test_context);
bool tdisp_test_start_interface_4_run (void *test_context);
bool tdisp_test_start_interface_4_teardown (void *test_context);

// StopInterface Case 6.1
bool tdisp_test_stop_interface_1_setup (void *test_context);
bool tdisp_test_stop_interface_1_run (void *test_context);
bool tdisp_test_stop_interface_1_teardown (void *test_context);

// StopInterface Case 6.2
bool tdisp_test_stop_interface_2_setup (void *test_context);
bool tdisp_test_stop_interface_2_run (void *test_context);
bool tdisp_test_stop_interface_2_teardown (void *test_context);

// StopInterface Case 6.3
bool tdisp_test_stop_interface_3_setup (void *test_context);
bool tdisp_test_stop_interface_3_run (void *test_context);
bool tdisp_test_stop_interface_3_teardown (void *test_context);

//
// TDISP Test Config
//

// default config
bool tdisp_test_config_default_enable_common (void *test_context);
bool tdisp_test_config_default_disable_common (void *test_context);
bool tdisp_test_config_default_support_common (void *test_context);
bool tdisp_test_config_default_check_common (void *test_context);

//
// TDISP Test Group
//

// selective_ide test group
bool tdisp_test_group_setup_sel (void *test_context);
bool tdisp_test_group_teardown_sel (void *test_context);


#endif
