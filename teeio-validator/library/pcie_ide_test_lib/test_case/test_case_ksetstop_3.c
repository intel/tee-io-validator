/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>

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

extern const char *k_set_names[];

// KSetStop Case 4.3
bool pcie_ide_test_ksetstop_3_setup(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  ide_common_test_port_context_t* upper_port = &group_context->upper_port;
  ide_common_test_port_context_t* lower_port = &group_context->lower_port;

  // first setup ide_stream for KS0
  bool res = setup_ide_stream(group_context->doe_context, group_context->spdm_context, &group_context->session_id,
                          upper_port->mapped_kcbar_addr, group_context->stream_id, PCI_IDE_KM_KEY_SET_K0,
                          group_context->k_set, group_context->rp_stream_index,
                          0, group_context->top->type, upper_port, lower_port, false);
  if(!res) {
    return false;
  }

  // then switch to KS1
  res = ide_key_switch_to(group_context->doe_context, group_context->spdm_context, &group_context->session_id,
                          upper_port->mapped_kcbar_addr, group_context->stream_id,
                          group_context->k_set, group_context->rp_stream_index,
                          0, group_context->top->type, upper_port, lower_port, PCIE_IDE_STREAM_KS1, false);

  return res;
}

bool pcie_ide_test_ksetstop_3_run(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  ide_common_test_port_context_t* upper_port = &group_context->upper_port;
  ide_common_test_port_context_t* lower_port = &group_context->lower_port;

  IDE_TEST_TOPOLOGY_TYPE top_type = group_context->top->type;

  // disable dev ide
  TEST_IDE_TYPE ide_type = TEST_IDE_TYPE_SEL_IDE;
  if (top_type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE)
  {
    ide_type = IDE_TEST_TOPOLOGY_TYPE_LINK_IDE;
  }
  else if(top_type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE)
  {
    NOT_IMPLEMENTED("selective_and_link_ide topoplogy");
  }
  enable_ide_stream_in_ecap(lower_port->cfg_space_fd, lower_port->ecap_offset, ide_type, lower_port->ide_id, false);

  // disable host ide stream
  enable_rootport_ide_stream(upper_port->cfg_space_fd,
                         upper_port->ecap_offset,
                         ide_type, upper_port->ide_id,
                         upper_port->mapped_kcbar_addr,
                         group_context->rp_stream_index, false);

  void* doe_context = group_context->doe_context;
  void* spdm_context = group_context->spdm_context;
  uint32_t session_id = group_context->session_id;
  uint8_t stream_id = group_context->stream_id;
  uint8_t port_index = 0;
  bool res = false;

  // then test KSetStop  
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetStop %s|RX|PR\n", k_set_names[PCI_IDE_KM_KEY_SET_K1]));
  res = test_pci_ide_km_key_set_stop(doe_context, spdm_context, &session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_PR, port_index,
                             "  Assertion 4.3");
  if(!res) {
    goto TestKSetStopCase3Done;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetStop %s|RX|NPR\n", k_set_names[PCI_IDE_KM_KEY_SET_K1]));
  res = test_pci_ide_km_key_set_stop(doe_context, spdm_context, &session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, port_index,
                             "  Assertion 4.3");
  if(!res) {
    goto TestKSetStopCase3Done;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetStop %s|RX|CPL\n", k_set_names[PCI_IDE_KM_KEY_SET_K1]));
  res = test_pci_ide_km_key_set_stop(doe_context, spdm_context, &session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, port_index,
                             "  Assertion 4.3");
  if(!res) {
    goto TestKSetStopCase3Done;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetStop %s|TX|PR\n", k_set_names[PCI_IDE_KM_KEY_SET_K1]));
  res = test_pci_ide_km_key_set_stop(doe_context, spdm_context, &session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_PR, port_index,
                             "  Assertion 4.3");
  if(!res) {
    goto TestKSetStopCase3Done;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetStop %s|TX|NPR\n", k_set_names[PCI_IDE_KM_KEY_SET_K1]));
  res = test_pci_ide_km_key_set_stop(doe_context, spdm_context, &session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, port_index,
                             "  Assertion 4.3");
  if(!res) {
    goto TestKSetStopCase3Done;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetStop %s|TX|CPL\n", k_set_names[PCI_IDE_KM_KEY_SET_K1]));
  res = test_pci_ide_km_key_set_stop(doe_context, spdm_context, &session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, port_index,
                             "  Assertion 4.3");

TestKSetStopCase3Done:
  case_context->result = res ? IDE_COMMON_TEST_CASE_RESULT_SUCCESS : IDE_COMMON_TEST_CASE_RESULT_FAILED;

  return true;
}

bool pcie_ide_test_ksetstop_3_teardown(void *test_context)
{
  return true;
}
