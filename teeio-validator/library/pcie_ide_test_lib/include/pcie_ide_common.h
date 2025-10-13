/**
 *  Copyright Notice:
 *  Copyright 2024-2025 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef _PCIE_IDE_COMMON_H_
#define _PCIE_IDE_COMMON_H_

//
// PCIE IDE test cases
//

// Query Case 1.1
bool pcie_ide_test_query_1_setup(void *test_context);
void pcie_ide_test_query_1_run(void *test_context);
void pcie_ide_test_query_1_teardown(void *test_context);

// Query Case 1.2
bool pcie_ide_test_query_2_setup(void *test_context);
void pcie_ide_test_query_2_run(void *test_context);
void pcie_ide_test_query_2_teardown(void *test_context);

// KeyProg Case 2.1
bool pcie_ide_test_keyprog_1_setup(void *test_context);
void pcie_ide_test_keyprog_1_run(void *test_context);
void pcie_ide_test_keyprog_1_teardown(void *test_context);

// KeyProg Case 2.2
bool pcie_ide_test_keyprog_2_setup(void *test_context);
void pcie_ide_test_keyprog_2_run(void *test_context);
void pcie_ide_test_keyprog_2_teardown(void *test_context);

// KeyProg Case 2.3
bool pcie_ide_test_keyprog_3_setup(void *test_context);
void pcie_ide_test_keyprog_3_run(void *test_context);
void pcie_ide_test_keyprog_3_teardown(void *test_context);

// KeyProg Case 2.4
bool pcie_ide_test_keyprog_4_setup(void *test_context);
void pcie_ide_test_keyprog_4_run(void *test_context);
void pcie_ide_test_keyprog_4_teardown(void *test_context);

// KeyProg Case 2.5
bool pcie_ide_test_keyprog_5_setup(void *test_context);
void pcie_ide_test_keyprog_5_run(void *test_context);
void pcie_ide_test_keyprog_5_teardown(void *test_context);

// KeyProg Case 2.6
bool pcie_ide_test_keyprog_6_setup(void *test_context);
void pcie_ide_test_keyprog_6_run(void *test_context);
void pcie_ide_test_keyprog_6_teardown(void *test_context);

// KSetGo Case 3.1
bool pcie_ide_test_ksetgo_1_setup(void *test_context);
void pcie_ide_test_ksetgo_1_run(void *test_context);
void pcie_ide_test_ksetgo_1_teardown(void *test_context);

// KSetGo Case 3.2
bool pcie_ide_test_ksetgo_2_setup(void *test_context);
void pcie_ide_test_ksetgo_2_run(void *test_context);
void pcie_ide_test_ksetgo_2_teardown(void *test_context);

// KSetGo Case 3.3
bool pcie_ide_test_ksetgo_3_setup(void *test_context);
void pcie_ide_test_ksetgo_3_run(void *test_context);
void pcie_ide_test_ksetgo_3_teardown(void *test_context);

// KSetGo Case 3.4
bool pcie_ide_test_ksetgo_4_setup(void *test_context);
void pcie_ide_test_ksetgo_4_run(void *test_context);
void pcie_ide_test_ksetgo_4_teardown(void *test_context);

// KSetStop Case 4.1
bool pcie_ide_test_ksetstop_1_setup(void *test_context);
void pcie_ide_test_ksetstop_1_run(void *test_context);
void pcie_ide_test_ksetstop_1_teardown(void *test_context);

// KSetStop Case 4.2
bool pcie_ide_test_ksetstop_2_setup(void *test_context);
void pcie_ide_test_ksetstop_2_run(void *test_context);
void pcie_ide_test_ksetstop_2_teardown(void *test_context);

// KSetStop Case 4.3
bool pcie_ide_test_ksetstop_3_setup(void *test_context);
void pcie_ide_test_ksetstop_3_run(void *test_context);
void pcie_ide_test_ksetstop_3_teardown(void *test_context);

// KSetStop Case 4.4
bool pcie_ide_test_ksetstop_4_setup(void *test_context);
void pcie_ide_test_ksetstop_4_run(void *test_context);
void pcie_ide_test_ksetstop_4_teardown(void *test_context);

// SpdmSession
bool pcie_ide_test_spdm_session_1_setup(void *test_context);
void pcie_ide_test_spdm_session_1_run(void *test_context);
void pcie_ide_test_spdm_session_1_teardown(void *test_context);

bool pcie_ide_test_spdm_session_2_setup(void *test_context);
void pcie_ide_test_spdm_session_2_run(void *test_context);
void pcie_ide_test_spdm_session_2_teardown(void *test_context);

// Full case
bool pcie_ide_test_full_1_setup(void *test_context);
void pcie_ide_test_full_1_run(void *test_context);
void pcie_ide_test_full_1_teardown(void *test_context);

// Full case - KeyRefresh
bool pcie_ide_test_full_keyrefresh_setup(void *test_context);
void pcie_ide_test_full_keyrefresh_run(void *test_context);
void pcie_ide_test_full_keyrefresh_teardown(void *test_context);

//
// PCIE_IDE Test Config
//

// default config
bool pcie_ide_test_config_default_enable_common(void *test_context);
bool pcie_ide_test_config_default_disable_common(void *test_context);
bool pcie_ide_test_config_default_support_common(void *test_context);
bool pcie_ide_test_config_default_check_common(void *test_context);

// test selective_and_link_ide with default config
bool pcie_ide_test_config_default_enable_sel_link(void *test_context);
bool pcie_ide_test_config_default_disable_sel_link(void *test_context);
bool pcie_ide_test_config_default_support_sel_link(void *test_context);
bool pcie_ide_test_config_default_check_sel_link(void *test_context);

// pcrc
bool pcie_ide_test_config_pcrc_enable_sel(void *test_context);
bool pcie_ide_test_config_pcrc_disable_sel(void *test_context);
bool pcie_ide_test_config_pcrc_support_sel(void *test_context);
bool pcie_ide_test_config_pcrc_check_sel(void *test_context);

bool pcie_ide_test_config_pcrc_enable_link(void *test_context);
bool pcie_ide_test_config_pcrc_disable_link(void *test_context);
bool pcie_ide_test_config_pcrc_support_link(void *test_context);
bool pcie_ide_test_config_pcrc_check_link(void *test_context);

bool pcie_ide_test_config_pcrc_enable_sel_link(void *test_context);
bool pcie_ide_test_config_pcrc_disable_sel_link(void *test_context);
bool pcie_ide_test_config_pcrc_support_sel_link(void *test_context);
bool pcie_ide_test_config_pcrc_check_sel_link(void *test_context);

// seleceive ide for configuration request
bool pcie_ide_test_config_enable_sel_ide_for_cfg_req(void *test_context);
bool pcie_ide_test_config_disable_sel_ide_for_cfg_req(void *test_context);
bool pcie_ide_test_config_support_sel_ide_for_cfg_req(void *test_context);
bool pcie_ide_test_config_check_sel_ide_for_cfg_req(void *test_context);

// flit mode disable
bool pcie_ide_test_config_flit_mode_disable_enable_sel(void *test_context);
bool pcie_ide_test_config_flit_mode_disable_disable_sel(void *test_context);
bool pcie_ide_test_config_flit_mode_disable_support_sel(void *test_context);
bool pcie_ide_test_config_flit_mode_disable_check_sel(void *test_context);

bool pcie_ide_test_config_flit_mode_disable_enable_link(void *test_context);
bool pcie_ide_test_config_flit_mode_disable_disable_link(void *test_context);
bool pcie_ide_test_config_flit_mode_disable_support_link(void *test_context);
bool pcie_ide_test_config_flit_mode_disable_check_link(void *test_context);

bool pcie_ide_test_config_flit_mode_disable_enable_sel_link(void *test_context);
bool pcie_ide_test_config_flit_mode_disable_disable_sel_link(void *test_context);
bool pcie_ide_test_config_flit_mode_disable_support_sel_link(void *test_context);
bool pcie_ide_test_config_flit_mode_disable_check_sel_link(void *test_context);
//
// PCIE_IDE Test Group
//

// selective_ide test group
bool pcie_ide_test_group_setup_sel(void *test_context);
bool pcie_ide_test_group_teardown_sel(void *test_context);

// link_ide test group
bool pcie_ide_test_group_setup_link(void *test_context);
bool pcie_ide_test_group_teardown_link(void *test_context);

// selective_and_link_ide test group
bool pcie_ide_test_group_setup_sel_link(void *test_context);
bool pcie_ide_test_group_teardown_sel_link(void *test_context);

#endif