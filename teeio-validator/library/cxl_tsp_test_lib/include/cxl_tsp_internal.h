/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __CXL_TSP_INTERNAL_H__
#define __CXL_TSP_INTERNAL_H__

#include "hal/base.h"
#include "hal/library/memlib.h"
#include "library/spdm_requester_lib.h"
#include "library/spdm_transport_pcidoe_lib.h"
#include "library/cxl_tsp_requester_lib.h"

#pragma pack(1)
typedef struct {
    cxl_tsp_header_t header;
    uint16_t reserved;
    uint8_t version_number_entry_count;
    cxl_tsp_version_number_t version_number_entry[1];
} teeio_cxl_tsp_get_target_tsp_version_rsp_mine_t;

typedef struct {
    cxl_tsp_header_t header;
    uint16_t reserved;
    uint16_t portion_length;
    uint16_t remainder_length;
    uint8_t report[LIBCXLTSP_CONFIGURATION_REPORT_PORTION_LEN];
} teeio_cxl_tsp_get_target_configuration_report_rsp_mine_t;

#pragma pack()


#endif