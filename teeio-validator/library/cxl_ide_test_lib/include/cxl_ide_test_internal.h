/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __CXL_IDE_TEST_INTERNAL_H__
#define __CXL_IDE_TEST_INTERNAL_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/param.h>
#include "ide_test.h"
#include "library/cxl_ide_km_requester_lib.h"

#pragma pack(1)
typedef struct {
    cxl_ide_km_header_t header;
    uint8_t reserved;
    uint8_t port_index;
    uint8_t dev_func_num;
    uint8_t bus_num;
    uint8_t segment;
    uint8_t max_port_index;
    uint8_t caps;
    uint32_t ide_reg_block[CXL_IDE_KM_IDE_CAP_REG_BLOCK_MAX_COUNT];
} cxl_ide_km_query_resp_teeio_t;

typedef struct {
    cxl_ide_km_header_t header;
    uint8_t reserved[2];
    uint8_t stream_id;
    uint8_t reserved2;
    uint8_t key_sub_stream;
    uint8_t port_index;
    cxl_ide_km_aes_256_gcm_key_buffer_t key_buffer;
} cxl_ide_km_key_prog_teeio_t;

#pragma pack()

// setup cxl ide stream
bool cxl_setup_ide_stream(void *doe_context, void *spdm_context,
                          uint32_t *session_id, uint8_t *kcbar_addr,
                          uint8_t stream_id, uint8_t port_index,
                          ide_common_test_port_context_t *upper_port,
                          ide_common_test_port_context_t *lower_port,
                          bool skip_ksetgo, uint32_t config_bitmap,
                          CXL_IDE_MODE ide_mode);
// stop cxl ide stream
bool cxl_stop_ide_stream(void *doe_context, void *spdm_context,
                         uint32_t *session_id, uint8_t *kcbar_addr,
                         uint8_t stream_id,
                         uint8_t port_index,
                         ide_common_test_port_context_t *upper_port,
                         ide_common_test_port_context_t *lower_port);

void test_cxl_ide_query(const void *pci_doe_context,
                        void *spdm_context, const uint32_t *session_id,
                        uint8_t port_index, uint8_t dev_func_num, uint8_t bus_num, uint8_t segment,
                        int case_class, int case_id);

bool cxl_ide_generate_key(const void *pci_doe_context,
                          void *spdm_context, const uint32_t *session_id,
                          uint8_t stream_id, uint8_t key_sub_stream, uint8_t port_index,
                          cxl_ide_km_aes_256_gcm_key_buffer_t *key_buffer,
                          bool key_iv_gen_capable, uint8_t direction,
                          uint8_t *cxl_ide_km_iv
                          );

typedef void(*cxl_ide_test_key_prog_t)(const void *doe_context,
                                   void *spdm_context, const uint32_t *session_id, int stream_id,
                                   uint8_t key_sub_stream, uint8_t port_index,
                                   const cxl_ide_km_aes_256_gcm_key_buffer_t *key_buffer, uint8_t* kp_ack_status,
                                   int case_class, int case_id);


void test_cxl_ide_key_prog(const void *doe_context,
                          void *spdm_context, const uint32_t *session_id, int stream_id, uint8_t sub_stream,
                          uint8_t port_index, bool key_iv_gen_capable, const char* case_info,
                          int case_class, int case_id, cxl_ide_test_key_prog_t test_func);

// Test cxl.key_prog with invalid parameters
// Invalid params includes:
//    stream_id, port_index, key_sub_stream
void test_cxl_ide_key_prog_invalid_params (
  const void *doe_context,
  void *spdm_context, const uint32_t *session_id, int stream_id,
  uint8_t key_sub_stream, uint8_t port_index,
  const cxl_ide_km_aes_256_gcm_key_buffer_t *key_buffer, uint8_t* kp_ack_status,
  int case_class, int case_id);

/**
 * Prepare the CXL IDE Keys with random values generated in host side
 */
bool cxl_ide_prepare_dynamic_keys (
  cxl_ide_km_aes_256_gcm_key_buffer_t* key_buffer,
  uint8_t* cxl_ide_km_iv);

/**
 * Prepare the CXL IDE Keys with random values generated in host side
 */
bool cxl_ide_prepare_keys_with_get_key (
  const void *doe_context, void *spdm_context,
  const uint32_t *session_id, int stream_id,
  uint8_t sub_stream, uint8_t port_index, 
  cxl_ide_km_aes_256_gcm_key_buffer_t* key_buffer, uint8_t* cxl_ide_km_iv);

bool cxl_ide_query(cxl_ide_test_group_context_t *group_context);

/**
 * get the dev_caps of the device
 */
bool cxl_ide_get_dev_caps(const void *doe_context,
                        void *spdm_context, const uint32_t *session_id,
                        int max_port_index, CXL_QUERY_RESP_CAPS* dev_caps);

#endif