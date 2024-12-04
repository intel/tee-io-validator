/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <ctype.h>

#include "hal/base.h"
#include "hal/library/debuglib.h"

#include "library/spdm_transport_pcidoe_lib.h"
#include "library/pci_ide_km_requester_lib.h"
#include "hal/library/memlib.h"
#include "ide_test.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "pcie_ide_test_internal.h"

static uint8_t mMaxPortIndex = 0;

bool
pcie_ide_test_query(const void *pci_doe_context,
                      void *spdm_context, const uint32_t *session_id,
                      uint8_t port_index, uint8_t *dev_func_num,
                      uint8_t *bus_num, uint8_t *segment, uint8_t *max_port_index,
                      uint32_t *ide_reg_buffer, uint32_t *ide_reg_buffer_count,
                      int case_class, int case_id);

// case 1.2
bool pcie_ide_test_query_2_setup(void *test_context)
{
  ide_common_test_case_context_t *case_context = (ide_common_test_case_context_t *)test_context;
  TEEIO_ASSERT(case_context);
  TEEIO_ASSERT(case_context->signature == CASE_CONTEXT_SIGNATURE);

  pcie_ide_test_group_context_t *group_context = case_context->group_context;
  TEEIO_ASSERT(group_context);
  TEEIO_ASSERT(group_context->common.signature == GROUP_CONTEXT_SIGNATURE);
  TEEIO_ASSERT(group_context->spdm_doe.spdm_context);
  TEEIO_ASSERT(group_context->spdm_doe.session_id);

  uint8_t port_index = 0;
  uint8_t dev_func = ((group_context->common.lower_port.port->device & 0x1f) << 3) | (group_context->common.lower_port.port->function & 0x7);
  uint8_t bus = group_context->common.lower_port.port->bus;
  uint8_t segment = 0;
  uint8_t max_port_index = 0;
  uint32_t ide_reg_block[PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT] = {0};
  uint32_t ide_reg_block_count = PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT;

  libspdm_return_t status = pci_ide_km_query(group_context->spdm_doe.doe_context,
                                            group_context->spdm_doe.spdm_context, &group_context->spdm_doe.session_id,
                                            port_index, &dev_func, &bus, &segment,
                                            &max_port_index, ide_reg_block, &ide_reg_block_count);

  if (LIBSPDM_STATUS_IS_ERROR(status)) {
    return false;
  }

  if (max_port_index == 0)
  {
    // skip the case
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Skip Query.2 because max_port_index == 0.\n"));
    case_context->action = IDE_COMMON_TEST_ACTION_SKIP;
  }

  mMaxPortIndex = max_port_index;

  return true;
}

bool pcie_ide_test_query_2_run(void *test_context)
{
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
  TEEIO_ASSERT(group_context->spdm_doe.spdm_context);
  TEEIO_ASSERT(group_context->spdm_doe.session_id);

  uint8_t port_index = mMaxPortIndex;
  uint8_t dev_func = ((group_context->common.lower_port.port->device & 0x1f) << 3) | (group_context->common.lower_port.port->function & 0x7);
  uint8_t bus = group_context->common.lower_port.port->bus;
  uint8_t segment = 0;
  uint8_t max_port_index = 0;
  uint32_t ide_reg_block[PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT] = {0};
  uint32_t ide_reg_block_count = PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT;

  pcie_ide_test_query(group_context->spdm_doe.doe_context,
                      group_context->spdm_doe.spdm_context,
                      &group_context->spdm_doe.session_id,
                      port_index, &dev_func, &bus, &segment,
                      &max_port_index, ide_reg_block, &ide_reg_block_count,
                      case_class, case_id);

  return true;
}

bool pcie_ide_test_query_2_teardown(void *test_context)
{
  mMaxPortIndex = 0;
  return true;
}
