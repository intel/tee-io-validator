/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>

#include "assert.h"
#include "hal/base.h"
#include "hal/library/debuglib.h"

#include "hal/library/memlib.h"
#include "library/spdm_requester_lib.h"
#include "library/pci_ide_km_requester_lib.h"
#include "ide_test.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "pcie_ide_lib.h"
#include "pcie_ide_test_lib.h"
#include "pcie_ide_test_internal.h"

//
// case 3.3
// IDE_KM responder shall return valid K_GOSTOP_ACK, if it receives a K_SET_GO switch KeySet from 0 to 1.
//

extern const char *k_set_names[];

// case 3.3
bool pcie_ide_test_ksetgo_3_setup(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  ide_common_test_port_context_t* upper_port = &group_context->common.upper_port;
  ide_common_test_port_context_t* lower_port = &group_context->common.lower_port;

  // first setup ide_stream for KS0 (skip ksetgo)
  bool res = setup_ide_stream(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context, &group_context->spdm_doe.session_id,
                          upper_port->mapped_kcbar_addr, group_context->stream_id, PCI_IDE_KM_KEY_SET_K0,
                          &group_context->k_set, group_context->rp_stream_index,
                          0, group_context->common.top->type,
                          upper_port, lower_port, true);
  if(!res) {
    return false;
  }

  return true;
}

bool test_ksetgo_3_run_phase1(void* doe_context, void* spdm_context, uint32_t* session_id,
                              uint8_t stream_id, uint8_t rp_stream_index,
                              uint8_t* kcbar_addr, IDE_TEST_TOPOLOGY_TYPE top_type,
                              ide_common_test_port_context_t* upper_port,
                              ide_common_test_port_context_t* lower_port,
                              int case_class, int case_id)
{
  // Now KSetGo
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "KSetGo %s|RX|PR\n", k_set_names[PCI_IDE_KM_KEY_SET_K0]));
  test_ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_PR, 0, true,
                             "K0|RX|PR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "KSetGo %s|RX|NPR\n", k_set_names[PCI_IDE_KM_KEY_SET_K0]));
  test_ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, 0, true,
                             "K0|RX|NPR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "KSetGo %s|RX|CPL\n", k_set_names[PCI_IDE_KM_KEY_SET_K0]));
  test_ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, 0, true,
                             "K0|RX|CPL", case_class, case_id);

  // set key_set_select in host ide
  INTEL_KEYP_ROOT_COMPLEX_KCBAR *kcbar = (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)kcbar_addr;
  if(teeio_test_case_result(case_class, case_id) == TEEIO_TEST_RESULT_PASS) {
    set_rp_ide_key_set_select(kcbar, rp_stream_index, PCI_IDE_KM_KEY_SET_K0);
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "KSetGo %s|TX|PR\n", k_set_names[PCI_IDE_KM_KEY_SET_K0]));
  test_ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_PR, 0, true,
                             "K0|TX|PR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "KSetGo %s|TX|NPR\n", k_set_names[PCI_IDE_KM_KEY_SET_K0]));
  test_ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, 0, true,
                             "K0|TX|NPR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "KSetGo %s|TX|CPL\n", k_set_names[PCI_IDE_KM_KEY_SET_K0]));
  test_ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K0 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, 0, true,
                             "K0|TX|CPL", case_class, case_id);

  if(teeio_test_case_result(case_class, case_id) != TEEIO_TEST_RESULT_PASS) {
    return false;
  }

  // enable dev ide
  TEST_IDE_TYPE ide_type = TEST_IDE_TYPE_SEL_IDE;
  if (top_type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE)
  {
    ide_type = IDE_TEST_TOPOLOGY_TYPE_LINK_IDE;
  }
  else if(top_type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE)
  {
    NOT_IMPLEMENTED("selective_and_link_ide topoplogy");
  }
  enable_ide_stream_in_ecap(lower_port->cfg_space_fd, lower_port->ecap_offset, ide_type, lower_port->ide_id, true);

  // enable host ide stream
  enable_rootport_ide_stream(upper_port->cfg_space_fd,
                        upper_port->ecap_offset,
                        ide_type, upper_port->ide_id,
                        kcbar_addr,
                        rp_stream_index, true);

  // wait for 10 ms for device to get ide ready
  libspdm_sleep(10 * 1000);

  // Now ide stream shall be in secure state
  uint32_t data = read_stream_status_in_rp_ecap(upper_port->cfg_space_fd, upper_port->ecap_offset, ide_type, upper_port->ide_id);
  PCIE_SEL_IDE_STREAM_STATUS stream_status = {.raw = data};
  if (stream_status.state != IDE_STREAM_STATUS_SECURE)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "ide_stream state is %x.\n", stream_status.state));
    return false;
  }

  return true;
}

bool test_ksetgo_3_run_phase2(void* doe_context, void* spdm_context, uint32_t* session_id,
                              uint8_t stream_id, uint8_t rp_stream_index, uint8_t ide_id,
                              uint8_t* kcbar_addr, IDE_TEST_TOPOLOGY_TYPE top_type,
                              int case_class, int case_id)
{
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K1|RX|PR\n"));
  test_ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_PR, 0,
                             false, "K1|RX|PR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K1|RX|NPR\n"));
  test_ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, 0,
                              false, "K1|RX|NPR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K1|RX|CPL\n"));
  test_ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, 0,
                             false, "K1|RX|CPL", case_class, case_id);

  // set key_set_select in host ide
  INTEL_KEYP_ROOT_COMPLEX_KCBAR *kcbar = (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)kcbar_addr;
  if(teeio_test_case_result(case_class, case_id) == TEEIO_TEST_RESULT_PASS) {
    set_rp_ide_key_set_select(kcbar, rp_stream_index, PCIE_IDE_STREAM_KS1);
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K1|TX|PR\n"));
  test_ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_PR, 0,
                             false, "K1|TX|PR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K1|TX|NPR\n"));
  test_ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, 0,
                              false, "K1|TX|NPR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K1|TX|CPL\n"));
  test_ide_km_key_set_go(doe_context, spdm_context, session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, 0,
                              false, "K1|TX|CPL", case_class, case_id);

  if(teeio_test_case_result(case_class, case_id) == TEEIO_TEST_RESULT_PASS) {
    // wait for 10 ms for device to get ide ready
    libspdm_sleep(10 * 1000);
  }

  return true;
}

bool pcie_ide_test_ksetgo_3_run(void *test_context)
{
  bool res = false;

  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_run_test_case_t* test_case = case_context->test_case;
  TEEIO_ASSERT(test_case);
  int case_class = test_case->class_id;
  int case_id = test_case->case_id;

  pcie_ide_test_group_context_t *group_context = (pcie_ide_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  uint8_t stream_id = group_context->stream_id;
  void *doe_context = group_context->spdm_doe.doe_context;
  void *spdm_context = group_context->spdm_doe.spdm_context;
  uint32_t session_id = group_context->spdm_doe.session_id;

  ide_common_test_port_context_t* upper_port = &group_context->common.upper_port;
  ide_common_test_port_context_t* lower_port = &group_context->common.lower_port;

  // phase 1
  res = test_ksetgo_3_run_phase1(doe_context, spdm_context, &session_id,
                                stream_id, group_context->rp_stream_index,
                                upper_port->mapped_kcbar_addr, group_context->common.top->type,
                                upper_port, lower_port,
                                case_class, case_id);
  if(!res) {
    goto Done;
  }

  // then switch to KS1 (skip ksetgo)
  res = ide_key_switch_to(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context, &group_context->spdm_doe.session_id,
                          upper_port->mapped_kcbar_addr, group_context->stream_id,
                          &group_context->k_set, group_context->rp_stream_index,
                          0, group_context->common.top->type, upper_port, lower_port, PCIE_IDE_STREAM_KS1, true);
  if(!res) {
    goto Done;
  }

  // phase 2
  res = test_ksetgo_3_run_phase2(doe_context, spdm_context, &session_id,
                                stream_id, group_context->rp_stream_index, upper_port->ide_id,
                                upper_port->mapped_kcbar_addr, group_context->common.top->type,
                                case_class, case_id);

Done:
  return true;
}

bool pcie_ide_test_ksetgo_3_teardown(void *test_context)
{
  return pcie_ide_teardown_common(test_context, PCI_IDE_KM_KEY_SET_K1);
}
