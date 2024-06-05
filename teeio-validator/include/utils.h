/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
// #include "intel_ide_test.h"
#include "pcie.h"
#include "intel_keyp.h"
#include "ide_test.h"
#include "teeio_debug.h"

typedef struct
{
    int fd;
    char device_name[MAX_NAME_LENGTH];
} IDE_TEST_DEVICES_INFO;

uint32_t device_pci_read_32 (uint32_t offset, int fp);
void device_pci_write_32 (uint32_t offset, uint32_t data, int fp);

void mmio_write_reg32(void *const reg_ptr, const uint32_t reg_val);
uint32_t mmio_read_reg32(void *reg_ptr);

void mmio_write_reg8(void *const reg_ptr, const uint8_t reg_val);
uint8_t mmio_read_reg8(void *reg_ptr);

void reg_memcpy_dw(void *dst, uint64_t dst_bytes, void *src, uint64_t nbytes);

void dump_cfg_space_to_file(const char* filepath, int fp);

// Function to calculate the checksum of an ACPI table
uint8_t calculate_checksum(uint8_t *table, size_t length);

bool set_deivce_info(int fp, char* device_name);
bool unset_device_info(int fd);
IDE_TEST_DEVICES_INFO *get_device_info_by_fd(int fp);

bool is_valid_bdf(uint8_t *bdf);
bool parse_bdf_string(uint8_t* bdf, uint8_t* bus, uint8_t* device, uint8_t* function);
bool is_valid_dev_func(uint8_t *df);

bool is_valid_topology_type(uint8_t *type);
IDE_TEST_TOPOLOGY_TYPE get_topology_type_from_name(uint8_t* name);

bool is_valid_topology_connection(uint8_t *connection);
IDE_TEST_CONNECT_TYPE get_connection_from_name(uint8_t* name);

bool is_valid_port(IDE_TEST_PORTS_CONFIG *ports, uint8_t* port);
int get_port_id_from_name(IDE_TEST_PORTS_CONFIG *ports, uint8_t* port);

IDE_TEST_TOPOLOGY* get_topology_by_id(IDE_TEST_CONFIG *test_config, int id);
IDE_TEST_CONFIGURATION* get_configuration_by_id(IDE_TEST_CONFIG *test_config, int id);
IDE_PORT* get_port_by_id(IDE_TEST_CONFIG *test_config, int id);
IDE_PORT* get_port_by_name(IDE_TEST_CONFIG *test_config, const char* portname);

bool revert_copy_by_dw(void* src, int src_size, void* dest, int dest_size);

bool validate_file_name(const char *file_name);
bool valid_decimal_int_array(const char* str);
bool decimal_str_to_array(const char* str, int* array, int size);

int find_char_in_str(const char *str, char c);
int revert_find_char_in_str(const char *str, char c);

bool is_valid_switch_by_id(IDE_TEST_CONFIG *test_config, int switch_id);
IDE_SWITCH *get_switch_by_id(IDE_TEST_CONFIG *test_config, int switch_id);
IDE_SWITCH *get_switch_by_name(IDE_TEST_CONFIG *test_config, const char* name);
IDE_PORT *get_port_from_switch_by_name(IDE_SWITCH *sw, const char* name);
bool is_valid_port_in_switch(IDE_SWITCH *sw, int port_id);
IDE_PORT* get_port_from_switch_by_id(IDE_SWITCH *sw, int port_id);

TEEIO_DEBUG_LEVEL get_ide_log_level_from_string(const char* debug_level);
const char* get_ide_log_level_string(TEEIO_DEBUG_LEVEL debug_level);

bool convert_hex_str_to_uint8(char* str, uint8_t* data8);
void dump_hex_array(uint8_t* data, int size);

/**
 * Suspends the execution of the current thread until the time-out interval elapses.
 *
 * @param microseconds     The time interval for which execution is to be suspended, in microseconds.
 *
 **/
extern void libspdm_sleep(uint64_t microseconds);

TEST_IDE_TYPE map_top_type_to_ide_type(IDE_TEST_TOPOLOGY_TYPE top_type);

#endif
