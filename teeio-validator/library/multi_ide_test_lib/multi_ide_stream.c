/**
 *  Copyright Notice:
 *  Copyright 2026 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "hal/base.h"
#include "hal/library/debuglib.h"

#include "library/spdm_requester_lib.h"
#include "library/pci_ide_km_requester_lib.h"
#include "ide_test.h"
#include "helperlib.h"
#include "teeio_debug.h"
#include "pcie_ide_lib.h"
#include "pcie_ide_test_lib.h"
#include "cxl_ide_lib.h"
#include "cxl_ide_test_lib.h"
#include "multi_ide_test_internal.h"

extern int m_dev_fp;
extern uint32_t g_doe_extended_offset;

// Root Complex IDE Programming Guide 3.2: wait at least 100ms after terminating a stream before reusing its Stream ID or resources
#define MULTI_IDE_STREAM_REUSE_DELAY_US (100 * 1000)

// Set when a stream is torn down. Kept across test groups because they share the rootport resources.
static bool m_stream_torn_down = false;

IDE_TEST_TOPOLOGY_TYPE multi_ide_get_stream_top_type(multi_ide_stream_t *stream)
{
  TEEIO_ASSERT(stream->type != IDE_TEST_STREAM_TYPE_CXL_CM_IDE);
  return stream->type == IDE_TEST_STREAM_TYPE_LINK_IDE ? IDE_TEST_TOPOLOGY_TYPE_LINK_IDE : IDE_TEST_TOPOLOGY_TYPE_SEL_IDE;
}

static TEST_IDE_TYPE multi_ide_get_stream_ide_type(multi_ide_stream_t *stream)
{
  TEEIO_ASSERT(stream->type != IDE_TEST_STREAM_TYPE_CXL_CM_IDE);
  return stream->type == IDE_TEST_STREAM_TYPE_LINK_IDE ? TEST_IDE_TYPE_LNK_IDE : TEST_IDE_TYPE_SEL_IDE;
}

bool multi_ide_has_stream_type(multi_ide_test_group_context_t *group_context, bool cxl_cm)
{
  for(int i = 0; i < group_context->streams_cnt; i++) {
    if((group_context->streams[i].type == IDE_TEST_STREAM_TYPE_CXL_CM_IDE) == cxl_cm) {
      return true;
    }
  }

  return false;
}

// The PCIE IDE helpers use ide_id, tc and assoc blocks of the port context, so give them copies with the values of this stream.
static void multi_ide_get_port_views(multi_ide_test_group_context_t *group_context, multi_ide_stream_t *stream,
                                     ide_common_test_port_context_t *rp, ide_common_test_port_context_t *ep)
{
  *rp = group_context->common.upper_port;
  rp->ide_id = stream->rp_ide_id;
  rp->tc = stream->tc;
  *ep = group_context->common.lower_port;
  ep->ide_id = stream->dev_ide_id;
  ep->tc = stream->tc;

  // the device side keeps the whole address range because it targets the host memory
  if(stream->has_addr_range) {
    PCIE_SEL_IDE_ADDR_ASSOC_1 addr_assoc1 = {.valid = 1,
                                             .mem_base_lower = (stream->addr_base >> 20) & 0xfff,
                                             .mem_limit_lower = (stream->addr_limit >> 20) & 0xfff};
    rp->addr_assoc_reg_block.addr_assoc1.raw = addr_assoc1.raw;
    rp->addr_assoc_reg_block.addr_assoc2.raw = (uint32_t)(stream->addr_limit >> 32);
    rp->addr_assoc_reg_block.addr_assoc3.raw = (uint32_t)(stream->addr_base >> 32);
  }
}

/**
 * Check that the addr range of each selective_ide stream is inside the device memory window,
 * which is the range a selective_ide stream without addr gets.
 */
bool multi_ide_check_addr_ranges(multi_ide_test_group_context_t *group_context)
{
  PCIE_SEL_IDE_ADDR_ASSOC_REG_BLOCK *window = &group_context->common.upper_port.addr_assoc_reg_block;
  uint64_t window_base = ((uint64_t)window->addr_assoc3.mem_base_upper << 32) |
                         ((uint64_t)window->addr_assoc1.mem_base_lower << 20);
  uint64_t window_limit = ((uint64_t)window->addr_assoc2.mem_limit_upper << 32) |
                          ((uint64_t)window->addr_assoc1.mem_limit_lower << 20) | (IDE_ADDR_ASSOC_GRANULARITY - 1);
  bool result = true;

  for(int i = 0; i < group_context->streams_cnt; i++) {
    multi_ide_stream_t *stream = &group_context->streams[i];
    if(!stream->has_addr_range) {
      continue;
    }
    if(stream->addr_base < window_base || stream->addr_limit > window_limit) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[Topology_%d] addr 0x%llx-0x%llx of %s is outside the device memory window 0x%llx-0x%llx.\n",
                   group_context->common.top->id, (unsigned long long)stream->addr_base, (unsigned long long)stream->addr_limit,
                   stream->name, (unsigned long long)window_base, (unsigned long long)window_limit));
      result = false;
    }
  }

  return result;
}

CXL_IDE_MODE multi_ide_get_cxl_ide_mode(multi_ide_test_group_context_t *group_context)
{
  IDE_TEST_CONFIGURATION *configuration = get_configuration_by_id(group_context->common.suite_context->test_config, group_context->common.config_id);
  TEEIO_ASSERT(configuration);

  return configuration->priv_data.cxl_ide.ide_mode;
}

/**
 * Open the rootport and the device port. The CXL.cachemem registers and kcbar
 * are mapped in addition to the PCIE ones if there is a cxl_cm_ide stream.
 * On failure the caller shall call multi_ide_close_ports().
 */
bool multi_ide_open_ports(multi_ide_test_group_context_t *group_context)
{
  ide_common_test_port_context_t *upper_port = &group_context->common.upper_port;
  ide_common_test_port_context_t *lower_port = &group_context->common.lower_port;
  IDE_TEST_TOPOLOGY *top = group_context->common.top;
  bool has_cxl = multi_ide_has_stream_type(group_context, true);

  TEEIO_ASSERT(top->connection == IDE_TEST_CONNECT_DIRECT);
  TEEIO_ASSERT(upper_port->port->id == group_context->common.root_port.port->id);

  if(!scan_devices_at_bus(group_context->common.root_port.port, lower_port->port, NULL, top->segment, top->bus)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Scan device failed.\n"));
    return false;
  }
  upper_port->port->bus = group_context->common.root_port.port->bus;
  strncpy(upper_port->port->bdf, group_context->common.root_port.port->bdf, BDF_LENGTH);

  if(!open_dev_port(lower_port)) {
    // open_dev_port closes the fd on failure
    lower_port->cfg_space_fd = 0;
    return false;
  }
  if(has_cxl && !cxl_map_dev_port_regs(lower_port, lower_port->cfg_space_fd)) {
    return false;
  }
  populate_dev_assoc_reg_blocks(lower_port);

  if(!open_root_port(upper_port)) {
    // open_root_port closes the fd on failure
    upper_port->cfg_space_fd = 0;
    return false;
  }
  if(has_cxl) {
    if(!cxl_map_root_port_regs(upper_port, upper_port->cfg_space_fd, &upper_port->mapped_cxl_kcbar_addr, &upper_port->cxl_kcbar_fd)) {
      return false;
    }

    INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *cxl_kcbar = (INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)upper_port->mapped_cxl_kcbar_addr;
    cxl_reset_kcbar(cxl_kcbar);
    cxl_clear_kcbar_key_ivs(cxl_kcbar);

    if(!cxl_ide_set_key_refresh_control_reg(upper_port, lower_port) ||
       !cxl_ide_set_truncation_transmit_control_reg(upper_port, lower_port)) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Set Key Refresh / Truncation transmit control registers failed.\n"));
      return false;
    }
  }

  return populate_rp_assoc_reg_blocks(upper_port, lower_port, upper_port->port->bdf);
}

void multi_ide_close_ports(multi_ide_test_group_context_t *group_context)
{
  ide_common_test_port_context_t *upper_port = &group_context->common.upper_port;
  ide_common_test_port_context_t *lower_port = &group_context->common.lower_port;

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "multi_ide_close_ports %s(%s) %s(%s)\n",
               upper_port->port->port_name, upper_port->port->bdf,
               lower_port->port->port_name, lower_port->port->bdf));

  cxl_unmap_memcache_reg_block(upper_port->cxl_data.memcache.mapped_fd, upper_port->cxl_data.memcache.mapped_memcache_reg_block);
  memset(&upper_port->cxl_data, 0, sizeof(CXL_PRIV_DATA));

  if(upper_port->cxl_kcbar_fd > 0) {
    unmap_kcbar_addr(upper_port->cxl_kcbar_fd, upper_port->mapped_cxl_kcbar_addr);
  }
  upper_port->cxl_kcbar_fd = 0;
  upper_port->mapped_cxl_kcbar_addr = NULL;

  if(upper_port->kcbar_fd > 0) {
    unmap_kcbar_addr(upper_port->kcbar_fd, upper_port->mapped_kcbar_addr);
  }
  upper_port->kcbar_fd = 0;
  upper_port->mapped_kcbar_addr = NULL;

  if(upper_port->cfg_space_fd > 0) {
    close(upper_port->cfg_space_fd);
    unset_device_info(upper_port->cfg_space_fd);
  }
  upper_port->cfg_space_fd = 0;

  cxl_unmap_memcache_reg_block(lower_port->cxl_data.memcache.mapped_fd, lower_port->cxl_data.memcache.mapped_memcache_reg_block);
  memset(&lower_port->cxl_data, 0, sizeof(CXL_PRIV_DATA));

  if(lower_port->cfg_space_fd > 0) {
    close(lower_port->cfg_space_fd);
    unset_device_info(lower_port->cfg_space_fd);
  }
  lower_port->cfg_space_fd = 0;

  m_dev_fp = 0;
  g_doe_extended_offset = 0;
}

static bool multi_ide_setup_pcie_stream(multi_ide_test_group_context_t *group_context, multi_ide_stream_t *stream)
{
  ide_common_test_port_context_t *upper_port = &group_context->common.upper_port;
  ide_common_test_port_context_t *lower_port = &group_context->common.lower_port;
  IDE_TEST_TOPOLOGY_TYPE top_type = multi_ide_get_stream_top_type(stream);
  TEST_IDE_TYPE ide_type = multi_ide_get_stream_ide_type(stream);
  ide_common_test_port_context_t rp;
  ide_common_test_port_context_t ep;

  // The allocators skip only enabled streams, so allocate right before this stream is set up.
  if(!find_free_rp_stream_index_and_ide_id(upper_port, &stream->rp_stream_index, &stream->rp_ide_id, top_type, true) ||
     !find_free_rp_stream_index_and_ide_id(lower_port, NULL, &stream->dev_ide_id, top_type, false) ||
     !pcie_ide_alloc_slot_ids(upper_port, stream->rp_stream_index, &stream->k_set)) {
    return false;
  }
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%s: rp_ide_id=%d, dev_ide_id=%d, rp_stream_index=%d\n",
               stream->name, stream->rp_ide_id, stream->dev_ide_id, stream->rp_stream_index));

  multi_ide_get_port_views(group_context, stream, &rp, &ep);

  stream->state = MULTI_IDE_STREAM_STATE_CONFIGURED;
  reset_ide_registers(&rp, top_type, stream->stream_id, stream->rp_stream_index, true);
  reset_ide_registers(&ep, top_type, stream->stream_id, stream->rp_stream_index, false);

  if(setup_ide_stream(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context,
                      &group_context->spdm_doe.session_id, stream->kcbar, stream->stream_id,
                      PCI_IDE_KM_KEY_SET_K0, &stream->k_set, stream->rp_stream_index,
                      stream->port_index, top_type, &rp, &ep, false)) {
    stream->state = MULTI_IDE_STREAM_STATE_ACTIVE;
    return true;
  }

  // setup_ide_stream also fails if the enabled stream is not Secure. That is checked in run().
  if(is_ide_enabled(rp.cfg_space_fd, ide_type, rp.ide_id, rp.ecap_offset) &&
     is_ide_enabled(ep.cfg_space_fd, ide_type, ep.ide_id, ep.ecap_offset)) {
    stream->state = MULTI_IDE_STREAM_STATE_ACTIVE;
    return true;
  }

  return false;
}

static bool multi_ide_setup_cxl_stream(multi_ide_test_group_context_t *group_context, multi_ide_stream_t *stream)
{
  CXL_IDE_MODE ide_mode = multi_ide_get_cxl_ide_mode(group_context);

  cxl_cfg_rp_mode((INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)stream->kcbar,
                  ide_mode == CXL_IDE_MODE_CONTAINMENT ? INTEL_CXL_IDE_MODE_CONTAINMENT : INTEL_CXL_IDE_MODE_SKID);

  stream->state = MULTI_IDE_STREAM_STATE_CONFIGURED;
  if(!cxl_setup_ide_stream(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context,
                           &group_context->spdm_doe.session_id, stream->kcbar, stream->stream_id,
                           stream->port_index, &group_context->common.upper_port, &group_context->common.lower_port,
                           false, 0, ide_mode, false)) {
    return false;
  }

  stream->state = MULTI_IDE_STREAM_STATE_ACTIVE;
  return true;
}

bool multi_ide_setup_stream(multi_ide_test_group_context_t *group_context, multi_ide_stream_t *stream)
{
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Set up %s\n", stream->name));

  if(m_stream_torn_down) {
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Wait %dms after the last stream teardown before the resources are reused.\n",
                 MULTI_IDE_STREAM_REUSE_DELAY_US / 1000));
    libspdm_sleep(MULTI_IDE_STREAM_REUSE_DELAY_US);
    m_stream_torn_down = false;
  }

  if(stream->type == IDE_TEST_STREAM_TYPE_CXL_CM_IDE) {
    return multi_ide_setup_cxl_stream(group_context, stream);
  }

  return multi_ide_setup_pcie_stream(group_context, stream);
}

bool multi_ide_setup_all_streams(multi_ide_test_group_context_t *group_context)
{
  for(int i = 0; i < group_context->streams_cnt; i++) {
    multi_ide_stream_t *stream = &group_context->streams[i];
    if(!multi_ide_setup_stream(group_context, stream)) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Set up %s failed. Tear down the streams which are set up.\n", stream->name));
      multi_ide_teardown_all_streams(group_context);
      return false;
    }
  }

  return true;
}

static bool multi_ide_teardown_pcie_stream(multi_ide_test_group_context_t *group_context, multi_ide_stream_t *stream)
{
  IDE_TEST_TOPOLOGY_TYPE top_type = multi_ide_get_stream_top_type(stream);
  TEST_IDE_TYPE ide_type = multi_ide_get_stream_ide_type(stream);
  ide_common_test_port_context_t rp;
  ide_common_test_port_context_t ep;
  INTEL_KEYP_KEY_SLOT keys = {0};
  INTEL_KEYP_IV_SLOT iv = {0};

  multi_ide_get_port_views(group_context, stream, &rp, &ep);

  enable_ide_stream_in_ecap(ep.cfg_space_fd, ep.ecap_offset, ide_type, ep.ide_id, false);
  enable_rootport_ide_stream(rp.cfg_space_fd, rp.ecap_offset, ide_type, rp.ide_id,
                             stream->kcbar, stream->rp_stream_index, false);

  // PCIe 6.2 Sections 7.9.26.4.2 / 7.9.26.5.3: Stream State must be Insecure once Stream Enable is Cleared
  bool insecure = is_ide_stream_in_state(IDE_STREAM_STATUS_INSECURE, ide_type,
                                         rp.cfg_space_fd, rp.ecap_offset, rp.ide_id,
                                         ep.cfg_space_fd, ep.ecap_offset, ep.ide_id);

  if(!pcie_ide_key_set_stop_all(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context,
                                &group_context->spdm_doe.session_id, stream->stream_id,
                                PCI_IDE_KM_KEY_SET_K0, stream->port_index)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "K_SET_STOP of %s failed.\n", stream->name));
  }

  // overwrite the keys in the rootport key/iv slots of this stream
  for(int direction = 0; direction < PCIE_IDE_STREAM_DIRECTION_NUM; direction++) {
    for(int sub_stream = 0; sub_stream < PCIE_IDE_SUB_STREAM_NUM; sub_stream++) {
      cfg_rootport_ide_keys((INTEL_KEYP_ROOT_COMPLEX_KCBAR *)stream->kcbar, stream->rp_stream_index,
                            direction, PCIE_IDE_STREAM_KS0, sub_stream,
                            stream->k_set.slot_id[direction][sub_stream], &keys, &iv);
    }
  }

  reset_ide_registers(&rp, top_type, stream->stream_id, stream->rp_stream_index, true);
  reset_ide_registers(&ep, top_type, stream->stream_id, stream->rp_stream_index, false);

  return insecure;
}

static bool multi_ide_teardown_cxl_stream(multi_ide_test_group_context_t *group_context, multi_ide_stream_t *stream)
{
  INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *kcbar_ptr = (INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)stream->kcbar;
  bool insecure = false;

  if(!cxl_teardown_ide_stream_common(group_context->spdm_doe.doe_context, group_context->spdm_doe.spdm_context,
                                     &group_context->spdm_doe.session_id, stream->kcbar, stream->stream_id,
                                     stream->port_index, &group_context->common.upper_port,
                                     &group_context->common.lower_port, &insecure)) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "K_SET_STOP of %s failed.\n", stream->name));
    insecure = false;
    // cxl_teardown_ide_stream_common returns before the rootport leaves the secure mode
    cxl_cfg_rp_txrx_transto_insecure_state(kcbar_ptr, CXL_IDE_STREAM_DIRECTION_RX, true);
    cxl_cfg_rp_txrx_transto_insecure_state(kcbar_ptr, CXL_IDE_STREAM_DIRECTION_TX, true);
    cxl_cfg_rp_linkenc_enable(kcbar_ptr, false);
  }

  cxl_reset_kcbar(kcbar_ptr);
  cxl_clear_kcbar_key_ivs(kcbar_ptr);

  return insecure;
}

bool multi_ide_teardown_stream(multi_ide_test_group_context_t *group_context, multi_ide_stream_t *stream)
{
  bool insecure;

  if(stream->state == MULTI_IDE_STREAM_STATE_IDLE) {
    return true;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Tear down %s\n", stream->name));

  if(stream->type == IDE_TEST_STREAM_TYPE_CXL_CM_IDE) {
    insecure = multi_ide_teardown_cxl_stream(group_context, stream);
  } else {
    insecure = multi_ide_teardown_pcie_stream(group_context, stream);
  }
  stream->state = MULTI_IDE_STREAM_STATE_IDLE;
  m_stream_torn_down = true;

  return insecure;
}

void multi_ide_teardown_all_streams(multi_ide_test_group_context_t *group_context)
{
  for(int i = 0; i < group_context->streams_cnt; i++) {
    multi_ide_teardown_stream(group_context, &group_context->streams[i]);
  }
}

bool multi_ide_is_stream_active(multi_ide_test_group_context_t *group_context, multi_ide_stream_t *stream)
{
  ide_common_test_port_context_t rp;
  ide_common_test_port_context_t ep;

  if(stream->type == IDE_TEST_STREAM_TYPE_CXL_CM_IDE) {
    uint8_t expected = multi_ide_get_cxl_ide_mode(group_context) == CXL_IDE_MODE_CONTAINMENT ?
                       CXL_IDE_STATUS_ACTIVE_CONTAINMENT : CXL_IDE_STATUS_ACTIVE_SKID;
    bool host_res = cxl_check_ide_status("host", &group_context->common.upper_port.cxl_data.memcache, expected);
    bool dev_res = cxl_check_ide_status("device", &group_context->common.lower_port.cxl_data.memcache, expected);
    return host_res && dev_res;
  }

  multi_ide_get_port_views(group_context, stream, &rp, &ep);
  return is_ide_stream_in_state(IDE_STREAM_STATUS_SECURE, multi_ide_get_stream_ide_type(stream),
                                rp.cfg_space_fd, rp.ecap_offset, rp.ide_id,
                                ep.cfg_space_fd, ep.ecap_offset, ep.ide_id);
}

void multi_ide_dump_stream(multi_ide_test_group_context_t *group_context, multi_ide_stream_t *stream)
{
  ide_common_test_port_context_t *upper_port = &group_context->common.upper_port;
  ide_common_test_port_context_t *lower_port = &group_context->common.lower_port;

  TEEIO_PRINT(("\n"));
  TEEIO_PRINT(("Print registers of %s.\n", stream->name));

  if(stream->type == IDE_TEST_STREAM_TYPE_CXL_CM_IDE) {
    cxl_dump_kcbar((INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR *)stream->kcbar);
    cxl_dump_ide_status(upper_port->cxl_data.memcache.cap_headers, upper_port->cxl_data.memcache.cap_headers_cnt,
                        upper_port->cxl_data.memcache.mapped_memcache_reg_block);
    cxl_dump_ide_status(lower_port->cxl_data.memcache.cap_headers, lower_port->cxl_data.memcache.cap_headers_cnt,
                        lower_port->cxl_data.memcache.mapped_memcache_reg_block);
    return;
  }

  TEST_IDE_TYPE ide_type = multi_ide_get_stream_ide_type(stream);
  dump_rootport_registers(stream->kcbar, stream->rp_stream_index, upper_port->cfg_space_fd,
                          stream->rp_ide_id, upper_port->ecap_offset, ide_type);
  dump_dev_registers(lower_port->cfg_space_fd, stream->dev_ide_id, lower_port->ecap_offset, ide_type);
}
