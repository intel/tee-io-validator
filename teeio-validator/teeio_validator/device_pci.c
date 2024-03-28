/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include "teeio_validator.h"
#include "hal/library/platform_lib.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include "pcie.h"

uint32_t g_doe_extended_offset = 0;
uint32_t g_ide_extended_offset = 0;
uint32_t g_aer_extended_offset = 0;

#define AER_UNCERRSTS_OFF 0x4
#define AER_UNCERRMSK_OFF 0x8
#define AER_UNCERRSEV_OFF 0xC
#define AER_CORERRSTS_OFF 0x10
#define AER_CORERRMSK_OFF 0x14
#define AER_AERHDRLOG0_OFF 0x1C
#define AER_AERHDRLOG1_OFF 0x20
#define AER_AERHDRLOG2_OFF 0x24
#define AER_AERHDRLOG3_OFF 0x28
#define AER_AERPREFIXLOG0_OFF 0x38
#define AER_AERPREFIXLOG1_OFF 0x3C
#define AER_AERPREFIXLOG2_OFF 0x40
#define AER_AERPREFIXLOG3_OFF 0x44


int m_dev_fp = 0;
FILE* m_logfile = NULL;

void dump_ecap(
    int fd,
    uint8_t ide_id,
    uint32_t ide_ecap_offset,
    TEST_IDE_TYPE ide_type
);

bool log_file_init(const char* filepath){
    m_logfile = fopen(filepath, "w");
    if(!m_logfile){
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "failed to open %s to write!\n", LOGFILE));
        return false;
    }
    return true;
}

void log_file_close(){
    if(!m_logfile){
        fclose(m_logfile);
    }
}

// more info please check file - new_cambria_core_regs_RWF_FM85.doc.xml
void check_pcie_advance_error()
{
    return;
}

void dump_dev_registers(int cfg_space_fd, uint8_t ide_id, uint32_t ecap_offset, TEST_IDE_TYPE ide_type)
{
    dump_ecap(cfg_space_fd, ide_id, ecap_offset, ide_type);
}