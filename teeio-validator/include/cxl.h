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

// Table 8-4
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

// Table 8-4
typedef union
{
  struct
  {
    uint16_t id;
  };
  uint16_t raw;
} CXL_DVS_HEADER2;

// Section 8.1.3.1
// DVSEC CXL Capability
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
} CXL_DEV_CAPABILITY;

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
} CXL_DEV_CONTROL;

typedef union
{
  struct
  {
    uint16_t rsvd0 : 14;
    uint16_t viral_status : 1;
    uint16_t rsvd1 : 1;
  };
  uint16_t raw;
} CXL_DEV_STATUS;

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
} CXL_DEV_CONTROL2;

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
} CXL_DEV_STATUS2;

typedef union
{
  struct
  {
    uint16_t config_lock : 15;
    uint16_t rsvd : 1;
  };
  uint16_t raw;
} CXL_DEV_LOCK;

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
} CXL_DEV_CAPABILITY2;

typedef union
{
  struct
  {
    uint32_t memory_size_high;
  };
  uint32_t raw;
} CXL_DEV_RANGE_SIZE_HIGH;

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
} CXL_DEV_RANGE_SIZE_LOW;

typedef union
{
  struct
  {
    uint32_t memory_base_high;
  };
  uint32_t raw;
} CXL_DEV_RANGE_BASE_HIGH;

typedef union
{
  struct
  {
    uint32_t rsvd : 28;
    uint32_t memory_base_low : 4;
  };
  uint32_t raw;
} CXL_DEV_RANGE_BASE_LOW;

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
} CXL_DEV_CAPABILITY3;

typedef struct
{
  CXL_DEV_RANGE_SIZE_HIGH high;
  CXL_DEV_RANGE_SIZE_LOW low;
} CXL_DEV_RANGE_SIZE;

typedef struct
{
  CXL_DEV_RANGE_BASE_HIGH high;
  CXL_DEV_RANGE_BASE_LOW low;
} CXL_DEV_RANGE_BASE;

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
  CXL_DEV_CAPABILITY capability;
  CXL_DEV_CONTROL control;
  CXL_DEV_STATUS status;
  CXL_DEV_CONTROL2 control2;
  CXL_DEV_STATUS2 status2;
  CXL_DEV_LOCK lock;
  CXL_DEV_CAPABILITY2 capability2;

  CXL_DEV_RANGE_SIZE range1_size;
  CXL_DEV_RANGE_BASE range1_base;
  CXL_DEV_RANGE_SIZE range2_size;
  CXL_DEV_RANGE_BASE range2_base;

  CXL_DEV_CAPABILITY3 capability3;
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

// Section 8.2.4.1
typedef union {
  struct {
    uint16_t  cap_id;
    uint8_t   cap_version:4;
    uint8_t   cache_mem_version:4;
    uint8_t   array_size;
  };
  uint32_t raw;
} CXL_CAPABILITY_HEADER;

typedef union {
  struct {
    uint16_t  cap_id;
    uint16_t  cap_version:4;
    uint16_t  pointer:12;
  };
  uint32_t raw;
} CXL_CAPABILITY_XXX_HEADER;

// Section 8.2.4.22.1
// CXL IDE Capability
typedef union {
  struct {
    uint32_t cxl_ide_capable:1;
    uint32_t supported_cxl_ide_modes:16;
    uint32_t supported_algo:5;
    uint32_t ide_stop_capable:1;
    uint32_t lopt_ide_capable:1;
    uint32_t rsvd:8;
  };
  uint32_t raw;
} CXL_IDE_CAPABILITY;

// Section 8.2.4.22.2
// CXL IDE Control
typedef union {
  struct {
    uint32_t pcrc_disable:1;
    uint32_t ide_stop_enable:1;
    uint32_t rsvd:30;
  };
  uint32_t raw;
} CXL_IDE_CONTROL;

// Section 8.2.4.22.3
// CXL IDE Status
typedef union {
  struct {
    uint8_t rx_ide_status:4;
    uint8_t tx_ide_status:4;
    uint8_t rsvd0;
    uint16_t rsvd1;
  };
  uint32_t raw;
} CXL_IDE_STATUS;

// Section 8.2.4.22.4
// CXL IDE Error Status
typedef union {
  struct {
    uint8_t rx_error_status:4;
    uint8_t tx_error_status:4;
    uint8_t unexpected_ide_stop_received:1;
    uint8_t rsvd0:7;
    uint16_t rsvd1;
  };
  uint32_t raw;
} CXL_IDE_ERROR_STATUS;

// Section 8.2.4.22.5
// Key Refresh Time Capability
typedef union {
  struct {
    uint32_t rx_min_key_refresh_time;
  };
  uint32_t raw;
} CXL_KEY_REFRESH_TIME_CAPABILITY;

// Section 8.2.4.22.6
// Truncation Transmit Delay Capability
typedef union {
  struct {
    uint8_t rx_min_truncation_transmit_delay;
    uint8_t rx_min_truncation_transmit_delay2;
    uint16_t rsvd;
  };
  uint32_t raw;
} CXL_TRUNCATION_TRANSMIT_DELAY_CAPABILITY;

// Section 8.2.4.22.7
// Key Refresh Time Control
typedef union {
  struct {
    uint32_t tx_key_refresh_time;
  };
  uint32_t raw;
} CXL_KEY_REFRESH_TIME_CONTROL;

// Section 8.2.4.22.8
// Truncation Transmit Delay Capability
typedef union {
  struct {
    uint8_t tx_truncation_transmit_delay;
    uint8_t rsvd0;
    uint16_t rsvd1;
  };
  uint32_t raw;
} CXL_TRUNCATION_TRANSMIT_DELAY_CONTROL;

// Section 8.2.4.22.9
// Key Refresh Time Capability2
typedef union {
  struct {
    uint32_t rx_min_key_refresh_time2;
  };
  uint32_t raw;
} CXL_KEY_REFRESH_TIME_CAPABILITY2;

// Section 8.2.4.22
// CXL IDE Capability Structure
typedef struct {
  CXL_IDE_CAPABILITY cap;
  CXL_IDE_CONTROL control;
  CXL_IDE_STATUS status;
  CXL_IDE_ERROR_STATUS error_status;
  CXL_KEY_REFRESH_TIME_CAPABILITY key_refresh_time_capability;
  CXL_TRUNCATION_TRANSMIT_DELAY_CAPABILITY truncation_transmit_delay_capability;
  CXL_KEY_REFRESH_TIME_CONTROL key_refresh_time_control;
  CXL_TRUNCATION_TRANSMIT_DELAY_CONTROL truncation_transmit_delay_control;
  CXL_KEY_REFRESH_TIME_CAPABILITY2 key_refresh_time_capability2;
} CXL_IDE_CAPABILITY_STRUCT;

// Section 11.4.4 Discovery Messages
// Table 11-7 Byte offset 13h in CXL_QUERY_RESP
typedef union {
  struct {
    uint8_t cxl_ide_cap_version: 4;
    uint8_t iv_generation_capable: 1;
    uint8_t ide_key_generation_capable: 1;
    uint8_t k_set_stop_capable: 1;
    uint8_t rsvd: 1;
  };
  uint8_t raw;
} CXL_QUERY_RESP_CAPS;

#pragma pack(0)

#define DVSEC_VENDOR_ID_CXL 0x1E98

typedef enum {
  CXL_CAPABILITY_ID_NULL = 0,
  CXL_CAPABILITY_ID_CAP,
  CXL_CAPABILITY_ID_RAS_CAP,
  CXL_CAPABILITY_ID_SECURITY_CAP,
  CXL_CAPABILITY_ID_LINK_CAP,
  CXL_CAPABILITY_ID_HDM_DECODER_CAP,
  CXL_CAPABILITY_ID_EXTENDED_SECURITY_CAP,
  CXL_CAPABILITY_ID_IDE_CAP,
  CXL_CAPABILITY_ID_SNOOP_FILTER_CAP,
  CXL_CAPABILITY_ID_TIMEOUT_ISOLATION_CAP,
  CXL_CAPABILITY_ID_CACHEMEM_EXTENDED_REG_CAP,
  CXL_CAPABILITY_ID_BI_ROUTE_TABLE_CAP,
  CXL_CAPABILITY_ID_BI_DECODER_CAP,
  CXL_CAPABILITY_ID_CACHE_ID_ROUTE_TABLE_CAP,
  CXL_CAPABILITY_ID_CACHE_ID_DECODER_TABLE_CAP,
  CXL_CAPABILITY_ID_EXTENDED_HDM_DECODE_CAP,
  CXL_CAPABILITY_ID_EXTENDED_METADATA_CAP,
  CXL_CAPABILITY_ID_NUM
} CXL_CAPABILITY_ID;

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

// Section 8.1.9.1 Register Block Identifier
typedef enum {
  CXL_DVSEC_REG_BLOCK_ID_EMPTY = 0x00,
  CXL_DVSEC_REG_BLOCK_ID_COMPONENT_REG = 0x01,
  CXL_DVSEC_REG_BLOCK_ID_BAR_VIRT_ACL_REG = 0x02,
  CXL_DVSEC_REG_BLOCK_ID_CXL_DVE_REG = 0x03,
  CXL_DVSEC_REG_BLOCK_ID_CPMU_REG = 0x04,
  CXL_DVSEC_REG_BLOCK_ID_DVS_REG = 0xff
} CXL_DVSEC_REG_BLOCK_ID;

// Table 8-21
#define CXL_IO_REG_BLOCK_SIZE 0x1000
#define CXL_CACHEMEM_REG_BLOCK_SIZE 0x1000
#define CXL_IMPLEMENTATION_REG_BLOCK_SIZE 0xC000
#define CXL_ARG_MUX_REG_BLOCK_SIZE  0x1000
#define CXL_RSVR_BLOCK_SIZE 0x1C00

// Section 8.2.4.22.1
#define CXL_IDE_MODE_SKID_MASK  0x2 // Bit[1]
#define CXL_IDE_MODE_CONTAINMENT_MASK 0x4 // Bit[2]

#endif