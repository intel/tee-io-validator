/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include "pcie.h"
#include "intel_keyp.h"
#include "utils.h"
#include "ide_test.h"
#include "teeio_debug.h"

#define MAX_SUPPORT_DEVICE_NUM  32
extern FILE* m_logfile;

#define MAX_TIME_STAMP_LENGTH 32

bool IsValidDecimalString(
    uint8_t *Decimal,
    uint32_t Length);

bool IsValidHexString(
    uint8_t *Hex,
    uint32_t Length);

IDE_TEST_DEVICES_INFO devices[MAX_SUPPORT_DEVICE_NUM] = {0};

const char* m_ide_log_level[] = {
  "error",
  "warn",
  "info",
  "verbose"
};

extern bool g_pci_log;
// extern uint8_t* g_rp_kcbar_address;
// extern int g_rp_keyslot_num;
// extern int g_rp_stream_num;
// extern ROOT_PORT_TEST_CONFIG g_rp_config;

// #define PCIE_STREAM_CAP_BASE                        0
// #define STREAM_CONFIG_REG_BLOCK_BASE                4
// #define TX_KEYSLOT_BASE(stream_num)                 (STREAM_CONFIG_REG_BLOCK_BASE + (stream_num)*0x24)
// #define TX_IVSLOT_BASE(stream_num, keyslot_num)     (TX_KEYSLOT_BASE(stream_num) + (keyslot_num)*0x20)
// #define RX_KEYSLOT_BASE(stream_num, keyslot_num)    (TX_IVSLOT_BASE(stream_num, keyslot_num) + (keyslot_num)*0x8)
// #define RX_IVSLOT_BASE(stream_num, keyslot_num)     (RX_KEYSLOT_BASE(stream_num, keyslot_num) + (keyslot_num)*0x20)
// #define MAX_KCBAR_OFFSET(stream_num, keyslot_num)   (RX_IVSLOT_BASE(stream_num, keyslot_num) + (keyslot_num)*0x8)

// #define MAX_REG_INFO_LENGTH    128

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
          TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "PCI_READ  : 0x%04x => 0x%08x (%s)\n", off_to_the_cfg_start, data, device->device_name));
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
          TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "PCI_WRITE : 0x%04x <= 0x%08x (%s)\n", off_to_the_cfg_start, value, device->device_name));
        }
    }
}

// get the kcbar register info based on reg_ptr
// the info is saved in buffer
// bool get_reg_info(void *const reg_ptr, char *info, int len, int* reg_offset, bool* keyiv_block)
// {
//     char buffer[MAX_REG_INFO_LENGTH] = {0};

//     TEEIO_ASSERT(reg_ptr != NULL && info != NULL && keyiv_block != NULL && reg_offset != NULL);

//     TEEIO_ASSERT((uint8_t *)reg_ptr >= g_rp_kcbar_address);

//     int offset = (uint8_t *)reg_ptr - g_rp_kcbar_address;
//     TEEIO_ASSERT(offset < MAX_KCBAR_OFFSET(g_rp_stream_num, g_rp_keyslot_num));

//     *keyiv_block = false;

//     if(offset < STREAM_CONFIG_REG_BLOCK_BASE) {
//         // This is INTEL_KEYP_PCIE_STREAM_CAP
//         sprintf(buffer, "rp_stream_cap");
//     } else if (offset < TX_KEYSLOT_BASE(g_rp_stream_num)) {
//         // This is INTEL_KEYP_STREAM_CONFIG_REG_BLOCK
//         sprintf(buffer, "rp_stream[%d]_config(%d)", g_rp_config.stream_id, (offset - STREAM_CONFIG_REG_BLOCK_BASE)/4);
//     } else if (offset < TX_IVSLOT_BASE(g_rp_stream_num, g_rp_keyslot_num)) {
//         // This is TX KEYSLOT
//         sprintf(buffer, "tx keyslot[%d]", (offset - TX_KEYSLOT_BASE(g_rp_stream_num))/0x20);
//         *keyiv_block = true;
//     } else if (offset < RX_KEYSLOT_BASE(g_rp_stream_num, g_rp_keyslot_num)) {
//         // This is TX IVSLOT
//         sprintf(buffer, "tx ivslot[%d]", (offset - TX_IVSLOT_BASE(g_rp_stream_num, g_rp_keyslot_num))/0x8);
//         *keyiv_block = true;
//     } else if (offset < RX_IVSLOT_BASE(g_rp_stream_num, g_rp_keyslot_num)) {
//         // This is RX KEYSLOT
//         sprintf(buffer, "rx keyslot[%d]", (offset - RX_KEYSLOT_BASE(g_rp_stream_num, g_rp_keyslot_num))/0x20);
//         *keyiv_block = true;
//     } else {
//         // This is RX IVSLOT
//         sprintf(buffer, "rx ivslot[%d]", (offset - RX_IVSLOT_BASE(g_rp_stream_num, g_rp_keyslot_num))/0x8);
//         *keyiv_block = true;
//     }

//     TEEIO_ASSERT(len > strlen(buffer) + 1);
//     strncpy(info, buffer, strlen(buffer));

//     *reg_offset = offset;

//     return true;
// }

void mmio_write_reg32(
    void *const reg_ptr,
    const uint32_t reg_val)
{
    // char reg_info[MAX_REG_INFO_LENGTH] = {0};
    // bool keyiv_block = false;
    // int reg_offset = 0;

    *(volatile uint32_t *)reg_ptr = reg_val;

    // if(g_pci_log && get_reg_info(reg_ptr, reg_info, MAX_REG_INFO_LENGTH, &reg_offset, &keyiv_block) && !keyiv_block) {
    //     TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "MMIO_WRITE : 0x%04x <= 0x%08x   %s\n", reg_offset, reg_val, reg_info));
    // }
}

uint32_t mmio_read_reg32(void *reg_ptr)
{
    // char reg_info[MAX_REG_INFO_LENGTH] = {0};
    // bool keyiv_block = false;
    // int reg_offset = 0;
    uint32_t data = 0;

    data = *(volatile uint32_t *)reg_ptr;

    // if(g_pci_log && get_reg_info(reg_ptr, reg_info, MAX_REG_INFO_LENGTH, &reg_offset, &keyiv_block) && !keyiv_block) {
    //     TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "MMIO_READ  : 0x%04x => 0x%08x   %s\n", reg_offset, data, reg_info));
    // }

    return data;
}

// memory(reg block) copy in 4Bytes aligned
void reg_memcpy_dw(void *dst, uint64_t dst_bytes, void *src, uint64_t nbytes)
{
    // char reg_info[MAX_REG_INFO_LENGTH] = {0};
    // bool keyiv_block = false;
    // int reg_offset = 0;
    // char str[MAX_REG_INFO_LENGTH] = {0};
    int i = 0;
    // int print_len = 0;

    TEEIO_ASSERT(dst_bytes == nbytes);
    TEEIO_ASSERT(dst_bytes%4 == 0);
    
    for(i = 0; i < dst_bytes>>2; i++) {
        mmio_write_reg32(dst + i*4, *(uint32_t *)(src + i*4));
    }

    // // This shall be key/iv write
    // if(g_pci_log && get_reg_info(dst, reg_info, MAX_REG_INFO_LENGTH, &reg_offset, &keyiv_block)) {
    //     TEEIO_ASSERT(keyiv_block);
    //     print_len = nbytes <= 8 ? nbytes : 8;
    //     for(i = 0; i < print_len; i++) {
    //         sprintf(str + i*3, "%02x ", *((uint8_t *)src + i));
    //     }
    //     if(print_len < nbytes) {
    //         sprintf(str + i*3, "...");
    //     }
    //     TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "MMIO_WRITE : 0x%04x <= 0x%02x bytes   %s (%s)\n", reg_offset, nbytes, reg_info, str));
    // }
}

void dump_cfg_space_to_file(const char* filepath, int fd){
    FILE* cfg_file = fopen(filepath, "w");
    if(!cfg_file){
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "failed to open file: %s to write!\n", filepath));
        return;
    }else{
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Successful open file %s for writing!\n", filepath));
    }

    // prepare the banner
    // fprintf(cfg_file, "        00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F\n");
    // fprintf(cfg_file, "        -----------------------------------------------------------------------------------------------\n");

    fprintf(cfg_file, "        00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
    fprintf(cfg_file, "        -----------------------------------------------\n");

    uint8_t bt = 0;
    for (uint32_t i = 0; i < 0x1000; i++){
        lseek(fd, i, SEEK_SET);
        size_t cnt = read(fd, &bt, 1);
        if(cnt != 1){
            TEEIO_DEBUG((TEEIO_DEBUG_INFO, "cnt != 1!\n"));
            return;
        }

        if ((i % 16) == 0){
            fprintf(cfg_file, "0x%04x: ", i);
        }

        fprintf(cfg_file, "%02x", bt);

        // if (((i + 1) % 8) == 0) {
        //     fprintf(cfg_file, " ");
        // }

        if (((i + 1) % 16) == 0) {
            fprintf(cfg_file, "\n");
        }else{
            fprintf(cfg_file, " ");
        }
    }

    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Successful dumped cfg space to file %s\n", filepath));
    fclose(cfg_file);
    return;
}

// Function to calculate the checksum of an ACPI table
uint8_t calculate_checksum(uint8_t *table, size_t length) {
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += table[i];
    }
    return sum;
}

extern const char *IDE_PORT_TYPE_NAMES[];
extern const char *IDE_TEST_IDE_TYPE_NAMES[];
extern const char *IDE_TEST_CONNECT_TYPE_NAMES[];
extern const char *IDE_TEST_TOPOLOGY_TYPE_NAMES[];

/**
 * the correct bdf string looks like: 2a:00.0
*/
bool is_valid_bdf(uint8_t *bdf)
{
  if(strlen((const char*)bdf) != BDF_LENGTH - 1 ||
      bdf[2] != ':' ||
      bdf[5] != '.') {
      return false;
  }

  if(isxdigit(bdf[0]) == 0 ||
      isxdigit(bdf[1]) == 0 ||
      isxdigit(bdf[3]) == 0 ||
      isxdigit(bdf[4]) == 0 ||
      isxdigit(bdf[6]) == 0) {
        return false;
      }

  return true;
}

int find_char_in_str(const char *str, char c)
{
  if(str == NULL) {
    return -1;
  }
  int i = 0;
  int size = strlen(str);

  for(; i < size; i++) {
    if(str[i] == c) {
      break;
    }
  }

  if(i == size) {
    i = -1;
  }

  return i;
}

int revert_find_char_in_str(const char *str, char c)
{
  if(str == NULL) {
    return -1;
  }
  int i = 0;
  int size = strlen(str);

  for(i = size; i >= 0; i--) {
    if(str[i] == c) {
      break;
    }
  }

  if(i < 0) {
    i = -1;
  }

  return i;
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


bool parse_bdf_string(uint8_t *bdf, uint8_t* bus, uint8_t* device, uint8_t* function)
{
    char buffer[BDF_LENGTH] = {0};
    char *ptr = buffer;
    uint32_t res = 0;

    if(bdf == NULL || strlen((const char*)bdf) != BDF_LENGTH - 1 || bus == NULL || device == NULL || function == NULL) {
      return false;
    }

    memcpy(buffer, bdf, sizeof(buffer));
    buffer[2] = '\0';
    buffer[5] = '\0';
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

bool is_valid_topology_connection(uint8_t *connection)
{
  if(connection == NULL) {
    return false;
  }
  for(int i = 0; i < IDE_TEST_CONNECT_NUM; i++) {
    if(strcmp(IDE_TEST_CONNECT_TYPE_NAMES[i], (const char *)connection) == 0) {
      return true;
    }
  }
  return false;
}

IDE_TEST_CONNECT_TYPE get_connection_from_name(uint8_t *name)
{
  if(name == NULL) {
    return IDE_TEST_CONNECT_NUM;
  }
  for(int i = 0; i < IDE_TEST_CONNECT_NUM; i++) {
    if(strcmp(IDE_TEST_CONNECT_TYPE_NAMES[i], (const char *)name) == 0) {
      return i;
    }
  }
  return IDE_TEST_CONNECT_NUM;
}

bool is_valid_port(IDE_TEST_PORTS_CONFIG *ports, uint8_t *port)
{
  if(ports == NULL || port == NULL) {
    return false;
  }

  for(int i = 0; i < ports->cnt; i++) {
    if(strcmp(ports->ports[i].port_name, (const char*)port) == 0) {
      return ports->ports[i].enabled;
    }
  }

  return false;
}
int get_port_id_from_name(IDE_TEST_PORTS_CONFIG *ports, uint8_t* port)
{
  if(ports == NULL || port == NULL) {
    return INVALID_PORT_ID;
  }

  for(int i = 0; i < ports->cnt; i++) {
    if(strcmp(ports->ports[i].port_name, (const char*)port) == 0) {
      return ports->ports[i].enabled ? ports->ports[i].id : INVALID_PORT_ID;
    }
  }

  return INVALID_PORT_ID;
}

// IDE_TEST_TOPOLOGY_TYPE_NAMES
bool is_valid_topology_type(uint8_t *type)
{
  if(type == NULL) {
    return false;
  }
  for(int i = 0; i < IDE_TEST_TOPOLOGY_TYPE_NUM; i++) {
    if(strcmp((const char*)type, IDE_TEST_TOPOLOGY_TYPE_NAMES[i]) == 0) {
      return true;
    }
  }
  return false;
}

IDE_TEST_TOPOLOGY_TYPE get_topology_type_from_name(uint8_t* name)
{
  if(name == NULL) {
    return IDE_TEST_TOPOLOGY_TYPE_NUM;
  }

  for(int i = 0; i < IDE_TEST_TOPOLOGY_TYPE_NUM; i++) {
    if(strcmp((const char*)name, IDE_TEST_TOPOLOGY_TYPE_NAMES[i]) == 0) {
      return i;
    }
  }
  return IDE_TEST_TOPOLOGY_TYPE_NUM;
}

IDE_TEST_CONFIGURATION* get_configuration_by_id(IDE_TEST_CONFIG *test_config, int id)
{
  if(test_config == NULL || id <= 0) {
    return NULL;
  }

  for(int i = 0; i < MAX_CONFIGURATION_NUM; i++) {
    if(test_config->configurations.configurations[i].id == id && test_config->configurations.configurations[i].enabled) {
      return test_config->configurations.configurations + i;
    }
  }

  return NULL;  
}

IDE_TEST_TOPOLOGY* get_topology_by_id(IDE_TEST_CONFIG *test_config, int id)
{
  if(test_config == NULL || id <= 0) {
    return NULL;
  }

  for(int i = 0; i < MAX_TOPOLOGY_NUM; i++) {
    if(test_config->topologies.topologies[i].id == id && test_config->topologies.topologies[i].enabled) {
      return test_config->topologies.topologies + i;
    }
  }

  return NULL;
}

IDE_PORT* get_port_by_id(IDE_TEST_CONFIG *test_config, int id)
{
  if(test_config == NULL || id <= 0) {
    return NULL;
  }

  for(int i = 0; i < test_config->ports_config.cnt; i++) {
    if(test_config->ports_config.ports[i].id == id && test_config->ports_config.ports[i].enabled) {
      return test_config->ports_config.ports + i;
    }
  }

  return NULL;
}

IDE_PORT* get_port_by_name(IDE_TEST_CONFIG *test_config, const char* portname)
{
  if(test_config == NULL || portname == NULL) {
    return NULL;
  }

  for(int i = 0; i < test_config->ports_config.cnt; i++) {
    if(strcmp(test_config->ports_config.ports[i].port_name, portname) == 0 && test_config->ports_config.ports[i].enabled) {
      return test_config->ports_config.ports + i;
    }
  }

  return NULL;
}

IDE_SWITCH *get_switch_by_id(IDE_TEST_CONFIG *test_config, int switch_id)
{
  if(test_config == NULL || switch_id <= 0) {
    return NULL;
  }

  for(int i = 0; i < test_config->switches_config.cnt; i++) {
    if(test_config->switches_config.switches[i].id == switch_id && test_config->switches_config.switches[i].enabled) {
      return test_config->switches_config.switches + i;
    }
  }

  return NULL;
}

IDE_SWITCH *get_switch_by_name(IDE_TEST_CONFIG *test_config, const char* name)
{
  if(test_config == NULL || name == NULL) {
    return NULL;
  }

  for(int i = 0; i < test_config->switches_config.cnt; i++) {
    if(strcmp(test_config->switches_config.switches[i].name, name) == 0 && test_config->switches_config.switches[i].enabled) {
      return test_config->switches_config.switches + i;
    }
  }

  return NULL;
}


bool is_valid_switch_by_id(IDE_TEST_CONFIG *test_config, int switch_id)
{
  IDE_SWITCH *sw = get_switch_by_id(test_config, switch_id);
  return sw != NULL;
}

IDE_PORT* get_port_from_switch_by_id(IDE_SWITCH *sw, int port_id)
{
  if(sw == NULL || port_id <= 0) {
    return NULL;
  }

  for(int i = 0; i < sw->ports_cnt; i++) {
    if(sw->ports[i].enabled && sw->ports[i].id == port_id) {
      return sw->ports + i;
    }
  }

  return NULL;
}

bool is_valid_port_in_switch(IDE_SWITCH *sw, int port_id)
{
  return get_port_from_switch_by_id(sw, port_id) != NULL;
}

IDE_PORT *get_port_from_switch_by_name(IDE_SWITCH *sw, const char* name)
{
  if(sw == NULL || name == NULL) {
    return NULL;
  }

  for(int i = 0; i < sw->ports_cnt; i++) {
    if(sw->ports[i].enabled && strcmp(sw->ports[i].port_name, name) == 0) {
      return sw->ports + i;
    }
  }

  return NULL;
}

/**
 * validate the file exists
*/
bool validate_file_name(const char *file_name)
{
  if(file_name == NULL) {
    return false;
  }

  if(strlen(file_name) > MAX_FILE_NAME) {
    return false;
  }

  return access(file_name, F_OK) != -1;
}

bool revert_copy_by_dw(void* src, int src_size, void* dest, int dest_size)
{
    if(src == NULL || dest == NULL ||
      src_size == 0 || src_size != dest_size || src_size%4 != 0) {
        TEEIO_ASSERT(false);
        return false;
    }

    int size_in_dw = src_size/4;
    for(int i = 0; i < size_in_dw; i++) {
        *((uint32_t *)dest + i) = *((uint32_t *)src + size_in_dw - i - 1);
    }

    return true;
}

// "1,2,3,4" => [1,2,3,4]
bool decimal_str_to_array(const char* str, int* array, int size)
{
  if(str == NULL || strlen(str) == 0) {
    return false;
  }

  bool valid = false;
  int len = strlen(str) + 2;
  char *buffer = (char*)malloc(len);
  strncpy(buffer, str, len);
  buffer[len] = ',';

  char *ptr = buffer;
  int cnt = 0;
  int pos = 0;
  char *end_ptr = NULL;
  uint32_t result;
  bool convert = array != NULL && size != 0;

  while(pos < len) {
    pos = find_char_in_str(ptr, ',');
    if(pos == -1) {
      break;
    }

    ptr[pos] = '\0';
    valid = IsValidDecimalString((uint8_t *)ptr, (uint32_t)strlen(ptr));
    if(!valid) {
      break;
    }

    if(convert) {
      end_ptr = NULL;
      result = strtoul((const char *)ptr, &end_ptr, 10);
      if (*end_ptr != '\0') {
        valid = false;
        break;
      }
      if(cnt == size) {
        // overflow of array
        valid = false;
        break;
      }
      array[cnt] = (int)result;
      cnt++;
      valid = true;
    }

    pos++;
    ptr += pos;
  }

  free(buffer);

  return valid;
}

bool valid_decimal_int_array(const char* str)
{
  return decimal_str_to_array(str, NULL, 0);
}

TEEIO_DEBUG_LEVEL get_ide_log_level_from_string(const char* debug_level)
{
  if(debug_level == NULL) {
    return TEEIO_DEBUG_WARN;
  }

  for(int i = 0; i < TEEIO_DEBUG_NUM; i++) {
    if(strcmp(debug_level, m_ide_log_level[i]) == 0) {
      return i;
    }
  }
  return TEEIO_DEBUG_WARN;
}

const char* get_ide_log_level_string(TEEIO_DEBUG_LEVEL debug_level)
{
  if(debug_level > TEEIO_DEBUG_NUM) {
    return "na";
  }

  return m_ide_log_level[debug_level];
}

bool convert_hex_str_to_uint8(char* str, uint8_t* data8)
{
  unsigned long result = 0;
  char *end_ptr = NULL;

  if(str == NULL || data8 == NULL) {
    return false;
  }

  if(!IsValidHexString((uint8_t *)str, (uint32_t)strlen(str))) {
    return false;
  }

  result = strtoul((const char *)str, &end_ptr, 0);
  if (*end_ptr != '\0') {
    return false;
  }

  if(result > UINT8_MAX) {
    return false;
  }

  *data8 = (uint8_t)result;
  return true;
}

bool log_file_init(const char* log_file){
  if(m_logfile > 0) {
    TEEIO_DEBUG((TEEIO_DEBUG_WARN, "m_logfile exists. Close it before init a new logfile.\n"));
    fclose(m_logfile);
    m_logfile = 0;
  }

  char full_log_file[MAX_FILE_NAME] = {0};
  char current_time_stamp[MAX_TIME_STAMP_LENGTH] = {0};
  struct timeval currentTime;
  gettimeofday(&currentTime, NULL);

  // Convert to local time
  time_t rawtime = currentTime.tv_sec;
  struct tm *localTime = localtime(&rawtime);

  strftime(current_time_stamp, MAX_TIME_STAMP_LENGTH, "%Y-%m-%d_%H-%M-%S", localTime);

  snprintf(full_log_file, MAX_FILE_NAME, "%s_%s.txt", log_file, current_time_stamp);
  m_logfile = fopen(full_log_file, "w");
  if(!m_logfile){
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to initialize log file [%s]\n", full_log_file));
    return false;
  }

  return true;
}

void log_file_close(){
    if(!m_logfile){
        fclose(m_logfile);
    }
}
