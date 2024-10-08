/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __CXL_IDE_H__
#define __CXL_IDE_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/param.h>
#include "cxl.h"
#include "intel_keyp.h"

#define MAX_CXL_QUERY_CASE_ID 2
#define MAX_CXL_KEYPROG_CASE_ID 9
#define MAX_CXL_KSETGO_CASE_ID 1
#define MAX_CXL_KSETSTOP_CASE_ID 1
#define MAX_CXL_GETKEY_CASE_ID 1
#define MAX_CXL_FULL_CASE_ID 1
#define MAX_CXL_CASE_ID \
  (MAX(MAX_CXL_QUERY_CASE_ID, MAX(MAX_CXL_KEYPROG_CASE_ID, MAX(MAX_CXL_KSETGO_CASE_ID, MAX(MAX_CXL_KSETSTOP_CASE_ID, MAX(MAX_CXL_GETKEY_CASE_ID, MAX_CXL_FULL_CASE_ID))))))

#define MAX_IDE_TEST_DVSEC_COUNT  16

typedef enum {
  CXL_MEM_IDE_TEST_CASE_QUERY = 0,
  CXL_MEM_IDE_TEST_CASE_KEYPROG,
  CXL_MEM_IDE_TEST_CASE_KSETGO,
  CXL_MEM_IDE_TEST_CASE_KSETSTOP,
  CXL_MEM_IDE_TEST_CASE_GETKEY,
  CXL_MEM_IDE_TEST_CASE_TEST,
  CXL_MEM_IDE_TEST_CASE_NUM
} CXL_MEM_IDE_TEST_CASE;

typedef enum {
  CXL_IDE_CONFIGURATION_TYPE_DEFAULT = 0,
  CXL_IDE_CONFIGURATION_TYPE_PCRC,
  CXL_IDE_CONFIGURATION_TYPE_IDE_STOP,
  CXL_IDE_CONFIGURATION_TYPE_SKID_MODE,
  CXL_IDE_CONFIGURATION_TYPE_CONTAINMENT_MODE,
  CXL_IDE_CONFIGURATION_TYPE_NUM
} CXL_IDE_CONFIGURATION_TYPE;

#define CXL_BIT_MASK(n) ((uint32_t)(1<<n))
#define CXL_LINK_IDE_CONFIGURATION_BITMASK \
  ((CXL_BIT_MASK(CXL_IDE_CONFIGURATION_TYPE_DEFAULT)) | \
  (CXL_BIT_MASK(CXL_IDE_CONFIGURATION_TYPE_PCRC)) | \
  (CXL_BIT_MASK(CXL_IDE_CONFIGURATION_TYPE_IDE_STOP)) | \
  (CXL_BIT_MASK(CXL_IDE_CONFIGURATION_TYPE_SKID_MODE)) | \
  (CXL_BIT_MASK(CXL_IDE_CONFIGURATION_TYPE_CONTAINMENT_MODE)))

#pragma pack(1)

typedef struct {
  uint32_t offset;          // offset in configuration space
  CXL_DVSEC_ID dvsec_id;    // DVSEC ID. 0xff is invalid DVSEC
} IDE_TEST_CXL_PCIE_DVSEC;

typedef struct {
  // DVSECs in Configuration Space
  IDE_TEST_CXL_PCIE_DVSEC dvsecs[MAX_IDE_TEST_DVSEC_COUNT];
  int dvsec_cnt;

  CXL_DEV_CAPABILITY cap;
  CXL_DEV_CAPABILITY2 cap2;
  CXL_DEV_CAPABILITY3 cap3;
} CXL_PRIV_DATA_ECAP;

typedef struct {
  // KCBar data
  INTEL_KEYP_CXL_LINK_ENC_GLOBAL_CONFIG link_enc_global_config;
} CXL_PRIV_DATA_KCBAR;

typedef struct {
  int mapped_fd;
  uint8_t* mapped_memcache_reg_block;

  CXL_CAPABILITY_XXX_HEADER cap_headers[CXL_CAPABILITY_ID_NUM];
  int cap_headers_cnt;

  CXL_IDE_CAPABILITY ide_cap;
} CXL_PRIV_DATA_MEMCACHE_REG_DATA;

typedef struct {
  uint8_t max_port_index;
  uint8_t dev_func_num;
  uint8_t bus_num;
  uint8_t segment;
  uint8_t caps;
} CXL_PRIV_DATA_QUERY_RESP_DATA;

typedef struct {
  CXL_PRIV_DATA_ECAP ecap;
  CXL_PRIV_DATA_KCBAR kcbar;
  CXL_PRIV_DATA_MEMCACHE_REG_DATA memcache;
  CXL_PRIV_DATA_QUERY_RESP_DATA query_resp;
} CXL_PRIV_DATA;

#pragma pack(0)

#endif
