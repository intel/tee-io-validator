/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __HELPER_LIB_H__
#define __HELPER_LIB_H__

#include <stdint.h>
#include "pcie.h"
#include "intel_keyp.h"
#include "ide_test.h"
#include "teeio_debug.h"

// Define the macro to get the offset of a field in a struct
#define OFFSET_OF(type, field) ((size_t) &(((type *)0)->field))

void libspdm_sleep(uint64_t microseconds);

// PCIE & MMIO helper APIs
uint32_t device_pci_read_32 (uint32_t offset, int fp);
void device_pci_write_32 (uint32_t offset, uint32_t data, int fp);

uint16_t device_pci_read_16 (uint32_t offset, int fp);
void device_pci_write_16 (uint32_t offset, uint16_t data, int fp);

void mmio_write_reg32(void *const reg_ptr, const uint32_t reg_val);
uint32_t mmio_read_reg32(void *reg_ptr);

void mmio_write_reg64(void *const reg_ptr, const uint64_t reg_val);
uint64_t mmio_read_reg64(void *reg_ptr);

void reg_memcpy_dw(void *dst, uint64_t dst_bytes, void *src, uint64_t nbytes);
bool parse_bdf_string(uint8_t* bdf, uint8_t* bus, uint8_t* device, uint8_t* function);
bool is_valid_dev_func(uint8_t *df);

// Function to calculate the checksum of an ACPI table
uint8_t calculate_checksum(uint8_t *table, size_t length);

// ide_test.ini helper APIs
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
bool is_valid_switch_by_id(IDE_TEST_CONFIG *test_config, int switch_id);
IDE_SWITCH *get_switch_by_id(IDE_TEST_CONFIG *test_config, int switch_id);
IDE_SWITCH *get_switch_by_name(IDE_TEST_CONFIG *test_config, const char* name);
IDE_PORT *get_port_from_switch_by_name(IDE_SWITCH *sw, const char* name);
bool is_valid_port_in_switch(IDE_SWITCH *sw, int port_id);
IDE_PORT* get_port_from_switch_by_id(IDE_SWITCH *sw, int port_id);

bool revert_copy_by_dw(void* src, int src_size, void* dest, int dest_size);
bool IsValidDecimalString(uint8_t *Decimal, uint32_t Length);
bool IsValidDigitalChar(uint8_t DigitalChar, bool IncludeHex);
bool IsValidHexString(uint8_t *Hex, uint32_t Length);
bool IsValidDigital(uint8_t *Digital, uint32_t Length, bool IncludeHex);
int find_char_in_str(const char *str, char c);
int revert_find_char_in_str(const char *str, char c);
bool convert_hex_str_to_uint8(char* str, uint8_t* data8);
void dump_hex_array(uint8_t* data, int size);
bool set_deivce_info(int fp, char* device_name);
bool unset_device_info(int fd);

TEST_IDE_TYPE map_top_type_to_ide_type(IDE_TEST_TOPOLOGY_TYPE top_type);

// file related helper APIs
bool validate_file_name(const char *file_name);

#endif
