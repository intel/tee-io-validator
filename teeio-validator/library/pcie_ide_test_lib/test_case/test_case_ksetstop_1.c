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

bool test_pci_ide_km_key_set_stop(const void *pci_doe_context,
                            void *spdm_context, const uint32_t *session_id,
                            uint8_t stream_id, uint8_t key_sub_stream,
                            uint8_t port_index, const char* case_info,
                            int case_class, int case_id)
{
    libspdm_return_t status;
    pci_ide_km_k_set_stop_t request;
    size_t request_size;
    pci_ide_km_k_gostop_ack_t response;
    size_t response_size;
    bool res;

    teeio_test_result_t assertion_result = TEEIO_TEST_RESULT_NOT_TESTED;

    teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_SEPARATOR, TEEIO_TEST_RESULT_NOT_TESTED, case_info);

    libspdm_zero_mem (&request, sizeof(request));
    request.header.object_id = PCI_IDE_KM_OBJECT_ID_K_SET_STOP;
    request.stream_id = stream_id;
    request.key_sub_stream = key_sub_stream;
    request.port_index = port_index;

    request_size = sizeof(request);
    response_size = sizeof(response);
    status = pci_ide_km_send_receive_data(spdm_context, session_id,
                                          &request, request_size,
                                          &response, &response_size);
    if (LIBSPDM_STATUS_IS_ERROR(status)) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "ide_km_kset_go send receive_data failed with status 0x%x\n", status));
        teeio_record_assertion_result(case_class, case_id, 0, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, TEEIO_TEST_RESULT_FAILED, "pci_ide_km_send_receive_data failed with 0x%x", status);
        return true;
    }

    // Assertion.1
    res = response_size == sizeof(pci_ide_km_kp_ack_t);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 1, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "sizeof(IdeKmMessage) = 0x%lx", response_size);
    if(!res) {
        return true;
    }

    // Assertion.2
    res = response.header.object_id == PCI_IDE_KM_OBJECT_ID_K_SET_GOSTOP_ACK;
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 2, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "IdeKmMessage.ObjectID = 0x%x", response.header.object_id);

    // Assertion.3
    res = (response.port_index == request.port_index);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 3, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "IdeKmMessage.PortIndex = 0x%x", response.port_index);

    // Assertion.4
    res = (response.stream_id == request.stream_id);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 4, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result, "IdeKmMessage.StreamID = 0x%x", response.stream_id);

    // Assertion.5
    res = (response.key_sub_stream == request.key_sub_stream);
    assertion_result = res ? TEEIO_TEST_RESULT_PASS : TEEIO_TEST_RESULT_FAILED;
    teeio_record_assertion_result(case_class, case_id, 5, IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST, assertion_result,
                                  "IdeKmMessage.KeySet = 0x%02x && IdeKmMessage.RxTx = 0x%02x && IdeKmMessage.SubStream = 0x%02x",
                                  response.key_sub_stream & PCI_IDE_KM_KEY_SET_MASK,
                                  response.key_sub_stream & PCI_IDE_KM_KEY_DIRECTION_MASK,
                                  response.key_sub_stream & PCI_IDE_KM_KEY_SUB_STREAM_MASK);
    return true;
}

// KSetStop Case 4.1
bool pcie_ide_test_ksetstop_1_setup(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  ide_common_test_port_context_t* upper_port = &group_context->common.upper_port;
  ide_common_test_port_context_t* lower_port = &group_context->common.lower_port;

  return setup_ide_stream(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context, &group_context->spdm_doe.session_id,
                          upper_port->mapped_kcbar_addr, group_context->stream_id, PCI_IDE_KM_KEY_SET_K0,
                          &group_context->k_set, group_context->rp_stream_index,
                          group_context->common.lower_port.port->port_index,
                          group_context->common.top->type, upper_port, lower_port, false);

}

void pcie_ide_test_ksetstop_1_run(void *test_context)
{
  // first diable dev_ide and host_ide
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  ide_run_test_case_t* test_case = case_context->test_case;
  TEEIO_ASSERT(test_case);
  int case_class = test_case->class_id;
  int case_id = test_case->case_id;

  pcie_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);

  ide_common_test_port_context_t* upper_port = &group_context->common.upper_port;
  ide_common_test_port_context_t* lower_port = &group_context->common.lower_port;

  IDE_TEST_TOPOLOGY_TYPE top_type = group_context->common.top->type;

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

  void* doe_context = group_context->spdm_doe.doe_context;
  void* spdm_context = group_context->spdm_doe.spdm_context;
  uint32_t session_id = group_context->spdm_doe.session_id;
  uint8_t stream_id = group_context->stream_id;
  uint8_t ks = PCI_IDE_KM_KEY_SET_K0;
  uint8_t port_index = group_context->common.lower_port.port->port_index;

  // then test KSetStop  
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetStop %s|RX|PR\n", k_set_names[ks]));

  test_pci_ide_km_key_set_stop(doe_context, spdm_context, &session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_PR, port_index,
                             "K0|RX|PR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetStop %s|RX|NPR\n", k_set_names[ks]));
  test_pci_ide_km_key_set_stop(doe_context, spdm_context, &session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, port_index,
                             "K0|RX|NPR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetStop %s|RX|CPL\n", k_set_names[ks]));
  test_pci_ide_km_key_set_stop(doe_context, spdm_context, &session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_RX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, port_index,
                             "K0|RX|CPL", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetStop %s|TX|PR\n", k_set_names[ks]));
  test_pci_ide_km_key_set_stop(doe_context, spdm_context, &session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_PR, port_index,
                             "K0|TX|PR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetStop %s|TX|NPR\n", k_set_names[ks]));
  test_pci_ide_km_key_set_stop(doe_context, spdm_context, &session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_NPR, port_index,
                             "K0|TX|NPR", case_class, case_id);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[idetest]       Test KSetStop %s|TX|CPL\n", k_set_names[ks]));
  test_pci_ide_km_key_set_stop(doe_context, spdm_context, &session_id, stream_id,
                             ks | PCI_IDE_KM_KEY_DIRECTION_TX | PCI_IDE_KM_KEY_SUB_STREAM_CPL, port_index,
                             "K0|TX|CPL", case_class, case_id);
}

void pcie_ide_test_ksetstop_1_teardown(void *test_context)
{
}
