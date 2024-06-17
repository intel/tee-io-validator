/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "teeio_debug.h"
#include "helperlib.h"
#include "helper_internal.h"
#include "pcap.h"

extern FILE* m_logfile;

#define MAX_TIME_STAMP_LENGTH 32
#define COLUME_SIZE 16

extern const char* m_ide_log_level[];

// Function to calculate the checksum of an ACPI table
uint8_t calculate_checksum(uint8_t *table, size_t length) {
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += table[i];
    }
    return sum;
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

/**
  Return if the decimal string is valid.

  @param[in] Decimal The decimal string to be checked.
  @param[in] Length  The length of decimal string in bytes.

  @retval true   The decimal string is valid.
  @retval false  The decimal string is invalid.
**/
bool IsValidDecimalString(
    uint8_t *Decimal,
    uint32_t Length)
{
  return IsValidDigital(Decimal, Length, false);
}

/**
  Return if the digital char is valid.

  @param[in] DigitalChar    The digital char to be checked.
  @param[in] IncludeHex     If it include HEX char.

  @retval true   The digital char is valid.
  @retval false  The digital char is invalid.
**/
bool IsValidDigitalChar(
    uint8_t DigitalChar,
    bool IncludeHex)
{
  if ((DigitalChar >= '0') && (DigitalChar <= '9'))
  {
    return true;
  }

  if (IncludeHex)
  {
    if ((DigitalChar >= 'a') && (DigitalChar <= 'f'))
    {
      return true;
    }

    if ((DigitalChar >= 'A') && (DigitalChar <= 'F'))
    {
      return true;
    }
  }

  return false;
}

/**
  Return if the hexadecimal string is valid.

  @param[in] Hex     The hexadecimal string to be checked.
  @param[in] Length  The length of hexadecimal string in bytes.

  @retval true   The hexadecimal string is valid.
  @retval false  The hexadecimal string is invalid.
**/
bool IsValidHexString(
    uint8_t *Hex,
    uint32_t Length)
{
  if (Length <= 2)
  {
    return false;
  }

  if (Hex[0] != '0')
  {
    return false;
  }

  if ((Hex[1] != 'x') && (Hex[1] != 'X'))
  {
    return false;
  }

  return IsValidDigital(&Hex[2], Length - 2, true);
}

/**
  Return if the digital string is valid.

  @param[in] Digital        The digital to be checked.
  @param[in] Length         The length of digital string in bytes.
  @param[in] IncludeHex     If it include HEX char.

  @retval true   The digital string is valid.
  @retval false  The digital string is invalid.
**/
bool IsValidDigital(
    uint8_t *Digital,
    uint32_t Length,
    bool IncludeHex)
{
  uint32_t Index;

  for (Index = 0; Index < Length; Index++)
  {
    if (!IsValidDigitalChar(Digital[Index], IncludeHex))
    {
      return false;
    }
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

static void dump_hex_array_to_str (
  uint8_t  *array,
  int  array_size,
  char *str,
  int str_size
  )
{
  int index;
  TEEIO_ASSERT(str_size >= array_size * 3);

  for (index = 0; index < array_size; index++) {
    if(index == array_size - 1) {
      sprintf(str + index * 3, "%02x", array[index]);
    } else {
      sprintf(str + index * 3, "%02x ", array[index]);
    }
  }
}

void dump_hex_array(uint8_t* data, int size)
{
  int i = 0;
  int count = size / COLUME_SIZE;
  int left = size % COLUME_SIZE;
  char one_line_buffer[COLUME_SIZE * 3] = {0};

  for(i = 0; i < count; i++) {
    memset(one_line_buffer, 0, sizeof(one_line_buffer));
    dump_hex_array_to_str(data + i * COLUME_SIZE, COLUME_SIZE, one_line_buffer, COLUME_SIZE * 3);
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%04x: %s\n", i * COLUME_SIZE, one_line_buffer));
  }

  if(left != 0) {
    memset(one_line_buffer, 0, sizeof(one_line_buffer));
    dump_hex_array_to_str(data + i * COLUME_SIZE, left, one_line_buffer, COLUME_SIZE * 3);
    TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%04x: %s\n", i * COLUME_SIZE, one_line_buffer));
  }
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

bool pcap_file_init(const char* pcap_file, uint32_t transport_layer)
{
  bool ret;

  char full_pcap_file[MAX_FILE_NAME] = {0};
  char current_time_stamp[MAX_TIME_STAMP_LENGTH] = {0};
  struct timeval currentTime;
  gettimeofday(&currentTime, NULL);

  // Convert to local time
  time_t rawtime = currentTime.tv_sec;
  struct tm *localTime = localtime(&rawtime);

  strftime(current_time_stamp, MAX_TIME_STAMP_LENGTH, "%Y-%m-%d_%H-%M-%S", localTime);

  snprintf(full_pcap_file, MAX_FILE_NAME, "%s_%s.pcap", pcap_file, current_time_stamp);
  ret = open_pcap_packet_file(full_pcap_file, transport_layer);
  if(!ret){
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to open pcap file [%s]\n", full_pcap_file));
    return false;
  }

  return true;
}

void pcap_file_close()
{
  close_pcap_packet_file();
}
