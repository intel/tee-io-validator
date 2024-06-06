/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __PCIE_H__
#define __PCIE_H__

#include <stdint.h>
#pragma pack(1)
// Table 7-323 IDE Extended Capability Header
typedef union
{
    struct
    {
        uint16_t id;
        uint16_t version:4;
        uint16_t next_cap_offset:12;
    };
    uint32_t raw;
} PCIE_CAP_ID;

// 7.9.26.2 IDE Capability Register
typedef union
{
    struct
    {
        uint8_t lnk_ide_supported :1;
        uint8_t sel_ide_supported :1;
        uint8_t ft_supported : 1;
        uint8_t rsvd0 :1;
        uint8_t aggr_supported:1;
        uint8_t pcrc_supported:1;
        uint8_t ide_km_protocol_supported:1;
        uint8_t sel_ide_cfg_req_supported:1;
        uint8_t supported_algo:5;
        uint8_t num_lnk_ide:3;
        uint8_t num_sel_ide;
        uint8_t rsvd2;
    };
    uint32_t raw;
} PCIE_IDE_CAP;

// 7.9.26.3 IDE Control Register
typedef union
{
    struct
    {
        uint32_t rsvd0 :2;
        uint32_t ft_supported :1;
    };
    uint32_t raw;
} PCIE_IDE_CTRL;

// 7.9.26.4.1 Link IDE Stream Control Register
typedef union
{
    struct
    {
        uint8_t enabled:1;
        uint8_t rsvd0:1;
        uint8_t tx_aggr_mode_npr : 2;
        uint8_t tx_aggr_mode_pr : 2;
        uint8_t tx_aggr_mode_cpl : 2;
        uint16_t pcrc_en : 1;
        uint16_t rsvd1:5;
        uint16_t selected_algo : 5;
        uint16_t tc :3;
        uint16_t rsvd2:2;
        uint8_t stream_id;
    };
    uint32_t raw;
} PCIE_LNK_IDE_STREAM_CTRL;

// 7.9.26.4.2 Link IDE Stream Status Register
typedef union
{
    struct
    {
        uint32_t state :4;
        uint32_t rsvd  :27;
        uint32_t recv_intg_check_fail_msg:1;
    };
    uint32_t raw;
} PCIE_LINK_IDE_STREAM_STATUS;

// 7.9.26.4 Link IDE Register Block
typedef struct
{
    PCIE_LNK_IDE_STREAM_CTRL control;
    PCIE_LINK_IDE_STREAM_STATUS status;
} PCIE_LNK_IDE_STREAM_REG_BLOCK;

// 7.9.26.5.1 Selective IDE Stream Capability Register
typedef union
{
    struct
    {
        uint32_t num_addr_assoc_reg_blocks : 4;
        uint32_t rsvd : 28;
    };
    uint32_t raw;
} PCIE_SEL_IDE_STREAM_CAP;

// 7.9.26.5.2 Selective IDE Stream Control Register
typedef union
{
  struct
  {

    uint8_t enabled:1;
    uint8_t reserved0:1;
    uint8_t tx_aggr_mode_npr:2;
    uint8_t tx_aggr_mode_pr:2;
    uint8_t tx_aggr_mode_cpl:2;
    uint16_t pcrc_en:1;
    uint16_t cfg_sel_ide:1;
    uint16_t reserved1:4;
    uint16_t algorithm:5;
    uint16_t tc:3;
    uint16_t default_stream:1;
    uint16_t reserved2:1;
    uint8_t stream_id;
  };
  uint32_t raw;
} PCIE_SEL_IDE_STREAM_CTRL;

// 7.9.26.5.3 Selective IDE Stream Status Register
typedef union
{
    struct
    {
        uint32_t state :4;
        uint32_t rsvd  :27;
        uint32_t recv_intg_check_fail_msg:1;
    };
    uint32_t raw;
} PCIE_SEL_IDE_STREAM_STATUS;

// 7.9.26.5.4.1 IDE RID Association Register 1
typedef union
{
  struct
  {
    uint8_t rsvd0;
    uint16_t rid_limit;
    uint8_t rsvd1;
  };
  uint32_t raw;
} PCIE_SEL_IDE_RID_ASSOC_1;

// 7.9.26.5.4.2 IDE RID Association Register 2
typedef union
{
  struct
  {
    uint8_t valid:1;
    uint8_t rsvd0:7;
    uint16_t rid_base;
    uint8_t rsvd1;
  };
  uint32_t raw;
} PCIE_SEL_IDE_RID_ASSOC_2;

// 7.9.26.5.4 Selective IDE RID Association Register Block
typedef struct
{
    PCIE_SEL_IDE_RID_ASSOC_1 rid_assoc1;
    PCIE_SEL_IDE_RID_ASSOC_2 rid_assoc2;
} PCIE_SEL_IDE_RID_ASSOC_REG_BLOCK;

// 7.9.26.5.5.1 IDE Address Association Register 1
typedef union
{
    struct
    {
        uint32_t valid :1;
        uint32_t rsvd :7;
        uint32_t mem_base_lower :12;
        uint32_t mem_limit_lower :12;
    };
    uint32_t raw;
} PCIE_SEL_IDE_ADDR_ASSOC_1;

// 7.9.26.5.5.2 IDE Address Association Register 2
typedef union
{
    uint32_t mem_limit_upper;
    uint32_t raw;
} PCIE_SEL_IDE_ADDR_ASSOC_2;

// 7.9.26.5.5.3 IDE Address Association Register 3
typedef union
{
    uint32_t mem_base_upper;
    uint32_t raw;
} PCIE_SEL_IDE_ADDR_ASSOC_3;

// 7.9.26.5.5 Selective IDE Address Association Register Block
typedef struct
{
    PCIE_SEL_IDE_ADDR_ASSOC_1 addr_assoc1;
    PCIE_SEL_IDE_ADDR_ASSOC_2 addr_assoc2;
    PCIE_SEL_IDE_ADDR_ASSOC_3 addr_assoc3;
} PCIE_SEL_IDE_ADDR_ASSOC_REG_BLOCK;

// 7.9.26.5 Selective IDE Stream Register Block
typedef struct
{
    PCIE_SEL_IDE_STREAM_CAP capability;
    PCIE_SEL_IDE_STREAM_CTRL control; // Read only
    PCIE_SEL_IDE_STREAM_STATUS status; // Read only
    PCIE_SEL_IDE_RID_ASSOC_REG_BLOCK rid_assoc_block;
} PCIE_SEL_IDE_STREAM_REG_BLOCK;

// 7.9.26 IDE Extended Capability
typedef struct
{
    PCIE_CAP_ID ide_ecap;
    PCIE_IDE_CAP ide_cap;
    PCIE_IDE_CTRL ide_ctrl;
    // PCIE_LNK_IDE_STREAM_REG_BLOCK lnk_ide_stream_block; // number of elements is dynamic
    // PCIE_SEL_IDE_STREAM_REG_BLOCK sel_ide_stream_block; // number of elements is dynamic
} PCIE_IDE_ECAP;

#pragma pack(0)

// 6.33.3 IDE Key Management 
#define PCIE_IDE_KEY_SIZE  (32)
#define PCIE_IDE_KEY_SIZE_IN_DWORDS (PCIE_IDE_KEY_SIZE / 4)
#define PCIE_IDE_IV_SIZE   (8)
#define PCIE_IDE_IV_SIZE_IN_DWORDS  (PCIE_IDE_IV_SIZE / 4)

#define PCIE_IDE_IV_INIT_VALUE      (1)

// 6.33.3 IDE Key Management 
enum PCIE_IDE_STREAM_DIRECTION_ENUM {
    PCIE_IDE_STREAM_RX = 0,
    PCIE_IDE_STREAM_TX = 1,
    PCIE_IDE_STREAM_DIRECTION_NUM
};

// 6.33.3 IDE Key Management 
enum PCIE_IDE_STREAM_KEY_SET_SEL_ENUM {
    PCIE_IDE_STREAM_KS0 = 0,
    PCIE_IDE_STREAM_KS1 = 1,
    PCIE_IDE_STREAM_KS_NUM
};

// 6.33.5 IDE TLP Sub-Streams
enum PCIE_IDE_STREAM_KEY_SUB_STREAM_ENUM {
    PCIE_IDE_SUB_STREAM_PR  = 0,
    PCIE_IDE_SUB_STREAM_NPR = 1,
    PCIE_IDE_SUB_STREAM_CPL = 2,
    PCIE_IDE_SUB_STREAM_NUM
};

#define IDE_STREAM_STATUS_SECURE 2
#define IDE_STREAM_STATUS_INSECURE 0

#define PCI_DOE_EXT_CAPABILITY_ID 0x002E
#define PCI_IDE_EXT_CAPABILITY_ID 0x0030
#define PCI_AER_EXT_CAPABILITY_ID 0x0001

#endif