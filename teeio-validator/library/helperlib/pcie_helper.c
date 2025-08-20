/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "pcie.h"
#include "teeio_debug.h"
#include "helperlib.h"
#include "helper_internal.h"

extern bool g_pci_log;

#define MAX_SUPPORT_DEVICE_NUM  32
IDE_TEST_DEVICES_INFO devices[MAX_SUPPORT_DEVICE_NUM] = {0};

/**
 * the correct bdf string looks like: 0001:2a:00.0
*/
bool is_valid_bdf(uint8_t *bdf)
{
  if(strlen((const char*)bdf) != BDF_LENGTH - 1 ||
      bdf[4] != ':' ||
      bdf[7] != ':' ||
      bdf[10] != '.') {
      return false;
  }

  if( isxdigit(bdf[0]) == 0 ||
      isxdigit(bdf[1]) == 0 ||
      isxdigit(bdf[2]) == 0 ||
      isxdigit(bdf[3]) == 0 ||
      isxdigit(bdf[5]) == 0 ||
      isxdigit(bdf[6]) == 0 ||
      isxdigit(bdf[8]) == 0 ||
      isxdigit(bdf[9]) == 0 ||
      isxdigit(bdf[11]) == 0) {
        return false;
      }

  return true;
}

/**
 * the correct bdf string looks like: 02.0
*/
bool is_valid_dev_func(uint8_t *df)
{
  if(strlen((const char*)df) != DF_LENGTH - 1 ||
      df[2] != '.') {
      return false;
  }

  if(isxdigit(df[0]) == 0 ||
      isxdigit(df[1]) == 0 ||
      isxdigit(df[3]) == 0) {
        return false;
      }

  return true;
}


bool parse_bdf_string(uint8_t *bdf, uint16_t* segment, uint8_t* bus, uint8_t* device, uint8_t* function)
{
    char buffer[BDF_LENGTH] = {0};
    char *ptr = buffer;
    uint32_t res = 0;

    if(bdf == NULL || strlen((const char*)bdf) != BDF_LENGTH - 1 || segment == NULL || bus == NULL || device == NULL || function == NULL) {
      return false;
    }

    memcpy(buffer, bdf, sizeof(buffer));
    buffer[4] = '\0';
    buffer[7] = '\0';
    buffer[10] = '\0';

    sscanf(ptr, "%x", &res);
    *segment = (uint16_t)res;

    ptr += 5;
    sscanf(ptr, "%x", &res);
    *bus = (uint8_t)res;

    ptr += 3;
    sscanf(ptr, "%x", &res);
    *device = (uint8_t)res;

    ptr += 3;
    sscanf(ptr, "%x", &res);
    *function = (uint8_t)res;

    return true;
}

bool unset_device_info(int fd)
{
    if(fd <= 0) {
        TEEIO_ASSERT(false);
        return false;
    }

    for(int i = 0; i < MAX_SUPPORT_DEVICE_NUM; i++) {
        if(devices[i].fd == fd) {
            devices[i].fd = 0;
            memset(devices[i].device_name, 0, sizeof(devices[i].device_name));
            return true;
        }
    }

    return true;
}

bool set_deivce_info(int fd, char* device_name)
{
    if(fd <= 0 || device_name == NULL || strlen(device_name) > MAX_NAME_LENGTH - 1) {
        TEEIO_ASSERT(false);
        return false;
    }

    for(int i = 0; i < MAX_SUPPORT_DEVICE_NUM; i++) {
        if(devices[i].fd == fd) {
            TEEIO_ASSERT(strcmp(devices[i].device_name, device_name) == 0);
            return true;
        } else if(devices[i].fd == 0) {
            // This is an empty slot
            devices[i].fd = fd;
            strncpy(devices[i].device_name, device_name, strlen(device_name));
            return true;
        }
    }

    return false;
}

IDE_TEST_DEVICES_INFO *get_device_info_by_fd(int fd)
{
    if(fd <= 0) {
        TEEIO_ASSERT(false);
        return NULL;
    }

    for(int i = 0; i < MAX_SUPPORT_DEVICE_NUM; i++) {
        if(devices[i].fd == fd) {
            return devices + i;
        }
    }

    return NULL;
}

uint32_t device_pci_read_32(uint32_t off_to_the_cfg_start, int fd){
    uint32_t data;
    IDE_TEST_DEVICES_INFO *device = NULL;

    TEEIO_ASSERT (fd > 0);

    lseek(fd,off_to_the_cfg_start,SEEK_SET);
    read(fd, &data, 4);

    if(g_pci_log) {
        device = get_device_info_by_fd(fd);
        if(device) {
          TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "PCI_READ32  : 0x%04x => 0x%08x (%s)\n", off_to_the_cfg_start, data, device->device_name));
        }
    }

    return data;
}

void device_pci_write_32(uint32_t off_to_the_cfg_start, uint32_t value, int fd){
    IDE_TEST_DEVICES_INFO *device = NULL;

    TEEIO_ASSERT (fd > 0);

    lseek(fd,off_to_the_cfg_start,SEEK_SET);
    write(fd, &value, 4);

    if(g_pci_log) {
        device = get_device_info_by_fd(fd);
        if(device) {
          TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "PCI_WRITE32 : 0x%04x <= 0x%08x (%s)\n", off_to_the_cfg_start, value, device->device_name));
        }
    }
}

uint16_t device_pci_read_16(uint32_t off_to_the_cfg_start, int fd){
    uint16_t data;
    IDE_TEST_DEVICES_INFO *device = NULL;

    TEEIO_ASSERT (fd > 0);

    lseek(fd,off_to_the_cfg_start,SEEK_SET);
    read(fd, &data, 2);

    if(g_pci_log) {
        device = get_device_info_by_fd(fd);
        if(device) {
          TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "PCI_READ16  : 0x%04x => 0x%04x (%s)\n", off_to_the_cfg_start, data, device->device_name));
        }
    }

    return data;
}

void device_pci_write_16(uint32_t off_to_the_cfg_start, uint16_t value, int fd){
    IDE_TEST_DEVICES_INFO *device = NULL;

    TEEIO_ASSERT (fd > 0);

    lseek(fd,off_to_the_cfg_start,SEEK_SET);
    write(fd, &value, 2);

    if(g_pci_log) {
        device = get_device_info_by_fd(fd);
        if(device) {
          TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "PCI_WRITE16 : 0x%04x <= 0x%04x (%s)\n", off_to_the_cfg_start, value, device->device_name));
        }
    }
}

void mmio_write_reg64(
    void *const reg_ptr,
    const uint64_t reg_val)
{
    *(volatile uint64_t *)reg_ptr = reg_val;
}

uint64_t mmio_read_reg64(void *reg_ptr)
{
    uint64_t data = 0;
    data = *(volatile uint64_t *)reg_ptr;
    return data;
}

void mmio_write_reg32(
    void *const reg_ptr,
    const uint32_t reg_val)
{
    *(volatile uint32_t *)reg_ptr = reg_val;
}

uint32_t mmio_read_reg32(void *reg_ptr)
{
    uint32_t data = 0;
    data = *(volatile uint32_t *)reg_ptr;
    return data;
}

// memory(reg block) copy in 4Bytes aligned
void reg_memcpy_dw(void *dst, uint64_t dst_bytes, void *src, uint64_t nbytes)
{
    int i = 0;
    TEEIO_ASSERT(dst_bytes == nbytes);
    TEEIO_ASSERT(dst_bytes%4 == 0);
    
    for(i = 0; i < dst_bytes>>2; i++) {
        mmio_write_reg32(dst + i*4, *(uint32_t *)(src + i*4));
    }
}
