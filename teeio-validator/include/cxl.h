/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __CXL_H__
#define __CXL_H__

#include <stdint.h>
#include "pcie.h"
//
// Compute Express Link (CXL) Spec
// August 2023, Revision 3.1
//

#pragma pack(1)

typedef union
{
  struct
  {
    uint16_t vendor_id;
    uint16_t revision : 4;
    uint16_t length : 12;
  };
  uint32_t raw;
} CXL_DVS_HEADER1;

typedef union
{
  struct
  {
    uint16_t id;
  };
  uint16_t raw;
} CXL_DVS_HEADER2;

typedef union
{
  struct
  {
    uint8_t cache_capable : 1;
    uint8_t io_capable : 1;
    uint8_t mem_capable : 1;
    uint8_t mem_hwinit_mode : 1;
    uint8_t hdm_count : 2;
    uint8_t cache_writeback_and_invalidate_capable : 1;
    uint8_t cxl_reset_capable : 1;

    uint8_t cxl_reset_timeout : 3;
    uint8_t cxl_mem_clr_capable : 1;
    uint8_t tsp_capable : 1;
    uint8_t multiple_logical_device : 1;
    uint8_t viral_capable : 1;
    uint8_t pm_init_completion_reporting_capable : 1;
  };
  uint16_t raw;
} CXL_CAPABILITY;

typedef union
{
  struct
  {
    uint8_t cache_enable : 1;
    uint8_t io_enable : 1;
    uint8_t mem_enable : 1;
    uint8_t cache_sf_coverage : 5;

    uint8_t cache_sf_granularity : 3;
    uint8_t cache_clean_eviction : 1;
    uint8_t direct_p2p_mem_enable : 1;
    uint8_t rsvd0 : 1;
    uint8_t viral_enable : 1;
    uint8_t rsvd1 : 1;
  };
  uint16_t raw;
} CXL_CONTROL;

typedef union
{
  struct
  {
    uint16_t rsvd0 : 14;
    uint16_t viral_status : 1;
    uint16_t rsvd1 : 1;
  };
  uint16_t raw;
} CXL_STATUS;

typedef union
{
  struct
  {
    uint16_t disable_caching : 1;
    uint16_t initiate_cache_write_back_and_invalidation : 1;
    uint16_t initiate_cxl_reset : 1;
    uint16_t cxl_reset_mem_clr_enable : 1;
    uint16_t desired_volatile_hdm_state_after_hot_reset : 1;
    uint16_t modified_completion_enable : 1;
    uint16_t rsvd : 10;
  };
  uint16_t raw;
} CXL_CONTROL2;

typedef union
{
  struct
  {
    uint16_t cache_invalid : 1;
    uint16_t cxl_reset_complete : 1;
    uint16_t cxl_reset_error : 1;
    uint16_t volatile_hdm_preservation_error : 1;
    uint16_t rsvd : 11;
    uint16_t power_management_initialization_complete : 1;
  };
  uint16_t raw;
} CXL_STATUS2;

typedef union
{
  struct
  {
    uint16_t config_lock : 15;
    uint16_t rsvd : 1;
  };
  uint16_t raw;
} CXL_LOCK;

typedef union
{
  struct
  {
    uint8_t cache_size_unit : 4;
    uint8_t fallback_capability : 2;
    uint8_t modified_completion_capable : 1;
    uint8_t no_clean_writeback : 1;

    uint8_t cache_size;
  };
  uint16_t raw;
} CXL_CAPABILITY2;

typedef union
{
  struct
  {
    uint32_t memory_size_high;
  };
  uint32_t raw;
} CXL_RANGE_SIZE_HIGH;

typedef union
{
  struct
  {
    uint8_t memory_info_valid : 1;
    uint8_t memory_active : 1;
    uint8_t media_type : 3;
    uint8_t memory_class : 3;

    uint8_t desired_interleave : 5;
    uint8_t memory_active_timeout : 3;

    uint16_t memory_active_degraded : 1;
    uint16_t rsvd : 11;
    uint16_t memory_size_low : 4;
  };
  uint32_t raw;
} CXL_RANGE_SIZE_LOW;

typedef union
{
  struct
  {
    uint32_t memory_base_high;
  };
  uint32_t raw;
} CXL_RANGE_BASE_HIGH;

typedef union
{
  struct
  {
    uint32_t rsvd : 28;
    uint32_t memory_base_low : 4;
  };
  uint32_t raw;
} CXL_RANGE_BASE_LOW;

typedef union
{
  struct
  {
    uint16_t default_volatile_hdm_state_after_cold_reset : 1;
    uint16_t default_volatile_hdm_state_after_warm_reset : 1;
    uint16_t default_volatile_hdm_state_after_hot_reset : 1;
    uint16_t volatile_hdm_state_after_hot_reset : 1;
    uint16_t direct_p2p_mem_capable : 1;
    uint16_t rsvd : 11;
  };
  uint16_t raw;
} CXL_CAPABILITY3;

typedef struct
{
  CXL_RANGE_SIZE_HIGH high;
  CXL_RANGE_SIZE_LOW low;
} CXL_RANGE_SIZE;

typedef struct
{
  CXL_RANGE_BASE_HIGH high;
  CXL_RANGE_BASE_LOW low;
} CXL_RANGE_BASE;

typedef struct
{
  PCIE_CAP_ID ide_ecap;
  CXL_DVS_HEADER1 header1;
  CXL_DVS_HEADER2 header2;
} CXL_DVSEC_COMMON_HEADER;

// Section 8.1.3
// PCIe DVSEC for CXL Device
// D2 mandatory
typedef struct
{
  CXL_DVSEC_COMMON_HEADER common_header;
  CXL_CAPABILITY capability;
  CXL_CONTROL control;
  CXL_STATUS status;
  CXL_CONTROL2 control2;
  CXL_STATUS2 status2;
  CXL_LOCK lock;
  CXL_CAPABILITY2 capability2;

  CXL_RANGE_SIZE range1_size;
  CXL_RANGE_BASE range1_base;
  CXL_RANGE_SIZE range2_size;
  CXL_RANGE_BASE range2_base;

  CXL_CAPABILITY3 capability3;
  uint16_t rsvd;
} CXL_DVSEC_FOR_DEVICE;

// Section 8.1.5
// CXL Extensions DVSEC for Ports
// R/DSP/USP mandatory
typedef struct {
  CXL_DVSEC_COMMON_HEADER common_header;
  // other fields
} CXL_DVSEC_FOR_PORTS;

// Section 8.1.6
// GPF DVSEC for CXL Ports
// R/DSP mandatory
typedef struct {
  CXL_DVSEC_COMMON_HEADER common_header;
  // other fields
} CXL_GPF_DVSEC_FOR_PORTS;

// Section 8.1.7
// GPF DVSEC for CXL Devices
// D2 mandatory
typedef struct {
  CXL_DVSEC_COMMON_HEADER common_header;
  // other fields
} CXL_GPF_DVSEC_FOR_DEVICES;

// Section 8.1.8
// PCIe DVSEC for Flex Bus Port
// D2/R mandatory
typedef struct {
  CXL_DVSEC_COMMON_HEADER common_header;
  // other fields
} CXL_DVSEC_FOR_FLEX_BUS_PORT;

// Section 8.1.9
// Register Locator DVSEC
// D2/R mandatory
typedef union {
  struct {
    uint8_t register_bir:3;
    uint8_t rsvd:5;
    uint8_t register_block_id;
    uint16_t register_block_offset_low;
  };
  uint32_t raw;
} CXL_REGISTER_OFFSET_LOW;

typedef struct {
  uint32_t register_block_offset_high;
} CXL_REGISTER_OFFSET_HIGH;

typedef struct {
  CXL_REGISTER_OFFSET_LOW low;
  CXL_REGISTER_OFFSET_HIGH high;
} CXL_REGISTER_BLOCK;

typedef struct {
  CXL_DVSEC_COMMON_HEADER common_header;
  uint16_t rsvd;
  // variable count of reg_blocks
  // CXL_REGISTER_BLOCK reg_block;
} CXL_DVSEC_REGISTER_LOCATOR;

#pragma pack(0)

#define DVSEC_VENDOR_ID_CXL 0x1E98

typedef enum
{
  CXL_IDE_STREAM_DIRECTION_RX = 0,
  CXL_IDE_STREAM_DIRECTION_TX,
  CXL_IDE_STREAM_DIRECTION_NUM
} CXL_IDE_STREAM_DIRECTION;

// Table 8-2
typedef enum {
  CXL_DVSEC_ID_PCIE_DVSEC_FOR_CXL_DEVICES = 0,
  CXL_DVSEC_ID_NON_CXL_FUNCTION_MAP_DVSEC = 0x2,
  CXL_DVSEC_ID_CXL_EXTENSIONS_DVSEC_FOR_PORTS = 0x3,
  CXL_DVSEC_ID_GPF_DVSEC_FOR_CXL_PORTS = 0x4,
  CXL_DVSEC_ID_GPF_DVSEC_FOR_CXL_DEVICES = 0x5,
  CXL_DVSEC_ID_PCIE_DVSEC_FOR_FLEX_BUS_PORT = 0x7,
  CXL_DVSEC_ID_REGISTER_LOCATOR_DVSEC = 0x8,
  CXL_DVSEC_ID_MLD_DVSEC = 0x09,
  CXL_DVSEC_ID_PCIE_DVSEC_FOR_TEST_CAP = 0xA,
  CXL_DVSEC_IN_INVALID = 0xFF
} CXL_DVSEC_ID;

#define CXL_DVS_HEADER1_OFFSET (sizeof(PCIE_CAP_ID))
#define CXL_DVS_HEADER2_OFFSET (CXL_DVS_HEADER1_OFFSET + sizeof(CXL_DVS_HEADER1))
#define CXL_CAPABILITY_OFFSET (CXL_DVS_HEADER2_OFFSET + sizeof(CXL_DVS_HEADER2))
#define CXL_CONTROL_OFFSET (CXL_CAPABILITY_OFFSET + sizeof(CXL_CAPABILITY))
#define CXL_STATUS_OFFSET (CXL_CONTROL_OFFSET + sizeof(CXL_CONTROL))
#define CXL_CONTROL2_OFFSET (CXL_STATUS_OFFSET + sizeof(CXL_STATUS))
#define CXL_STATUS2_OFFSET (CXL_CONTROL2_OFFSET + sizeof(CXL_CONTROL2))
#define CXL_LOCK_OFFSET (CXL_STATUS2_OFFSET + sizeof(CXL_STATUS2))
#define CXL_CAPABILITY2_OFFSET (CXL_LOCK_OFFSET + sizeof(CXL_LOCK))

#define CXL_RANGE1_SIZE_HIGH_OFFSET (CXL_CAPABILITY2_OFFSET + sizeof(CXL_CAPABILITY2))
#define CXL_RANGE1_SIZE_LOW_OFFSET (CXL_RANGE1_SIZE_HIGH_OFFSET + sizeof(CXL_RANGE_SIZE_HIGH))
#define CXL_RANGE1_BASE_HIGH_OFFSET (CXL_RANGE1_SIZE_LOW_OFFSET + sizeof(CXL_RANGE_SIZE_LOW))
#define CXL_RANGE1_BASE_LOW_OFFSET (CXL_RANGE1_BASE_HIGH_OFFSET + sizeof(CXL_RANGE_BASE_HIGH))

#define CXL_RANGE2_SIZE_HIGH_OFFSET (CXL_RANGE1_BASE_LOW_OFFSET + sizeof(CXL_RANGE_BASE_LOW))
#define CXL_RANGE2_SIZE_LOW_OFFSET (CXL_RANGE2_SIZE_HIGH_OFFSET + sizeof(CXL_RANGE_SIZE_HIGH))
#define CXL_RANGE2_BASE_HIGH_OFFSET (CXL_RANGE2_SIZE_LOW_OFFSET + sizeof(CXL_RANGE_SIZE_LOW))
#define CXL_RANGE2_BASE_LOW_OFFSET (CXL_RANGE2_BASE_HIGH_OFFSET + sizeof(CXL_RANGE_BASE_HIGH))

#define CXL_CAPABILITY3_OFFSET (CXL_RANGE2_BASE_LOW_OFFSET + sizeof(CXL_RANGE_BASE_LOW))

#endif