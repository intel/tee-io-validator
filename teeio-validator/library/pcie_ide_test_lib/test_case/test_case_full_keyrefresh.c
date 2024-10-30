/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>
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

static uint8_t mKeySet = 0;

// bool setup_ide_stream(void* doe_context, void* spdm_context,
//                                 uint32_t* session_id, uint8_t* kcbar_addr,
//                                 uint8_t stream_id, uint8_t ks,
//                                 ide_key_set_t* k_set, uint8_t rp_stream_index,
//                                 uint8_t port_index, IDE_TEST_TOPOLOGY_TYPE top_type,
//                                 ide_common_test_port_context_t* upper_port,
//                                 ide_common_test_port_context_t* lower_port,
//                                 bool skip_ksetgo);

// // key switch to @ks
// bool ide_key_switch_to(void* doe_context, void* spdm_context,
//                     uint32_t* session_id, uint8_t* kcbar_addr,
//                     uint8_t stream_id, ide_key_set_t* k_set, uint8_t rp_stream_index,
//                     uint8_t port_index, IDE_TEST_TOPOLOGY_TYPE top_type,
//                     ide_common_test_port_context_t* upper_port,
//                     ide_common_test_port_context_t* lower_port,
//                     uint8_t ks, bool skip_ksetgo);

bool pcie_ide_test_full_keyrefresh_setup(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  ide_common_test_port_context_t* upper_port = &group_context->upper_port;
  ide_common_test_port_context_t* lower_port = &group_context->lower_port;

  mKeySet = PCI_IDE_KM_KEY_SET_K0;

  // An ide_stream is first setup so that key_refresh can be tested in run.
  return setup_ide_stream(group_context->doe_context, group_context->spdm_context, &group_context->session_id,
                          upper_port->mapped_kcbar_addr, group_context->stream_id, mKeySet,
                          group_context->k_set, group_context->rp_stream_index,
                          0, group_context->top->type, upper_port, lower_port, false);
}

bool pcie_ide_test_full_keyrefresh_run(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = (pcie_ide_test_group_context_t *)case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->signature == GROUP_CONTEXT_SIGNATURE);

  uint8_t stream_id = group_context->stream_id;
  void *doe_context = group_context->doe_context;
  void *spdm_context = group_context->spdm_context;
  uint32_t session_id = group_context->session_id;

  ide_common_test_port_context_t* upper_port = &group_context->upper_port;
  ide_common_test_port_context_t* lower_port = &group_context->lower_port;

  TEST_IDE_TYPE ide_type = TEST_IDE_TYPE_SEL_IDE;
  if (group_context->top->type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE)
  {
    ide_type = IDE_TEST_TOPOLOGY_TYPE_LINK_IDE;
  }
  else if(group_context->top->type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE)
  {
    NOT_IMPLEMENTED("selective_and_link_ide topoplogy");
  }

  int cmd = 0;
  bool res = true;

  while(true){
    TEEIO_PRINT(("\n"));
    TEEIO_PRINT(("Current KeySet=%d. Check the registers below.\n", mKeySet));
    TEEIO_PRINT(("Print host registers.\n"));
    dump_rootport_registers(group_context->upper_port.mapped_kcbar_addr,
                      group_context->rp_stream_index,
                      group_context->upper_port.cfg_space_fd,
                      group_context->upper_port.ide_id,
                      group_context->upper_port.ecap_offset,
                      ide_type);

    TEEIO_PRINT(("\n"));
    TEEIO_PRINT(("Print device registers.\n"));
    dump_dev_registers(group_context->lower_port.cfg_space_fd,
                      group_context->lower_port.ide_id,
                      group_context->lower_port.ecap_offset,
                      ide_type);

    mKeySet = mKeySet == PCI_IDE_KM_KEY_SET_K0 ? PCI_IDE_KM_KEY_SET_K1 : PCI_IDE_KM_KEY_SET_K0;

    TEEIO_PRINT(("Press 'q' to quit test or any other keys to key_refresh to %d.\n", mKeySet));
    cmd = getchar();
    if(cmd == 'q' || cmd == 'Q') {
      break;
    } else {
      res = ide_key_switch_to(doe_context, spdm_context, &session_id,
                              upper_port->mapped_kcbar_addr, stream_id,
                              group_context->k_set, group_context->rp_stream_index,
                              0, group_context->top->type, upper_port, lower_port, mKeySet, false);
      if(!res) {
        break;
      }

    }
  }

  case_context->result = res ? IDE_COMMON_TEST_CASE_RESULT_SUCCESS : IDE_COMMON_TEST_CASE_RESULT_FAILED;

  return res;
}

bool pcie_ide_test_full_keyrefresh_teardown(void *test_context)
{
  return true;
}
