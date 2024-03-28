/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __IDE_TEST_INI_H__
#define __IDE_TEST_INI_H__

#include <stdint.h>
#include <library/spdm_return_status.h>

/**
  Open an INI config file and return a context.

  @param[in] DataBuffer      Config raw file buffer.
  @param[in] BufferSize      Size of raw buffer.

  @return       Config data buffer is opened and context is returned.
  @retval NULL  No enough memory is allocated.
  @retval NULL  Config data buffer is invalid.
**/
void *
OpenIniFile(
    uint8_t *DataBuffer,
    uint32_t BufferSize);

/**
  Get section entry string value.

  @param[in]  Context         INI Config file context.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] EntryValue      Point to the got entry string value.

  @retval true    Section entry string value is got.
  @retval false   Section is not found.
**/
bool GetStringFromDataFile(
    void *Context,
    uint8_t *SectionName,
    uint8_t *EntryName,
    uint8_t **EntryValue);

/**
  Get section entry decimal uint32_t value.

  @param[in]  Context         INI Config file context.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] Data            Point to the got decimal uint32_t value.

  @retval true    Section entry decimal uint32_t value is got.
  @retval false   Section is not found.
**/
bool GetDecimalUint32FromDataFile(
    void *Context,
    uint8_t *SectionName,
    uint8_t *EntryName,
    uint32_t *Data);

/**
  Get section entry hexadecimal uint32_t value.

  @param[in]  Context         INI Config file context.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] Data            Point to the got hexadecimal uint32_t value.

  @retval true    Section entry hexadecimal uint32_t value is got.
  @retval false   Section is not found.
**/
bool GetHexUint32FromDataFile(
    void *Context,
    uint8_t *SectionName,
    uint8_t *EntryName,
    uint32_t *Data);

/**
  Get section entry decimal uint32_t array.
  "1,2 ,3,15" => [1,2,3,15]

  @param[in]  Context         INI Config file context.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] Data            Point to the got hexadecimal uint64_t value.

  @retval true      Section entry decimal uint32_t array is got.
  @retval false     Failed to get the array.
  @retval 
**/
bool
GetUint32ArrayFromDataFile(
    void *Context,
    uint8_t *SectionName,
    uint8_t *EntryName,
    uint32_t *DataArray,
    uint32_t DataArrayLength);

/**
  Get the array length of section entry decimal uint32_t array.
  "1,2 ,3,15" => 4

  @param[in]  Context         INI Config file context.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] Data            Point to the got hexadecimal uint64_t value.

  @retval true      Section entry decimal uint32_t array length is got.
  @retval false     Failed to get the array length.
  @retval 
**/
bool
GetUint32ArrayLengthFromDataFile(
    void *Context,
    uint8_t *SectionName,
    uint8_t *EntryName,
    uint32_t *DataArrayLength);

/**
  Close an INI config file and free the context.

  @param[in] Context         INI Config file context.
**/
void CloseIniFile(
    void *Context);

#endif