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

bool is_power_of_two(uint8_t x);

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
bool parse_bdf_string(uint8_t *bdf, uint16_t* segment, uint8_t* bus, uint8_t* device, uint8_t* function);
bool is_valid_dev_func(uint8_t *df);

// Function to calculate the checksum of an ACPI table
uint8_t calculate_checksum(uint8_t *table, size_t length);

// ide_test.ini helper APIs
bool is_valid_topology_type(uint8_t *type);
IDE_TEST_TOPOLOGY_TYPE get_topology_type_from_name(uint8_t* name);

bool is_valid_test_category(uint8_t *test_category);
TEEIO_TEST_CATEGORY get_test_category_from_name(const char* name);

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

bool pcie_construct_rp_keys(void* key_prog_keys, int key_prog_keys_size, void* rp_keys, int rp_keys_size);
bool cxl_construct_rp_keys(void* key_prog_keys, int key_prog_keys_size, void* rp_keys, int rp_keys_size);
bool cxl_construct_rp_iv(uint32_t* key_prog_iv, int key_prog_iv_size, uint32_t* rp_iv, int rp_iv_size);
bool IsValidDecimalString(uint8_t *Decimal, uint32_t Length);
bool IsValidDigitalChar(uint8_t DigitalChar, bool IncludeHex);
bool IsValidHexString(uint8_t *Hex, uint32_t Length);
bool IsValidDigital(uint8_t *Digital, uint32_t Length, bool IncludeHex);
int find_char_in_str(const char *str, char c);
int revert_find_char_in_str(const char *str, char c);
bool convert_hex_str_to_uint16(char* str, uint16_t* data16);
bool convert_hex_str_to_uint8(char* str, uint8_t* data8);
void dump_hex_array(uint8_t* data, int size);
bool set_deivce_info(int fp, char* device_name);
bool unset_device_info(int fd);

TEST_IDE_TYPE map_top_type_to_ide_type(IDE_TEST_TOPOLOGY_TYPE top_type);

// file related helper APIs
bool validate_file_name(const char *file_name);

// get uint32_t array from string. "1,2,3,4,5,6" is an example.
bool get_uint32_array_from_string(uint32_t* array, uint32_t* array_size, const char* string);
// get the max value from uint32_t array
uint32_t get_max_from_uint32_array(uint32_t* array, uint32_t size);

// dump key/iv stored in IDE-KM KEY_PROG message.
// Refer to PCIe Spec 6.1 Figure 6-57
void dump_key_iv_in_key_prog(const uint32_t *key, int key_dw_size, const uint32_t *iv, int iv_dw_size);

bool teeio_record_assertion_result(
  int case_class,
  int case_id,
  int assertion_id,
  ide_common_test_case_assertion_type_t assertion_type,
  teeio_test_result_t result,
  const char *message_format,
  ...);

/**
 * Check the test result of a case.
 * Failure or Pass of a case is decided by its assertions result.
 */
teeio_test_result_t teeio_test_case_result(
  int case_class,
  int case_id
  );

bool teeio_record_config_item_result(
  int config_item_id,
  teeio_test_config_func_t func,
  teeio_test_result_t result
  );

bool teeio_record_group_result(
  teeio_test_group_func_t func,
  teeio_test_result_t result,
  const char *message_format,
  ...  );

#endif
