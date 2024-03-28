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
#include "hal/library/platform_lib.h"
#include "library/spdm_requester_lib.h"
#include "library/pci_ide_km_requester_lib.h"
#include "ide_test.h"
#include "utils.h"
#include "teeio_debug.h"

//
// case 3.2
// IDE_KM responder shall return valid K_GOSTOP_ACK, if it receives a K_SET_GO with KeySet=1.
//

bool test_ksetgo_setup_common(
  void *doe_context, void* spdm_context,
  uint32_t* session_id, uint8_t* kcbar_addr,
  uint8_t stream_id, uint8_t rp_stream_index, uint8_t ide_id,
  ide_key_set_t *k_set, uint8_t port_index, uint8_t ks);

bool enable_ide_stream_in_ecap(int cfg_space_fd, uint32_t ecap_offset, TEST_IDE_TYPE ide_type, uint8_t ide_id, bool enable);
void enable_host_ide_stream(int cfg_space_fd, uint32_t ecap_offset, TEST_IDE_TYPE ide_type, uint8_t ide_id, uint8_t *kcbar_addr, uint8_t rp_stream_index, bool enable);
void set_host_ide_key_set(
    INTEL_KEYP_ROOT_COMPLEX_KCBAR *const kcbar_ptr,
    const uint8_t rp_stream_index,
    const uint8_t key_set_select);

libspdm_return_t test_ide_km_key_set_go(const void *pci_doe_context,
                                   void *spdm_context, const uint32_t *session_id,
                                   uint8_t stream_id, uint8_t key_sub_stream,
                                   uint8_t port_index,
                                   bool phase1, const char *assertion_msg);

bool test_ksetgo_2_setup(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  assert(case_context);
  assert(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_common_test_group_context_t *group_context = case_context->group_context;
  assert(group_context);
  assert(group_context->signature == GROUP_CONTEXT_SIGNATURE);
  assert(group_context->spdm_context);
  assert(group_context->session_id);

  return test_ksetgo_setup_common(group_context->doe_context, group_context->spdm_context, &group_context->session_id,
    group_context->upper_port.mapped_kcbar_addr, group_context->stream_id, group_context->rp_stream_index,
    group_context->upper_port.ide_id, group_context->k_set, 0, PCIE_IDE_STREAM_KS1);
}

bool test_ksetgo_2_run(void *test_context)
{
  libspdm_return_t status;
  bool res = false;

  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  assert(case_context);
  assert(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_common_test_group_context_t *group_context = (ide_common_test_group_context_t *)case_context->group_context;
  assert(group_context);
  assert(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  uint8_t stream_id = group_context->stream_id;
  void *doe_context = group_context->doe_context;
  void *spdm_context = group_context->spdm_context;
  uint32_t session_id = group_context->session_id;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K1|RX|PR\n"));
  status = test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_PR, 0,
                             true, "  Assertion 3.2");
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    goto Done;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K1|RX|NPR\n"));
  status = test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, 0,
                              true, "  Assertion 3.2");
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    goto Done;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K1|RX|CPL\n"));
  status = test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, 0,
                              true, "  Assertion 3.2");
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    goto Done;
  }

  // set key_set_select in host ide
  INTEL_KEYP_ROOT_COMPLEX_KCBAR *kcbar = (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)group_context->upper_port.mapped_kcbar_addr;
  set_host_ide_key_set(kcbar, group_context->rp_stream_index, PCIE_IDE_STREAM_KS0);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K1|TX|PR\n"));
  status = test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                             PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_PR, 0,
                             true, "  Assertion 3.2");
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    goto Done;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K1|TX|NPR\n"));
  status = test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, 0,
                              true, "  Assertion 3.2");
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    goto Done;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetGo K1|TX|CPL\n"));
  status = test_ide_km_key_set_go(doe_context, spdm_context, &session_id, stream_id,
                              PCI_IDE_KM_KEY_SET_K1 | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, 0,
                              true, "  Assertion 3.2");
  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    goto Done;
  }

  // enable dev ide
  TEST_IDE_TYPE ide_type = TEST_IDE_TYPE_SEL_IDE;
  if (group_context->top->type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE)
  {
    ide_type = IDE_TEST_TOPOLOGY_TYPE_LINK_IDE;
  }
  else if(group_context->top->type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE)
  {
    NOT_IMPLEMENTED("selective_and_link_ide topoplogy");
  }
  enable_ide_stream_in_ecap(group_context->lower_port.cfg_space_fd, group_context->lower_port.ecap_offset, ide_type, group_context->lower_port.ide_id, true);

  // enable host ide stream
  enable_host_ide_stream(group_context->upper_port.cfg_space_fd,
                         group_context->upper_port.ecap_offset,
                         ide_type, group_context->upper_port.ide_id,
                         group_context->upper_port.mapped_kcbar_addr,
                         group_context->rp_stream_index, true);

  res = true;

  // wait for 10 ms for device to get ide ready
  libspdm_sleep(10 * 1000);

Done:
  case_context->result = res ? IDE_COMMON_TEST_CASE_RESULT_SUCCESS : IDE_COMMON_TEST_CASE_RESULT_FAILED;

  return true;
}

bool test_ksetgo_2_teardown(void *test_context)
{
  return true;
}
