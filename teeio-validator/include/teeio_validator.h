/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __TEEIO_VALIDATOR_H__
#define __TEEIO_VALIDATOR_H__

#include "hal/base.h"
#include "hal/library/memlib.h"
#include "library/spdm_requester_lib.h"
#include "library/spdm_transport_pcidoe_lib.h"
#include "library/pci_doe_requester_lib.h"
#include "library/pci_ide_km_requester_lib.h"
#include "library/pci_tdisp_requester_lib.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"

#include "teeio_debug.h"
#include "helperlib.h"

#define TEEIO_VALIDATOR_NAME "teeio_validator"
#define TEEIO_VALIDATOR_VERSION "0.2.0"

#define LOGFILE "./teeio_log"
#define PCAPFILE "./teeio_pcap"

#endif
