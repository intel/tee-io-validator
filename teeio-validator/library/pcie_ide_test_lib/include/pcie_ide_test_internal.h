/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __PCIE_IDE_TEST_INTERNAL_H__
#define __PCIE_IDE_TEST_INTERNAL_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/param.h>

#include "library/pci_ide_km_requester_lib.h"

#pragma pack(1)
typedef struct {
    pci_ide_km_header_t header;
    uint8_t reserved[2];
    uint8_t stream_id;
    uint8_t reserved2;
    uint8_t key_sub_stream;
    uint8_t port_index;
    pci_ide_km_aes_256_gcm_key_buffer_t key_buffer;
} test_pci_ide_km_key_prog_t;

typedef struct
{
  pci_ide_km_header_t header;
  uint8_t reserved;
  uint8_t port_index;
  uint8_t dev_func_num;
  uint8_t bus_num;
  uint8_t segment;
  uint8_t max_port_index;
  uint32_t ide_reg_block[PCI_IDE_KM_IDE_REG_BLOCK_SUPPORTED_COUNT];
} test_pci_ide_km_query_resp_t;
#pragma pack()

void dump_key_iv(pci_ide_km_aes_256_gcm_key_buffer_t* key_buffer);

bool ide_km_key_prog(
    const void *pci_doe_context,
    void *spdm_context,
    const uint32_t *session_id,
    uint8_t ks,
    uint8_t direction,
    uint8_t substream,
    uint8_t port_index,
    uint8_t stream_id,
    uint8_t *kcbar_addr,
    ide_key_set_t *k_set,
    uint8_t rp_stream_index);

libspdm_return_t ide_km_key_set_go(const void *pci_doe_context,
                                   void *spdm_context, const uint32_t *session_id,
                                   uint8_t stream_id, uint8_t key_sub_stream,
                                   uint8_t port_index);

// setup ide stream
bool setup_ide_stream(void* doe_context, void* spdm_context,
                    uint32_t* session_id, uint8_t* kcbar_addr,
                    uint8_t stream_id, uint8_t ks,
                    ide_key_set_t* k_set, uint8_t rp_stream_index,
                    uint8_t port_index, IDE_TEST_TOPOLOGY_TYPE top_type,
                    ide_common_test_port_context_t* upper_port,
                    ide_common_test_port_context_t* lower_port,
                    bool skip_ksetgo);

// key switch to @ks
bool ide_key_switch_to(void* doe_context, void* spdm_context,
                    uint32_t* session_id, uint8_t* kcbar_addr,
                    uint8_t stream_id, ide_key_set_t* k_set, uint8_t rp_stream_index,
                    uint8_t port_index, IDE_TEST_TOPOLOGY_TYPE top_type,
                    ide_common_test_port_context_t* upper_port,
                    ide_common_test_port_context_t* lower_port,
                    uint8_t ks, bool skip_ksetgo);

bool test_keyprog_setup_common(void *test_context);

libspdm_return_t test_ide_km_key_set_go(const void *pci_doe_context,
                                   void *spdm_context, const uint32_t *session_id,
                                   uint8_t stream_id, uint8_t key_sub_stream,
                                   uint8_t port_index,
                                   bool phase1, const char *assertion_msg);

bool test_ksetgo_setup_common(
  void *doe_context, void* spdm_context,
  uint32_t* session_id, uint8_t* kcbar_addr,
  uint8_t stream_id, uint8_t rp_stream_index, uint8_t ide_id,
  ide_key_set_t *k_set, uint8_t port_index, uint8_t ks);

bool test_pci_ide_km_key_set_stop(const void *pci_doe_context,
                            void *spdm_context, const uint32_t *session_id,
                            uint8_t stream_id, uint8_t key_sub_stream,
                            uint8_t port_index, const char* case_msg);

/**
 * Dump key_iv in rootport registers
 * Refer to Root Complex IDE Key Configuration Unit Software Programing Guide Revision 1.01
 *   Figure 2-3 Key Slot
 *   Figure 2-4 IV Value Slot
 */
void pcie_dump_key_iv_in_rp(const char* direction, uint8_t *key, int key_size, uint8_t* iv, int iv_size);

#endif