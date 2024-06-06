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

extern const char *k_set_names[];

bool setup_ide_stream(void* doe_context, void* spdm_context,
                                uint32_t* session_id, uint8_t* kcbar_addr,
                                uint8_t stream_id, uint8_t ks,
                                ide_key_set_t* k_set, uint8_t rp_stream_index,
                                uint8_t port_index, IDE_TEST_TOPOLOGY_TYPE top_type,
                                ide_common_test_port_context_t* upper_port,
                                ide_common_test_port_context_t* lower_port,
                                bool skip_ksetgo);

static const char* mAssertion[] = {
  "ide_km_ksetstop send receive_data",                // 0
  "sizeof(IdeKmMessage) == sizeof(K_GOSTOP_ACK)",     // 1
  "IdeKmMessage.ObjectID == K_GOSTOP_ACK",            // 2
  "IdeKmMessage.PortIndex == KEY_PROG.PortIndex",     // 3
  "IdeKmMessage.StreamID == KEY_PROG.StreamID",       // 4
  "IdeKmMessage.KeySet == KEY_PROG.KeySet && IdeKmMessage.RxTx == KEY_PROG.RxTx && IdeKmMessage.SubStream == KEY_PROG.SubStream"  // 5
};

bool test_pci_ide_km_key_set_stop(const void *pci_doe_context,
                            void *spdm_context, const uint32_t *session_id,
                            uint8_t stream_id, uint8_t key_sub_stream,
                            uint8_t port_index, const char* case_msg)
{
    libspdm_return_t status;
    pci_ide_km_k_set_stop_t request;
    size_t request_size;
    pci_ide_km_k_gostop_ack_t response;
    size_t response_size;

    libspdm_zero_mem (&request, sizeof(request));
    request.header.object_id = PCI_IDE_KM_OBJECT_ID_K_SET_STOP;
    request.stream_id = stream_id;
    request.key_sub_stream = key_sub_stream;
    request.port_index = port_index;

    // const char* case_msg = "  Assertion 4.1";

    request_size = sizeof(request);
    response_size = sizeof(response);
    status = pci_ide_km_send_receive_data(spdm_context, session_id,
                                          &request, request_size,
                                          &response, &response_size);
    // Assertion.0
    TEEIO_PRINT(("         %s.0: %s(0x%x) %s.\n", case_msg, mAssertion[0], status, !LIBSPDM_STATUS_IS_ERROR(status) ? "Pass" : "failed"));
    if (LIBSPDM_STATUS_IS_ERROR(status))
    {
        return false;
    }

    // Assertion.1
    bool res = response_size == sizeof(pci_ide_km_k_gostop_ack_t);
    TEEIO_PRINT(("         %s.1: %s %s\n", case_msg, mAssertion[1], res ? "Pass" : "Fail"));
    if(!res) {
        return false;
    }

    // Assertion.2
    res = response.header.object_id == PCI_IDE_KM_OBJECT_ID_K_SET_GOSTOP_ACK;
    TEEIO_PRINT(("         %s.2: %s %s\n", case_msg, mAssertion[2], res ? "Pass" : "Fail"));
    if(!res) {
        return false;
    }

    // Assertion.3
    res = (response.port_index == request.port_index);
    TEEIO_PRINT(("         %s.3: %s %s\n", case_msg, mAssertion[3], res ? "Pass" : "Fail"));
    if(!res) {
        return false;
    }

    // Assertion.4
    res = (response.stream_id == request.stream_id);
    TEEIO_PRINT(("         %s.4: %s %s\n", case_msg, mAssertion[4], res ? "Pass" : "Fail"));
    if(!res) {
        return false;
    }

    // Assertion.5
    res = (response.key_sub_stream == request.key_sub_stream);
    TEEIO_PRINT(("         %s.5: %s %s\n", case_msg, mAssertion[5], res ? "Pass" : "Fail"));
    if(!res) {
        return false;
    }

    return true;
}

// KSetStop Case 4.1
bool pcie_ide_test_ksetstop_1_setup(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_common_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  ide_common_test_port_context_t* upper_port = &group_context->upper_port;
  ide_common_test_port_context_t* lower_port = &group_context->lower_port;

  return setup_ide_stream(group_context->doe_context, group_context->spdm_context, &group_context->session_id,
                          upper_port->mapped_kcbar_addr, group_context->stream_id, PCI_IDE_KM_KEY_SET_K0,
                          group_context->k_set, group_context->rp_stream_index,
                          0, group_context->top->type, upper_port, lower_port, false);

}

bool pcie_ide_test_ksetstop_1_run(void *test_context)
{
  // first diable dev_ide and host_ide
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_common_test_group_context_t *group_context = case_context->group_context;
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
  uint8_t ks = PCI_IDE_KM_KEY_SET_K0;
  uint8_t port_index = 0;
  bool res = false;

  // then test KSetStop  
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetStop %s|RX|PR\n", k_set_names[ks]));
  res = test_pci_ide_km_key_set_stop(doe_context, spdm_context, &session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_PR, port_index,
                             "  Assertion 4.1");
  if(!res) {
    goto TestKSetStopCase1Done;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetStop %s|RX|NPR\n", k_set_names[ks]));
  res = test_pci_ide_km_key_set_stop(doe_context, spdm_context, &session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, port_index,
                             "  Assertion 4.1");
  if(!res) {
    goto TestKSetStopCase1Done;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetStop %s|RX|CPL\n", k_set_names[ks]));
  res = test_pci_ide_km_key_set_stop(doe_context, spdm_context, &session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, port_index,
                             "  Assertion 4.1");
  if(!res) {
    goto TestKSetStopCase1Done;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetStop %s|TX|PR\n", k_set_names[ks]));
  res = test_pci_ide_km_key_set_stop(doe_context, spdm_context, &session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_PR, port_index,
                             "  Assertion 4.1");
  if(!res) {
    goto TestKSetStopCase1Done;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetStop %s|TX|NPR\n", k_set_names[ks]));
  res = test_pci_ide_km_key_set_stop(doe_context, spdm_context, &session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, port_index,
                             "  Assertion 4.1");
  if(!res) {
    goto TestKSetStopCase1Done;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetStop %s|TX|CPL\n", k_set_names[ks]));
  res = test_pci_ide_km_key_set_stop(doe_context, spdm_context, &session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, port_index,
                             "  Assertion 4.1");

TestKSetStopCase1Done:
  case_context->result = res ? IDE_COMMON_TEST_CASE_RESULT_SUCCESS : IDE_COMMON_TEST_CASE_RESULT_FAILED;

  return true;
}

bool pcie_ide_test_ksetstop_1_teardown(void *test_context)
{
  return true;
}
