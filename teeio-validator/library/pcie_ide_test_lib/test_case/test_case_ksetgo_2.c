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

#include "library/spdm_requester_lib.h"
#include "library/pci_ide_km_requester_lib.h"
#include "ide_test.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "pcie_ide_lib.h"
#include "pcie_ide_test_lib.h"
#include "pcie_ide_test_internal.h"

//
// case 3.2
// IDE_KM responder shall return valid K_GOSTOP_ACK, if it receives a K_SET_GO with KeySet=1.
//

bool pcie_ide_test_ksetgo_2_setup(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);
  TEEIO_ASSERT(group_context->spdm_doe.spdm_context);
  TEEIO_ASSERT(group_context->spdm_doe.session_id);

  return test_ksetgo_setup_common(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context, &group_context->spdm_doe.session_id,
    group_context->common.upper_port.mapped_kcbar_addr, group_context->stream_id, group_context->rp_stream_index,
    group_context->common.upper_port.ide_id, &group_context->k_set, group_context->common.lower_port.port->port_index, PCIE_IDE_STREAM_KS1);
}

void pcie_ide_test_ksetgo_2_run(void *test_context)
{
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
  uint8_t port_index = group_context->common.lower_port.port->port_index;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K1|RX|PR\n"));
  test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_PR, port_index,
                             true, "K1|RX|PR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K1|RX|NPR\n"));
  test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, port_index,
                              true, "K1|RX|NPR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K1|RX|CPL\n"));
  test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, port_index,
                              true, "K1|RX|CPL", case_class, case_id);

  // set key_set_select in host ide
  INTEL_KEYP_ROOT_COMPLEX_KCBAR *kcbar = (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)group_context->common.upper_port.mapped_kcbar_addr;
  if(teeio_test_case_result(case_class, case_id) == TEEIO_TEST_RESULT_PASS) {
    set_rp_ide_key_set_select(kcbar, group_context->rp_stream_index, PCIE_IDE_STREAM_KS1);
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K1|TX|PR\n"));
  test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_PR, port_index,
                             true, "K1|TX|PR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K1|TX|NPR\n"));
  test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, port_index,
                              true, "K1|TX|NPR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K1|TX|CPL\n"));
  test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, port_index,
                              true, "K1|TX|CPL", case_class, case_id);

  if(teeio_test_case_result(case_class, case_id) != TEEIO_TEST_RESULT_PASS) {
    return;
  }
  // enable dev ide
  TEST_IDE_TYPE ide_type = TEST_IDE_TYPE_SEL_IDE;
  if (group_context->common.top->type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE)
  {
    ide_type = IDE_TEST_TOPOLOGY_TYPE_LINK_IDE;
  }
  else if(group_context->common.top->type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE)
  {
    NOT_IMPLEMENTED("selective_and_link_ide topoplogy");
  }
  enable_ide_stream_in_ecap(group_context->common.lower_port.cfg_space_fd, group_context->common.lower_port.ecap_offset, ide_type, group_context->common.lower_port.ide_id, true);

  // enable host ide stream
  enable_rootport_ide_stream(group_context->common.upper_port.cfg_space_fd,
                        group_context->common.upper_port.ecap_offset,
                        ide_type, group_context->common.upper_port.ide_id,
                        group_context->common.upper_port.mapped_kcbar_addr,
                        group_context->rp_stream_index, true);

  // wait for 10 ms for device to get ide ready
  libspdm_sleep(10 * 1000);
}

void pcie_ide_test_ksetgo_2_teardown(void *test_context)
{
  pcie_ide_teardown_common(test_context, PCI_IDE_KM_KEY_SET_K1);
}
