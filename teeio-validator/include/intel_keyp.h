/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __INTEL_KEYP_H__
#define __INTEL_KEYP_H__

#include <stdint.h>

#pragma pack(1)

// Below defitions are based on Root Complex IDE Programming Guide

// Table 2-2. Protocol Type
typedef enum {
  INTEL_KEYP_PROTOCOL_TYPE_PCIE_CXLIO = 1,
  INTEL_KEYP_PROTOCOL_TYPE_CXL_MEMCACHE = 2
} INTEL_KEYP_PROTOCOL_TYPE;

#define CXL_LINK_ENC_KEYS_SLOT_NUM  4

typedef enum {
  INTEL_CXL_IDE_MODE_RSVD = 0,
  INTEL_CXL_IDE_MODE_CONTAINMENT,
  INTEL_CXL_IDE_MODE_SKID,
  INTEL_CXL_IDE_MODE_MAX
} INTEL_CXL_IDE_MODE;

//
// PCIE & CXL.IO IDE
//

// Table 2-1. Key Programming Table Reporting Structure
typedef struct {
  uint8_t   signature[4];
  uint32_t  length;
  uint8_t   revision;
  uint8_t   check_sum;
  uint8_t   oem_id[6];
  uint8_t   oem_table_id[8];
  uint32_t  oem_revision;
  uint32_t  creator_id;
  uint32_t  creator_revision;
  uint32_t  rsvd;
  // a list of INTEL_KEYP_KEY_CONFIGURATION_UNIT
} INTEL_KEYP_ACPI;

// Table 2-2. Key Configuration Unit Structure
typedef struct {
  uint8_t   Type;
  uint8_t   Rsvd;
  uint16_t  Length;
  uint8_t   ProtocolType;
  uint8_t   Version;
  uint8_t   RootPortCount;
  uint8_t   Flags;
  uint64_t  RegisterBaseAddr;
  // root ports under the purview of this key_configuration_unit_t
  // chain of root_port_information_t
} INTEL_KEYP_KEY_CONFIGURATION_UNIT;

// Table 2-3. Root Port Information Structure
typedef struct {
  uint16_t  SegmentNumber;
  uint8_t   Bus;
  struct {
    uint8_t Function: 3;    // 0:2
    uint8_t Device : 5;     // 3:7
  } Bits;
} INTEL_KEYP_ROOT_PORT_INFORMATION;

// Table 2-4. PCIe Stream Capability Structure
typedef union
{
    struct
    {
        uint32_t num_stream_supported : 8; // 0:7
        uint32_t rsvd : 2;
        uint32_t num_tx_key_slots : 10; // 10:19
        uint32_t num_rx_key_slots : 10; // 20:29
        uint32_t rsvd2 : 2; // 30:31
    };
    uint32_t raw;
} INTEL_KEYP_PCIE_STREAM_CAP;

// Table 2-5. Stream Control
typedef union
{
    struct
    {
        uint32_t en : 1;
        uint32_t rsvd: 23;
    };
    struct
    {
        uint8_t :8;
        uint16_t :16;
        uint8_t stream_id;
    };
    uint32_t raw;
} INTEL_KEYP_STREAM_CONTROL;

// Table 2-6. Tx Control
// Table 2-8. Rx Control
typedef union
{
    struct
    {
        uint32_t key_set_select : 2; // 0:1
        uint32_t rsvd_1 : 6; // 2:7
        uint32_t prime_key_set_0 : 1; // 8
        uint32_t rsvd_2 : 7; // 9:15
        uint32_t prime_key_set_1 : 1; // 16
    } stream_tx_control;
    struct
    {
        uint32_t rsvd_1 : 8; // 0:7
        uint32_t prime_key_set_0 : 1; // 8

        uint32_t rsvd_2 : 7; // 9:15
        uint32_t prime_key_set_1 : 1; // 16
    } stream_rx_control;
    struct
    {
        uint8_t rsvd; // 0:7
        uint8_t prime_key_set_0 : 1; // 8
        uint8_t rsvd_2 : 7; // 9:15
        uint8_t prime_key_set_1 : 1; // 16
    } common;
    uint32_t raw;
} INTEL_KEYP_STREAM_TXRX_CONTROL;

// Table 2-7. Tx Status
// Table 2-9. Rx Status
typedef union
{
    struct
    {
        uint32_t key_set_status : 2;
        uint32_t rsvd : 6;
        uint32_t ready_key_set_0 : 1;
        uint32_t ready_key_set_1 : 1;
    } stream_tx_status;
    struct
    {
        uint32_t last_rcvd_set_pr : 2;
        uint32_t last_rcvd_set_npr : 2;
        uint32_t last_rcvd_set_cpl : 2;
        uint32_t rsvd : 2;
        uint32_t ready_key_set_0 : 1;
        uint32_t ready_key_set_1 : 1;
    } stream_rx_status;
    struct
    {
        uint32_t rsvd : 8;
        uint32_t ready_key_set_0 : 1;
        uint32_t ready_key_set_1 : 1;
    } common;
    uint32_t raw;
} INTEL_KEYP_STREAM_TXRX_STATUS;

// Table 2-10. Tx Key Set 0 Indices
// Table 2-11. Tx Key Set 1 Indices
typedef union
{
    struct
    {
        uint32_t pr:10;
        uint32_t npr:10;
        uint32_t cpl:10;
        uint32_t rsvd:2;
    };
    uint32_t raw;
} INTEL_KEYP_STREAM_KEYSET_SLOT_ID;

// Figure 2-2. Per-Stream Configuration Register Block
typedef struct
{
    INTEL_KEYP_STREAM_CONTROL control;
    INTEL_KEYP_STREAM_TXRX_CONTROL tx_ctrl;
    INTEL_KEYP_STREAM_TXRX_STATUS tx_status;
    INTEL_KEYP_STREAM_TXRX_CONTROL rx_ctrl;
    INTEL_KEYP_STREAM_TXRX_STATUS rx_status;
    INTEL_KEYP_STREAM_KEYSET_SLOT_ID tx_key_set_0;
    INTEL_KEYP_STREAM_KEYSET_SLOT_ID tx_key_set_1;
    INTEL_KEYP_STREAM_KEYSET_SLOT_ID rx_key_set_0;
    INTEL_KEYP_STREAM_KEYSET_SLOT_ID rx_key_set_1;
} INTEL_KEYP_STREAM_CONFIG_REG_BLOCK;

// Figure 2-1. Key Configuration Unit Register Block
typedef struct
{
    INTEL_KEYP_PCIE_STREAM_CAP capabilities;
    // Variable size depends on capabilities.stream_cap.num_stream_supported + 1
    INTEL_KEYP_STREAM_CONFIG_REG_BLOCK stream_config_reg_block;
} INTEL_KEYP_ROOT_COMPLEX_KCBAR;

// Figure 2-3. Key Slot
typedef union
{
    uint8_t bytes[PCIE_IDE_KEY_SIZE];
    uint32_t dwords[PCIE_IDE_KEY_SIZE_IN_DWORDS];
} INTEL_KEYP_KEY_SLOT;

// Figure 2-4. IV Value Slot
typedef union
{
    uint8_t bytes[PCIE_IDE_IV_SIZE];
    uint32_t dwords[PCIE_IDE_IV_SIZE_IN_DWORDS];
} INTEL_KEYP_IV_SLOT;

//
// CXL.memcache IDE
//

typedef union {
  struct {
    uint32_t  link_enc_enable:1;
    uint32_t  mode:5;
    uint32_t  algorithm:6;
    uint32_t  rsvd:20;
  };
  uint32_t raw;
} INTEL_KEYP_CXL_LINK_ENC_GLOBAL_CONFIG;

typedef union {
  struct {
    uint32_t  start_trigger:1;
    uint32_t  rxkey_valid:1;
    uint32_t  txkey_valid:1;
    uint32_t  txtransto_insecure_state:1;
    uint32_t  rxtransto_insecure_state:1;
    uint32_t  rsvd:27;
  };
  uint32_t  raw;
} INTEL_KEYP_CXL_LINK_ENC_CONTROL;

typedef struct {
  uint64_t  link_enc_key;
} INTEL_KEYP_CXL_TXRX_ENCRYPTION_KEY;

typedef struct {
  uint64_t  iv;
} INTEL_KEYP_CXL_TXRX_IV;

typedef struct {
  INTEL_KEYP_CXL_TXRX_ENCRYPTION_KEY  keys[CXL_LINK_ENC_KEYS_SLOT_NUM];
} INTEL_KEYP_CXL_TXRX_ENC_KEYS_BLOCK;

// Figure 2-5
typedef struct {
  INTEL_KEYP_CXL_LINK_ENC_GLOBAL_CONFIG link_enc_global_config;
  uint32_t                              rsvd0;
  INTEL_KEYP_CXL_LINK_ENC_CONTROL       link_enc_control;
  uint32_t                              rsvd1;
  INTEL_KEYP_CXL_TXRX_ENC_KEYS_BLOCK    tx_enc_keys;
  INTEL_KEYP_CXL_TXRX_IV                tx_iv;
  INTEL_KEYP_CXL_TXRX_ENC_KEYS_BLOCK    rx_enc_keys;
  INTEL_KEYP_CXL_TXRX_IV                rx_iv;
} INTEL_KEYP_CXL_ROOT_COMPLEX_KCBAR;

#pragma pack()

#endif