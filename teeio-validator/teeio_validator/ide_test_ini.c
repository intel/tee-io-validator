/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.

  This library parses the INI configuration file.

  The INI file format is:
    ================
    [SectionName]
    EntryName=EntryValue
    ================

    Where:
      1) SectionName is an ASCII string. The valid format is [A-Za-z0-9_]+
      2) EntryName is an ASCII string. The valid format is [A-Za-z0-9_]+
      3) EntryValue can be:
         3.1) an ASCII String. The valid format is [A-Za-z0-9_]+
         3.2) a GUID. The valid format is xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx, where x is [A-Fa-f0-9]
         3.3) a decimal value. The valid format is [0-9]+
         3.4) a hexadecimal value. The valid format is 0x[A-Fa-f0-9]+
      4) '#' or ';' can be used as comment at anywhere.
      5) TAB(0x20) or SPACE(0x9) can be used as separator.
      6) LF(\n, 0xA) or CR(\r, 0xD) can be used as line break.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "hal/base.h"
#include "hal/library/debuglib.h"

#include <library/spdm_return_status.h>
#include <industry_standard/pci_tdisp.h>
#include "helperlib.h"
#include "teeio_debug.h"
#include "ide_test.h"

extern uint16_t g_scan_segment;
extern uint8_t g_scan_bus;
extern bool g_run_test_suite;
extern pci_tdisp_interface_id_t g_tdisp_interface_id;

ide_test_case_name_t* get_test_case_from_string(const char* test_case_name, int* index, TEEIO_TEST_CATEGORY test_category);
const char* get_test_configuration_name(int configuration_type, TEEIO_TEST_CATEGORY test_category);
const char** get_test_configuration_priv_names(TEEIO_TEST_CATEGORY test_category);
bool parse_test_configuration_priv_names(const char* key, const char* value, TEEIO_TEST_CATEGORY test_category, IDE_TEST_CONFIGURATION* config);
ide_test_case_name_t* get_test_case_name(int case_class, TEEIO_TEST_CATEGORY test_category);
bool teeio_check_configuration_bitmap(uint32_t* bitmap, TEEIO_TEST_CATEGORY test_category);

const char *IDE_PORT_TYPE_NAMES[] = {
    "rootport",
    "endpoint"};

const char *IDE_TEST_IDE_TYPE_NAMES[] = {
    "selective_ide",
    "link_ide"};

const char *IDE_TEST_CONNECT_TYPE_NAMES[] = {
    "direct",
    "switch",
    "peer2peer"};

const char *IDE_TEST_TOPOLOGY_TYPE_NAMES[] = {
    "selective_ide",
    "link_ide",
    "selective_and_link_ide"};

const char *TEEIO_TEST_CATEGORY_NAMES[] = {
    "pcie-ide",
    "cxl-ide",
    "cxl-tsp",
    "tdisp",
    "spdm"
};

#define IS_HYPHEN(a) ((a) == '-')
#define IS_NULL(a) ((a) == '\0')

// This is default allocation. Reallocation will happen if it is not enough.
// #define MAX_LINE_LENGTH 512

uint8_t valid_chars[] = {
    '_', ':', ',', '.', '-'};

typedef struct _INI_SECTIOIN_ITME SECTION_ITEM;
struct _INI_SECTIOIN_ITME
{
  uint8_t *PtrSection;
  uint32_t SecNameLen;
  uint8_t *PtrEntry;
  uint8_t *PtrValue;
  SECTION_ITEM *PtrNext;
};

typedef struct _INI_COMMENT_LINE COMMENT_LINE;
struct _INI_COMMENT_LINE
{
  uint8_t *PtrComment;
  COMMENT_LINE *PtrNext;
};

typedef struct
{
  SECTION_ITEM *SectionHead;
  COMMENT_LINE *CommentHead;
} INI_PARSING_LIB_CONTEXT;

// /**
//   Return if the digital char is valid.

//   @param[in] DigitalChar    The digital char to be checked.
//   @param[in] IncludeHex     If it include HEX char.

//   @retval true   The digital char is valid.
//   @retval false  The digital char is invalid.
// **/
// bool IsValidDigitalChar(
//     uint8_t DigitalChar,
//     bool IncludeHex)
// {
//   if ((DigitalChar >= '0') && (DigitalChar <= '9'))
//   {
//     return true;
//   }

//   if (IncludeHex)
//   {
//     if ((DigitalChar >= 'a') && (DigitalChar <= 'f'))
//     {
//       return true;
//     }

//     if ((DigitalChar >= 'A') && (DigitalChar <= 'F'))
//     {
//       return true;
//     }
//   }

//   return false;
// }

/**
  Return if the name char is valid.

  @param[in] NameChar    The name char to be checked.

  @retval true   The name char is valid.
  @retval false  The name char is invalid.
**/
bool IsValidNameChar(
    uint8_t NameChar)
{
  if ((NameChar >= 'a') && (NameChar <= 'z'))
  {
    return true;
  }

  if ((NameChar >= 'A') && (NameChar <= 'Z'))
  {
    return true;
  }

  if ((NameChar >= '0') && (NameChar <= '9'))
  {
    return true;
  }

  for (int i = 0; i < sizeof(valid_chars); i++)
  {
    if (NameChar == valid_chars[i])
    {
      return true;
    }
  }

  return false;
}

// /**
//   Return if the digital string is valid.

//   @param[in] Digital        The digital to be checked.
//   @param[in] Length         The length of digital string in bytes.
//   @param[in] IncludeHex     If it include HEX char.

//   @retval true   The digital string is valid.
//   @retval false  The digital string is invalid.
// **/
// bool IsValidDigital(
//     uint8_t *Digital,
//     uint32_t Length,
//     bool IncludeHex)
// {
//   uint32_t Index;

//   for (Index = 0; Index < Length; Index++)
//   {
//     if (!IsValidDigitalChar(Digital[Index], IncludeHex))
//     {
//       return false;
//     }
//   }

//   return true;
// }

// /**
//   Return if the decimal string is valid.

//   @param[in] Decimal The decimal string to be checked.
//   @param[in] Length  The length of decimal string in bytes.

//   @retval true   The decimal string is valid.
//   @retval false  The decimal string is invalid.
// **/
// bool IsValidDecimalString(
//     uint8_t *Decimal,
//     uint32_t Length)
// {
//   return IsValidDigital(Decimal, Length, false);
// }

// /**
//   Return if the hexadecimal string is valid.

//   @param[in] Hex     The hexadecimal string to be checked.
//   @param[in] Length  The length of hexadecimal string in bytes.

//   @retval true   The hexadecimal string is valid.
//   @retval false  The hexadecimal string is invalid.
// **/
// bool IsValidHexString(
//     uint8_t *Hex,
//     uint32_t Length)
// {
//   if (Length <= 2)
//   {
//     return false;
//   }

//   if (Hex[0] != '0')
//   {
//     return false;
//   }

//   if ((Hex[1] != 'x') && (Hex[1] != 'X'))
//   {
//     return false;
//   }

//   return IsValidDigital(&Hex[2], Length - 2, true);
// }

/**
  Return if the name string is valid.

  @param[in] Name    The name to be checked.
  @param[in] Length  The length of name string in bytes.

  @retval true   The name string is valid.
  @retval false  The name string is invalid.
**/
bool IsValidName(
    uint8_t *Name,
    uint32_t Length)
{
  uint32_t Index;

  for (Index = 0; Index < Length; Index++)
  {
    if (!IsValidNameChar(Name[Index]))
    {
      return false;
    }
  }

  return true;
}

/**
  Return if the value string is valid GUID.

  @param[in] Value   The value to be checked.
  @param[in] Length  The length of value string in bytes.

  @retval true   The value string is valid GUID.
  @retval false  The value string is invalid GUID.
**/
bool IsValidGuid(
    uint8_t *Value,
    uint32_t Length)
{
  if (Length != sizeof("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx") - 1)
  {
    return false;
  }

  if (!IS_HYPHEN(Value[8]))
  {
    return false;
  }

  if (!IS_HYPHEN(Value[13]))
  {
    return false;
  }

  if (!IS_HYPHEN(Value[18]))
  {
    return false;
  }

  if (!IS_HYPHEN(Value[23]))
  {
    return false;
  }

  if (!IsValidDigital(&Value[0], 8, true))
  {
    return false;
  }

  if (!IsValidDigital(&Value[9], 4, true))
  {
    return false;
  }

  if (!IsValidDigital(&Value[14], 4, true))
  {
    return false;
  }

  if (!IsValidDigital(&Value[19], 4, true))
  {
    return false;
  }

  if (!IsValidDigital(&Value[24], 12, true))
  {
    return false;
  }

  return true;
}

/**
  Return if the value string is valid decimal array

  @param[in] Value   The value to be checked.
  @param[in] Length  The length of value string in bytes.

  @retval true   The value string is valid decimal array.
  @retval false  The value string is invalid decimal array.
**/
bool IsValidDecimalArray(
    uint8_t *Value,
    uint32_t Length)
{
  bool valid = false;

  if (Value == NULL || Length == 0)
  {
    return false;
  }

  uint8_t *buffer = malloc(Length + 1);
  if (buffer == NULL)
  {
    TEEIO_ASSERT(false);
    return false;
  }

  memcpy(buffer, Value, Length + 1);
  char *token = strtok((char *)buffer, ",");

  while (token != NULL)
  {
    if (!IsValidDecimalString((uint8_t *)token, strlen(token)))
    {
      valid = false;
      break;
    }
    valid = true;
  }

  free(buffer);

  return valid;
}

/**
  Return if the value string is valid.

  @param[in] Value    The value to be checked.
  @param[in] Length  The length of value string in bytes.

  @retval true   The name string is valid.
  @retval false  The name string is invalid.
**/
bool IsValidValue(
    uint8_t *Value,
    uint32_t Length)
{
  if (IsValidName(Value, Length) || IsValidGuid(Value, Length))
  {
    return true;
  }

  return false;
}

bool IsValidSection(
  void *Context,
  uint8_t *SectionName
)
{
  SECTION_ITEM *Section = NULL;
  INI_PARSING_LIB_CONTEXT *IniContext;

  if(Context == NULL || SectionName == NULL) {
    return false;
  }

  IniContext = Context;
  Section = IniContext->SectionHead;

  while (Section != NULL)
  {
    if (strcmp((const char *)Section->PtrSection, (const char *)SectionName) == 0)
    {
      if (Section->PtrEntry != NULL)
      {
        return true;
      }
    }

    Section = Section->PtrNext;
  }

  return false;
}

/**
  Dump an INI config file context.

  @param[in] Context         INI Config file context.
**/
void DumpIniSection(
    void *Context)
{
  INI_PARSING_LIB_CONTEXT *IniContext;
  SECTION_ITEM *PtrSection;
  SECTION_ITEM *Section;

  if (Context == NULL)
  {
    return;
  }

  IniContext = Context;
  Section = IniContext->SectionHead;

  while (Section != NULL)
  {
    PtrSection = Section;
    Section = Section->PtrNext;
    if (PtrSection->PtrSection != NULL)
    {
      TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "Section - %s\n", PtrSection->PtrSection));
    }

    if (PtrSection->PtrEntry != NULL)
    {
      TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  Entry - %s\n", PtrSection->PtrEntry));
    }

    if (PtrSection->PtrValue != NULL)
    {
      TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  Value - %s\n", PtrSection->PtrValue));
    }
  }
}

/**
  Copy one line data from buffer data to the line buffer.

  @param[in]      Buffer          Buffer data.
  @param[in]      BufferSize      Buffer Size.
  @param[in, out] LineBuffer      Line buffer to store the found line data.
  @param[in, out] LineSize        On input, size of the input line buffer.
                                  On output, size of the actual line buffer.

  @retval EFI_BUFFER_TOO_SMALL  The size of input line buffer is not enough.
  @retval EFI_SUCCESS           Copy line data into the line buffer.

**/
libspdm_return_t
ProfileGetLine(
    uint8_t *Buffer,
    uint32_t BufferSize,
    uint8_t *LineBuffer,
    uint32_t *LineSize)
{
  uint32_t Length;
  uint8_t *PtrBuf;
  uint8_t *PtrEnd;

  PtrBuf = Buffer;
  PtrEnd = Buffer + BufferSize;

  //
  // 0x0D indicates a line break. Otherwise there is no line break
  //
  while (PtrBuf < PtrEnd)
  {
    if ((*PtrBuf == 0x0D) || (*PtrBuf == 0x0A))
    {
      break;
    }

    PtrBuf++;
  }

  if (PtrBuf >= PtrEnd - 1)
  {
    //
    // The buffer ends without any line break
    // or it is the last character of the buffer
    //
    Length = BufferSize;
  }
  else if (*(PtrBuf + 1) == 0x0A)
  {
    //
    // Further check if a 0x0A follows. If yes, count 0xA
    //
    Length = PtrBuf - Buffer + 2;
  }
  else
  {
    Length = PtrBuf - Buffer + 1;
  }

  if (Length > (*LineSize))
  {
    *LineSize = Length;
    return LIBSPDM_STATUS_BUFFER_TOO_SMALL;
  }

  memset(LineBuffer, 0, *LineSize);
  *LineSize = Length;
  memcpy(LineBuffer, Buffer, Length);

  return LIBSPDM_STATUS_SUCCESS;
}

/**
  Trim Buffer by removing all CR, LF, TAB, and SPACE chars in its head and tail.

  @param[in, out] Buffer          On input,  buffer data to be trimmed.
                                  On output, the trimmed buffer.
  @param[in, out] BufferSize      On input,  size of original buffer data.
                                  On output, size of the trimmed buffer.

**/
void ProfileTrim(
    uint8_t *Buffer,
    uint32_t *BufferSize)
{
  uint32_t Length;
  uint8_t *PtrBuf;
  uint8_t *PtrEnd;

  if (*BufferSize == 0)
  {
    return;
  }

  //
  // Trim the tail first, include CR, LF, TAB, and SPACE.
  //
  Length = *BufferSize;
  PtrBuf = Buffer + Length - 1;
  while (PtrBuf >= Buffer)
  {
    if ((*PtrBuf != 0x0D) && (*PtrBuf != 0x0A) && (*PtrBuf != 0x20) && (*PtrBuf != 0x09))
    {
      break;
    }

    PtrBuf--;
  }

  //
  // all spaces, a blank line, return directly;
  //
  if (PtrBuf < Buffer)
  {
    *BufferSize = 0;
    return;
  }

  Length = PtrBuf - Buffer + 1;
  PtrEnd = PtrBuf;
  PtrBuf = Buffer;

  //
  // Now skip the heading CR, LF, TAB and SPACE
  //
  while (PtrBuf <= PtrEnd)
  {
    if ((*PtrBuf != 0x0D) && (*PtrBuf != 0x0A) && (*PtrBuf != 0x20) && (*PtrBuf != 0x09))
    {
      break;
    }

    PtrBuf++;
  }

  //
  // If no heading CR, LF, TAB or SPACE, directly return
  //
  if (PtrBuf == Buffer)
  {
    *BufferSize = Length;
    return;
  }

  *BufferSize = PtrEnd - PtrBuf + 1;

  //
  // The first Buffer..PtrBuf characters are CR, LF, TAB or SPACE.
  // Now move out all these characters.
  //
  while (PtrBuf <= PtrEnd)
  {
    *Buffer = *PtrBuf;
    Buffer++;
    PtrBuf++;
  }

  return;
}

/**
  Insert new comment item into comment head.

  @param[in]      Buffer          Comment buffer to be added.
  @param[in]      BufferSize      Size of comment buffer.
  @param[in, out] CommentHead     Comment Item head entry.

  @retval EFI_OUT_OF_RESOURCES   No enough memory is allocated.
  @retval EFI_SUCCESS            New comment item is inserted.

**/
bool ProfileGetComments(
    uint8_t *Buffer,
    uint32_t BufferSize,
    COMMENT_LINE **CommentHead)
{
  COMMENT_LINE *CommentItem;

  CommentItem = NULL;
  CommentItem = malloc(sizeof(COMMENT_LINE));
  if (CommentItem == NULL)
  {
    TEEIO_ASSERT(false);
    return false;
  }

  CommentItem->PtrNext = *CommentHead;
  *CommentHead = CommentItem;

  //
  // Add a trailing '\0'
  //
  CommentItem->PtrComment = malloc(BufferSize + 1);
  if (CommentItem->PtrComment == NULL)
  {
    free(CommentItem);
    TEEIO_ASSERT(false);
    return false;
  }

  memcpy(CommentItem->PtrComment, Buffer, BufferSize);
  *(CommentItem->PtrComment + BufferSize) = '\0';

  return true;
}

/**
  Add new section item into Section head.

  @param[in]      Buffer          Section item data buffer.
  @param[in]      BufferSize      Size of section item.
  @param[in, out] SectionHead     Section item head entry.

  @retval EFI_OUT_OF_RESOURCES   No enough memory is allocated.
  @retval EFI_SUCCESS            Section item is NULL or Section item is added.

**/
bool ProfileGetSection(
    uint8_t *Buffer,
    uint32_t BufferSize,
    SECTION_ITEM **SectionHead)
{
  SECTION_ITEM *SectionItem;
  uint32_t Length;
  uint8_t *PtrBuf;
  uint8_t *PtrEnd;

  TEEIO_ASSERT(BufferSize >= 1);
  //
  // The first character of Buffer is '[', now we want for ']'
  //
  PtrEnd = Buffer + BufferSize - 1;
  PtrBuf = Buffer + 1;
  while (PtrBuf <= PtrEnd)
  {
    if (*PtrBuf == ']')
    {
      break;
    }

    PtrBuf++;
  }

  if (PtrBuf > PtrEnd)
  {
    //
    // Not found. Invalid line
    //
    return false;
  }

  if (PtrBuf <= Buffer + 1)
  {
    // Empty name
    return false;
  }

  //
  // excluding the heading '[' and tailing ']'
  //
  Length = PtrBuf - Buffer - 1;
  ProfileTrim(
      Buffer + 1,
      &Length);

  //
  // Invalid line if the section name is null
  //
  if (Length == 0)
  {
    return false;
  }

  if (!IsValidName((uint8_t *)Buffer + 1, Length))
  {
    return false;
  }

  SectionItem = malloc(sizeof(SECTION_ITEM));
  if (SectionItem == NULL)
  {
    TEEIO_ASSERT(false);
    return false;
  }

  SectionItem->PtrSection = NULL;
  SectionItem->SecNameLen = Length;
  SectionItem->PtrEntry = NULL;
  SectionItem->PtrValue = NULL;
  SectionItem->PtrNext = *SectionHead;
  *SectionHead = SectionItem;

  //
  // Add a trailing '\0'
  //
  SectionItem->PtrSection = malloc(Length + 1);
  if (SectionItem->PtrSection == NULL)
  {
    TEEIO_ASSERT(false);
    return false;
  }

  //
  // excluding the heading '['
  //
  memcpy(SectionItem->PtrSection, Buffer + 1, Length);
  *(SectionItem->PtrSection + Length) = '\0';

  return true;
}

/**
  Add new section entry and entry value into Section head.

  @param[in]      Buffer          Section entry data buffer.
  @param[in]      BufferSize      Size of section entry.
  @param[in, out] SectionHead     Section item head entry.

  @retval EFI_OUT_OF_RESOURCES   No enough memory is allocated.
  @retval EFI_SUCCESS            Section entry is added.
  @retval EFI_NOT_FOUND          Section entry is not found.
  @retval EFI_INVALID_PARAMETER  Section entry is invalid.

**/
bool ProfileGetEntry(
    uint8_t *Buffer,
    uint32_t BufferSize,
    SECTION_ITEM **SectionHead)
{
  SECTION_ITEM *SectionItem;
  SECTION_ITEM *PtrSection;
  uint32_t Length;
  uint8_t *PtrBuf;
  uint8_t *PtrEnd;

  PtrBuf = Buffer;
  PtrEnd = Buffer + BufferSize - 1;

  //
  // First search for '='
  //
  while (PtrBuf <= PtrEnd)
  {
    if (*PtrBuf == '=')
    {
      break;
    }

    PtrBuf++;
  }

  if (PtrBuf > PtrEnd)
  {
    //
    // Not found. Invalid line
    //
    return false;
  }

  if (PtrBuf <= Buffer)
  {
    // Empty name
    return false;
  }

  //
  // excluding the tailing '='
  //
  Length = PtrBuf - Buffer;
  ProfileTrim(
      Buffer,
      &Length);

  //
  // Invalid line if the entry name is null
  //
  if (Length == 0)
  {
    return false;
  }

  if (!IsValidName((uint8_t *)Buffer, Length))
  {
    return false;
  }

  //
  // Omit this line if no section header has been found before
  //
  if (*SectionHead == NULL)
  {
    return true;
  }

  PtrSection = *SectionHead;

  SectionItem = malloc(sizeof(SECTION_ITEM));
  if (SectionItem == NULL)
  {
    TEEIO_ASSERT(false);
    return false;
  }

  SectionItem->PtrSection = NULL;
  SectionItem->PtrEntry = NULL;
  SectionItem->PtrValue = NULL;
  SectionItem->SecNameLen = PtrSection->SecNameLen;
  SectionItem->PtrNext = *SectionHead;
  *SectionHead = SectionItem;

  //
  // SectionName, add a trailing '\0'
  //
  SectionItem->PtrSection = malloc(PtrSection->SecNameLen + 1);
  if (SectionItem->PtrSection == NULL)
  {
    TEEIO_ASSERT(false);
    return false;
  }

  memcpy(SectionItem->PtrSection, PtrSection->PtrSection, PtrSection->SecNameLen + 1);

  //
  // EntryName, add a trailing '\0'
  //
  SectionItem->PtrEntry = malloc(Length + 1);
  if (SectionItem->PtrEntry == NULL)
  {
    free(SectionItem->PtrSection);
    TEEIO_ASSERT(false);
    return false;
  }

  memcpy(SectionItem->PtrEntry, Buffer, Length);
  *(SectionItem->PtrEntry + Length) = '\0';

  //
  // Next search for '#' or ';'
  //
  PtrBuf = PtrBuf + 1;
  Buffer = PtrBuf;
  while (PtrBuf <= PtrEnd)
  {
    if ((*PtrBuf == '#') || (*PtrBuf == ';'))
    {
      break;
    }

    PtrBuf++;
  }

  if (PtrBuf <= Buffer)
  {
    // Empty name
    free(SectionItem->PtrEntry);
    free(SectionItem->PtrSection);
    return false;
  }

  Length = PtrBuf - Buffer;
  ProfileTrim(
      Buffer,
      &Length);

  //
  // Invalid line if the entry value is null
  //
  if (Length == 0)
  {
    free(SectionItem->PtrEntry);
    free(SectionItem->PtrSection);
    return false;
  }

  if (!IsValidValue((uint8_t *)Buffer, Length))
  {
    free(SectionItem->PtrEntry);
    free(SectionItem->PtrSection);
    return false;
  }

  //
  // EntryValue, add a trailing '\0'
  //
  SectionItem->PtrValue = malloc(Length + 1);
  if (SectionItem->PtrValue == NULL)
  {
    free(SectionItem->PtrEntry);
    free(SectionItem->PtrSection);
    TEEIO_ASSERT(false);
    return false;
  }

  memcpy(SectionItem->PtrValue, Buffer, Length);
  *(SectionItem->PtrValue + Length) = '\0';

  return true;
}

/**
  Free all comment entry and section entry.

  @param[in] Section         Section entry list.
  @param[in] Comment         Comment entry list.

**/
void FreeAllList(
    SECTION_ITEM *Section,
    COMMENT_LINE *Comment)
{
  SECTION_ITEM *PtrSection;
  COMMENT_LINE *PtrComment;

  while (Section != NULL)
  {
    PtrSection = Section;
    Section = Section->PtrNext;
    if (PtrSection->PtrEntry != NULL)
    {
      free(PtrSection->PtrEntry);
    }

    if (PtrSection->PtrSection != NULL)
    {
      free(PtrSection->PtrSection);
    }

    if (PtrSection->PtrValue != NULL)
    {
      free(PtrSection->PtrValue);
    }

    free(PtrSection);
  }

  while (Comment != NULL)
  {
    PtrComment = Comment;
    Comment = Comment->PtrNext;
    if (PtrComment->PtrComment != NULL)
    {
      free(PtrComment->PtrComment);
    }

    free(PtrComment);
  }

  return;
}

/**
  Get section entry value.

  @param[in]  Section         Section entry list.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] EntryValue      Point to the got entry value.

  @retval EFI_NOT_FOUND  Section is not found.
  @retval EFI_SUCCESS    Section entry value is got.

**/
bool UpdateGetProfileString(
    SECTION_ITEM *Section,
    uint8_t *SectionName,
    uint8_t *EntryName,
    uint8_t **EntryValue)
{
  *EntryValue = NULL;

  while (Section != NULL)
  {
    if (strcmp((const char *)Section->PtrSection, (const char *)SectionName) == 0)
    {
      if (Section->PtrEntry != NULL)
      {
        if (strcmp((const char *)Section->PtrEntry, (const char *)EntryName) == 0)
        {
          break;
        }
      }
    }

    Section = Section->PtrNext;
  }

  if (Section == NULL)
  {
    return false;
  }

  *EntryValue = Section->PtrValue;

  return true;
}

/**
  Pre process config data buffer into Section entry list and Comment entry list.

  @param[in]      DataBuffer      Config raw file buffer.
  @param[in]      BufferSize      Size of raw buffer.
  @param[in, out] SectionHead     Pointer to the section entry list.
  @param[in, out] CommentHead     Pointer to the comment entry list.

  @retval EFI_OUT_OF_RESOURCES  No enough memory is allocated.
  @retval EFI_SUCCESS           Config data buffer is preprocessed.
  @retval EFI_NOT_FOUND         Config data buffer is invalid, because Section or Entry is not found.
  @retval EFI_INVALID_PARAMETER Config data buffer is invalid, because Section or Entry is invalid.

**/
bool PreProcessDataFile(
    uint8_t *DataBuffer,
    uint32_t BufferSize,
    SECTION_ITEM **SectionHead,
    COMMENT_LINE **CommentHead)
{
  uint8_t *Source;
  uint8_t *CurrentPtr;
  uint8_t *BufferEnd;
  uint8_t *PtrLine;
  uint32_t LineLength;
  uint32_t SourceLength;
  uint32_t MaxLineLength;
  libspdm_return_t Status;

  *SectionHead = NULL;
  *CommentHead = NULL;
  BufferEnd = DataBuffer + BufferSize;
  CurrentPtr = DataBuffer;
  MaxLineLength = MAX_LINE_LENGTH;
  Status = LIBSPDM_STATUS_SUCCESS;

  PtrLine = malloc(MaxLineLength);
  if (PtrLine == NULL)
  {
    TEEIO_ASSERT(false);
    return false;
  }

  while (CurrentPtr < BufferEnd)
  {
    Source = CurrentPtr;
    SourceLength = BufferEnd - CurrentPtr;
    LineLength = MaxLineLength;
    //
    // With the assumption that line length is less than 512
    // characters. Otherwise BUFFER_TOO_SMALL will be returned.
    //
    Status = ProfileGetLine(
        Source,
        SourceLength,
        PtrLine,
        &LineLength);
    if (LIBSPDM_STATUS_IS_ERROR(Status))
    {
      if (Status == LIBSPDM_STATUS_BUFFER_TOO_SMALL)
      {
        //
        // If buffer too small, re-allocate the buffer according
        // to the returned LineLength and try again.
        //
        free(PtrLine);
        PtrLine = NULL;
        PtrLine = malloc(LineLength);
        if (PtrLine == NULL)
        {
          TEEIO_ASSERT(false);
          return false;
        }

        SourceLength = LineLength;
        Status = ProfileGetLine(
            (uint8_t *)Source,
            SourceLength,
            (uint8_t *)PtrLine,
            &LineLength);
        if (LIBSPDM_STATUS_IS_ERROR(Status))
        {
          break;
        }

        MaxLineLength = LineLength;
      }
      else
      {
        break;
      }
    }

    CurrentPtr = CurrentPtr + LineLength;

    //
    // Line got. Trim the line before processing it.
    //
    ProfileTrim(
        (uint8_t *)PtrLine,
        &LineLength);

    //
    // Blank line
    //
    if (LineLength == 0)
    {
      continue;
    }

    if ((PtrLine[0] == '#') || (PtrLine[0] == ';'))
    {
      Status = ProfileGetComments(
          (uint8_t *)PtrLine,
          LineLength,
          CommentHead);
    }
    else if (PtrLine[0] == '[')
    {
      Status = ProfileGetSection(
          (uint8_t *)PtrLine,
          LineLength,
          SectionHead);
    }
    else
    {
      Status = ProfileGetEntry(
          (uint8_t *)PtrLine,
          LineLength,
          SectionHead);
    }

    if (LIBSPDM_STATUS_IS_ERROR(Status))
    {
      break;
    }
  }

  //
  // Free buffer
  //
  free(PtrLine);

  return Status;
}

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
    uint32_t BufferSize)
{
  libspdm_return_t Status;
  INI_PARSING_LIB_CONTEXT *IniContext;

  if ((DataBuffer == NULL) || (BufferSize == 0))
  {
    return NULL;
  }

  IniContext = malloc(sizeof(INI_PARSING_LIB_CONTEXT));
  if (IniContext == NULL)
  {
    TEEIO_ASSERT(false);
    return NULL;
  }

  memset(IniContext, 0, sizeof(INI_PARSING_LIB_CONTEXT));

  //
  // First process the data buffer and get all sections and entries
  //
  Status = PreProcessDataFile(
      DataBuffer,
      BufferSize,
      &IniContext->SectionHead,
      &IniContext->CommentHead);
  if (LIBSPDM_STATUS_IS_ERROR(Status))
  {
    free(IniContext);
    return NULL;
  }

  // DumpIniSection(IniContext);

  return IniContext;
}

/**
  Get section entry string value.

  @param[in]  Context         INI Config file context.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] EntryValue      Point to the got entry string value.

  @retval EFI_SUCCESS    Section entry string value is got.
  @retval EFI_NOT_FOUND  Section is not found.
**/
bool GetStringFromDataFile(
    void *Context,
    uint8_t *SectionName,
    uint8_t *EntryName,
    uint8_t **EntryValue)
{
  INI_PARSING_LIB_CONTEXT *IniContext;

  if ((Context == NULL) || (SectionName == NULL) || (EntryName == NULL) || (EntryValue == NULL))
  {
    return false;
  }

  IniContext = Context;

  *EntryValue = NULL;
  return UpdateGetProfileString(
      IniContext->SectionHead,
      SectionName,
      EntryName,
      EntryValue);
}

/**
  Get section entry decimal uint32_t value.

  @param[in]  Context         INI Config file context.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] Data            Point to the got decimal uint32_t value.

  @retval EFI_SUCCESS    Section entry decimal uint32_t value is got.
  @retval EFI_NOT_FOUND  Section is not found.
**/
bool GetDecimalUint32FromDataFile(
    void *Context,
    uint8_t *SectionName,
    uint8_t *EntryName,
    uint32_t *Data)
{
  uint8_t *Value;
  unsigned long result = 0;
  char *end_ptr = NULL;

  if ((Context == NULL) || (SectionName == NULL) || (EntryName == NULL) || (Data == NULL))
  {
    return false;
  }

  if (!GetStringFromDataFile(
          Context,
          SectionName,
          EntryName,
          &Value))
  {
    return false;
  }

  TEEIO_ASSERT(Value != NULL);
  if (!IsValidDecimalString(Value, strlen((const char *)Value)))
  {
    return false;
  }

  result = strtoul((const char *)Value, &end_ptr, 10);
  if (*end_ptr != '\0')
  {
    return false;
  }
  if (result > UINT32_MAX)
  {
    return false;
  }
  *Data = (uint32_t)result;

  return true;
}

/**
  Get section entry hexadecimal uint32_t value.

  @param[in]  Context         INI Config file context.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] Data            Point to the got hexadecimal uint32_t value.

  @retval EFI_SUCCESS    Section entry hexadecimal uint32_t value is got.
  @retval EFI_NOT_FOUND  Section is not found.
**/
bool GetHexUint32FromDataFile(
    void *Context,
    uint8_t *SectionName,
    uint8_t *EntryName,
    uint32_t *Data)
{
  uint8_t *Value;
  unsigned long result = 0;
  char *end_ptr = NULL;

  if ((Context == NULL) || (SectionName == NULL) || (EntryName == NULL) || (Data == NULL))
  {
    return false;
  }

  if (!GetStringFromDataFile(Context, SectionName, EntryName, &Value))
  {
    return false;
  }

  TEEIO_ASSERT(Value != NULL);
  if (!IsValidHexString(Value, strlen((const char *)Value)))
  {
    return false;
  }

  result = strtoul((const char *)Value, &end_ptr, 0);
  if (*end_ptr != '\0')
  {
    return false;
  }
  if (result > UINT32_MAX)
  {
    return false;
  }
  *Data = (uint32_t)result;

  return true;
}

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
bool GetUint32ArrayFromDataFile(
    void *Context,
    uint8_t *SectionName,
    uint8_t *EntryName,
    uint32_t *DataArray,
    uint32_t DataArrayLength)
{
  uint8_t *value = NULL;

  if (Context == NULL || SectionName == NULL || EntryName == NULL || DataArray == NULL || DataArrayLength == 0)
  {
    return false;
  }

  if (!GetStringFromDataFile(Context, SectionName, EntryName, &value))
  {
    return false;
  }

  TEEIO_ASSERT(value != NULL);

  return get_uint32_array_from_string(DataArray, &DataArrayLength, (const char*) value);
}

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
bool GetUint32ArrayLengthFromDataFile(
    void *Context,
    uint8_t *SectionName,
    uint8_t *EntryName,
    uint32_t *DataArrayLength)
{
  uint8_t *value = NULL;

  if (Context == NULL || SectionName == NULL || EntryName == NULL || DataArrayLength == NULL)
  {
    return false;
  }

  if (!GetStringFromDataFile(Context, SectionName, EntryName, &value))
  {
    return false;
  }

  TEEIO_ASSERT(value != NULL);

  return get_uint32_array_from_string(NULL, DataArrayLength, (const char*)value);
}

/**
  Close an INI config file and free the context.

  @param[in] Context         INI Config file context.
**/
void CloseIniFile(
    void *Context)
{
  INI_PARSING_LIB_CONTEXT *IniContext;

  if (Context == NULL)
  {
    return;
  }

  IniContext = Context;
  FreeAllList(IniContext->SectionHead, IniContext->CommentHead);

  return;
}

bool ParseTestSuiteCaseEntry(void *context, uint8_t *section_name, uint8_t *entry_name, uint32_t *cases_id, uint32_t *cases_cnt, uint32_t max_case_id)
{
  uint32_t array_size = 0;

  if (context == NULL || section_name == NULL || entry_name == NULL || cases_id == NULL || cases_cnt == NULL)
  {
    return false;
  }

  if (!GetUint32ArrayLengthFromDataFile(context, section_name, entry_name, &array_size))
  {
    return false;
  }
  if (array_size > *cases_cnt)
  {
    return false;
  }

  if (!GetUint32ArrayFromDataFile(context, section_name, entry_name, cases_id, array_size))
  {
    return false;
  }

  for (int i = 0; i < array_size; i++)
  {
    if (cases_id[i] > max_case_id)
    {
      return false;
    }
  }

  *cases_cnt = array_size;
  return true;
}

bool ParseTestSuiteSection(void *context, IDE_TEST_CONFIG *test_config, int index)
{
  char entry_name[MAX_ENTRY_NAME_LENGTH] = {0};
  uint8_t section_name[MAX_SECTION_NAME_LENGTH] = {0};
  uint8_t *entry_value = NULL;
  uint32_t data32 = 0;
  IDE_TEST_SUITE test_suite = {0};
  uint32_t cases_id[MAX_CASE_ID] = {0};
  uint32_t cases_cnt = 0;

  if (index <= 0)
  {
    return false;
  }

  sprintf((char *)section_name, "TestSuite_%d", index);

  if(!IsValidSection(context, (uint8_t *)section_name)) {
    return false;
  }

  // enabled
  test_suite.enabled = true;
  sprintf(entry_name, "enabled");
  if (GetDecimalUint32FromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &data32))
  {
    test_suite.enabled = data32 == 1;
  }

  sprintf(entry_name, "category");
  if (GetStringFromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &entry_value))
  {
    if (!is_valid_test_category(entry_value))
    {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] Invalid test_category. %s\n", section_name, entry_value));
      return false;
    }
    test_suite.test_category = get_test_category_from_name((const char*)entry_value);
  }
  else
  {
    test_suite.test_category = TEEIO_TEST_CATEGORY_PCIE_IDE;
  }

  if(test_suite.test_category == TEEIO_TEST_CATEGORY_PCIE_IDE ||
    test_suite.test_category == TEEIO_TEST_CATEGORY_TDISP) {
    // type
    sprintf(entry_name, "type");
    if (!GetStringFromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &entry_value))
    {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] type is missing.\n", section_name));
      return false;
    }
    if (!is_valid_topology_type(entry_value))
    {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] Invalid type. %s\n", section_name, entry_value));
      return false;
    }
    test_suite.type = get_topology_type_from_name(entry_value);
  } else {
    // Other test category either not care the type or use fixed LINK_IDE (like CXL).
    test_suite.type = IDE_TEST_TOPOLOGY_TYPE_LINK_IDE;
  }

  // topology
  sprintf(entry_name, "topology");
  if (!GetDecimalUint32FromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &data32))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] topology is missing.\n", section_name));
    return false;
  }
  IDE_TEST_TOPOLOGY *top = get_topology_by_id(test_config, data32);
  if (top == NULL)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] topology_%d is not valid.\n", section_name, data32));
    return false;
  }
  if (top->type != test_suite.type)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] topology_%d type doesnot match test_suite.type\n", section_name, data32));
    return false;
  }
  test_suite.topology_id = data32;

  // configuration
  sprintf(entry_name, "configuration");
  if (!GetDecimalUint32FromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &data32))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] configuration is missing.\n", section_name));
    return false;
  }
  IDE_TEST_CONFIGURATION *config = get_configuration_by_id(test_config, data32);
  if (config == NULL)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] configuratioin_%d is not valid.\n", section_name, data32));
    return false;
  }
  if (config->type != test_suite.type)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] configuration_%d type doesnot match test_suite.type\n", section_name, data32));
    return false;
  }
  if(config->test_category != test_suite.test_category) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] configuration_%d category doesnot match test_suite.category\n", section_name, data32));
    return false;
  }
  test_suite.configuration_id = data32;

  ide_test_case_name_t* test_case;
  for(int i = 0; ; i++) {
    test_case = get_test_case_name(i, test_suite.test_category);
    if(test_case->class == NULL) {
      break;
    }
    cases_cnt = 0;
    if(!get_uint32_array_from_string(NULL, &cases_cnt, test_case->names)) {
      continue;
    }
    uint32_t* u32_array = (uint32_t *)malloc(cases_cnt * sizeof(uint32_t));
    get_uint32_array_from_string(u32_array, &cases_cnt, test_case->names);
    uint32_t max_case_id = get_max_from_uint32_array(u32_array, cases_cnt);

    memset(cases_id, 0, sizeof(cases_id));
    sprintf(entry_name, "%s", test_case->class);

    if (!ParseTestSuiteCaseEntry(context, (uint8_t *)section_name, (uint8_t *)entry_name, cases_id, &cases_cnt, max_case_id))
    {
      TEEIO_DEBUG((TEEIO_DEBUG_INFO, "[%s] [%s] not found.\n", section_name, entry_name));
      cases_cnt = 0;
    }
    for (int j = 0; j < cases_cnt; j++)
    {
      test_suite.test_cases.cases[i].cases_id[j] = cases_id[j];
    }
    test_suite.test_cases.cases[i].cases_cnt = cases_cnt;

    free(u32_array);
  }

  // id
  test_suite.id = index;

  IDE_TEST_SUITE *ts = test_config->test_suites.test_suites + index - 1;
  ts->id = test_suite.id;
  ts->enabled = test_suite.enabled;
  ts->type = test_suite.type;
  ts->topology_id = test_suite.topology_id;
  ts->configuration_id = test_suite.configuration_id;
  ts->test_category = test_suite.test_category;

  IDE_TEST_CASES *tc = &ts->test_cases;

  for(int i = 0; i < MAX_TEST_CASE_NUM; i++) {
    tc->cases[i].cases_cnt = test_suite.test_cases.cases[i].cases_cnt;
    for(int j = 0; j < MAX_CASE_ID; j++) {
      tc->cases[i].cases_id[j] = test_suite.test_cases.cases[i].cases_id[j];
    }
  }

  test_config->test_suites.cnt += 1;

  return true;
}

bool ParseConfigurationSection(void *context, IDE_TEST_CONFIG *test_config, int index)
{
  char entry_name[MAX_ENTRY_NAME_LENGTH] = {0};
  char section_name[MAX_SECTION_NAME_LENGTH] = {0};
  uint8_t *entry_value = NULL;
  uint32_t data32 = 0;
  IDE_TEST_CONFIGURATION config = {0};
  int i = 0;

  if (index <= 0)
  {
    return false;
  }

  sprintf(section_name, "Configuration_%d", index);

  if(!IsValidSection(context, (uint8_t *)section_name)) {
    return false;
  }

  // enabled
  config.enabled = true;
  sprintf(entry_name, "enabled");
  if (GetDecimalUint32FromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &data32))
  {
    config.enabled = data32 == 1;
  }

  sprintf(entry_name, "category");
  if (GetStringFromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &entry_value))
  {
    if (!is_valid_test_category(entry_value))
    {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] Invalid test_category. %s\n", section_name, entry_value));
      return false;
    }
    config.test_category = get_test_category_from_name((const char*)entry_value);
  }
  else
  {
    config.test_category = TEEIO_TEST_CATEGORY_PCIE_IDE;
  }

  // type
  if(config.test_category == TEEIO_TEST_CATEGORY_PCIE_IDE ||
    config.test_category == TEEIO_TEST_CATEGORY_TDISP) {
    sprintf(entry_name, "type");
    if (!GetStringFromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &entry_value))
    {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] type is missing.\n", section_name));
      return false;
    }
    if (!is_valid_topology_type(entry_value))
    {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] Invalid type. %s\n", section_name, *entry_value));
      return false;
    }
    config.type = get_topology_type_from_name(entry_value);
  } else {
    // Other test category either not care the type or use fixed LINK_IDE (like CXL).
    config.type = IDE_TEST_TOPOLOGY_TYPE_LINK_IDE;
  }

  const char** configuration_priv_names = get_test_configuration_priv_names(config.test_category);
  if(configuration_priv_names != NULL) {
    while(configuration_priv_names[i]) {
      sprintf(entry_name, "%s", configuration_priv_names[i]);
      if (GetStringFromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &entry_value)) {
        parse_test_configuration_priv_names(entry_name, (const char*)entry_value, config.test_category, &config);
      }
      i++;
    }
  }

  const char* configuration_name;
  while(true) {
    configuration_name = get_test_configuration_name(i, config.test_category);
    if(configuration_name == NULL) {
      break;
    }
    sprintf(entry_name, "%s", configuration_name);
    if (GetDecimalUint32FromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &data32))
    {
      if(data32 == 1) {
        config.bit_map |= (uint32_t)(1<<i);
      }
    }
    i++;
  }
  if(!teeio_check_configuration_bitmap(&config.bit_map, config.test_category)) {
    return false;
  }

  // id
  config.id = index;
  for (int i = 0; i < MAX_CONFIGURATION_NUM; i++)
  {
    if (test_config->configurations.configurations[i].id == index)
    {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] Duplicated configuration.\n", section_name));
      return false;
    }
  }

  IDE_TEST_CONFIGURATION *cfg = test_config->configurations.configurations + (index - 1);
  cfg->id = config.id;
  cfg->enabled = config.enabled;
  cfg->type = config.type;
  cfg->bit_map = config.bit_map;
  cfg->test_category = config.test_category;
  memcpy(&cfg->priv_data, &config.priv_data, sizeof(config.priv_data));

  test_config->configurations.cnt += 1;

  return true;
}

bool ParseSwitchesSection(void *context, IDE_TEST_CONFIG *test_config, int index)
{
  char entry_name[MAX_ENTRY_NAME_LENGTH] = {0};
  char section_name[MAX_SECTION_NAME_LENGTH] = {0};
  uint8_t *entry_value = NULL;
  uint32_t data32 = 0;
  IDE_PORT *port = NULL;

  if (index <= 0) {
    return false;
  }

  sprintf(section_name, "Switch_%d", index);

  if(!IsValidSection(context, (uint8_t *)section_name)) {
    return false;
  }

  IDE_SWITCH *sw = test_config->switches_config.switches + test_config->switches_config.cnt;

  // enabled
  sw->enabled = true;
  sprintf(entry_name, "enabled");
  if (GetDecimalUint32FromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &data32))
  {
    sw->enabled = data32 == 1;
  }

  // port_x
  for(int i = 0; i < MAX_SUPPORTED_SWITCH_PORTS_NUM; i++) {
    sprintf(entry_name, "port_%d", i + 1);
    if (!GetStringFromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &entry_value)) {
      continue;
    }

    if (!is_valid_dev_func(entry_value)) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] Invalid %s df(%s).\n", section_name, entry_name, entry_value));
      TEEIO_ASSERT(false);
      return false;
    }

    port = sw->ports + sw->ports_cnt;

    port->enabled = true;
    sprintf(port->bdf, "0000:00:%s", entry_value);
    parse_bdf_string((uint8_t *)port->bdf, &port->segment, &port->bus, &port->device, &port->function);
    strncpy(port->port_name, entry_name, PORT_NAME_LENGTH);
    port->port_type = IDE_PORT_TYPE_SWITCH;
    port->id = i + 1;
    sw->ports_cnt += 1;
  }

  sw->id = index;
  sprintf(sw->name, "switch_%d", index);

  test_config->switches_config.cnt += 1;

  return true;
}

bool ParseSwitchString(IDE_TEST_CONFIG *test_config, IDE_SWITCH_INTERNAL_CONNECTION* conn, char* switch_str)
{
  //switch_1:port_1-port_2
  char *str = switch_str;
  int pos = find_char_in_str(str, ':');
  if(pos == -1) {
    return false;
  }
  str[pos] = '\0';

  // check switch_id is valid
  IDE_SWITCH *sw = get_switch_by_name(test_config, str);
  if(sw == NULL) {
    return false;
  }

  str += (pos + 1);
  pos = find_char_in_str(str, '-');
  if(pos == -1) {
    return false;
  }
  str[pos] = '\0';
  char *port1 = str;
  char *port2 = str + pos + 1;

  IDE_PORT* p1 = get_port_from_switch_by_name(sw, port1);
  IDE_PORT* p2 = get_port_from_switch_by_name(sw, port2);
  if(p1 == NULL || p2 == NULL) {
    return false;
  }

  conn->next = NULL;
  conn->switch_id = sw->id;
  conn->ups_port = p1->id;
  conn->dps_port = p2->id;

  return true;
}

IDE_SWITCH_INTERNAL_CONNECTION *ParseSwConnsString(IDE_TEST_CONFIG *test_config, char* sw_conns)
{
  IDE_SWITCH_INTERNAL_CONNECTION *header = NULL;
  IDE_SWITCH_INTERNAL_CONNECTION *itr = NULL;
  bool res = true;
  int pos = 0;
  char str_buf[MAX_LINE_LENGTH] = {0};

  // switch_1:port_1-port_2,switch_2:port_1-port_2,switch_3:port_1-port_2
  char *str_header = sw_conns;
  int size = 0;

  do {
    IDE_SWITCH_INTERNAL_CONNECTION *conn = (IDE_SWITCH_INTERNAL_CONNECTION*)malloc(sizeof(IDE_SWITCH_INTERNAL_CONNECTION));
    TEEIO_ASSERT(conn != NULL);
    memset(conn, 0, sizeof(IDE_SWITCH_INTERNAL_CONNECTION));

    pos = find_char_in_str((const char *)str_header, ',');
    if(pos == -1) {
      size = strlen(str_header);
    } else {
      size = pos;
    }
    memset(str_buf, 0, sizeof(str_buf));
    memcpy(str_buf, str_header, size);

    if(!ParseSwitchString(test_config, conn, str_buf)) {
      res = false;
      break;
    }

    if(pos != -1) {
      str_header += (pos + 1);
    }

    if(header == NULL) {
      header = conn;
    } else {
      itr = header;
      while(itr->next != NULL) {
        itr = itr->next;
      }
      itr->next = conn;
    }
  } while(pos != -1);

  if(res == false && header != NULL) {
    itr = header->next;
    do {
      free(header);
      header = itr;
      itr = itr->next;
    } while (header != NULL);
  }

  return header;
}

// Parse the path string in Topology_x section
// The path string may be one of below formats:
//  rootport_1,endpoint_1
//  rootport_1,switch_1:port_1-port_2,endpoint_1
//  rootport_1,switch_1:port_1-port_2,switch_2:port_1-port_2,endpoint_1
bool ParsePathInTopologySection(void *context, char* section_name, char* entry_name, IDE_TEST_TOPOLOGY* top, bool path1or2, IDE_TEST_CONFIG* test_config)
{
  char str_buf[MAX_LINE_LENGTH] = {0};
  uint8_t *entry_value = NULL;
  int pos = 0;
  char* ptr = NULL;

  if (!GetStringFromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &entry_value))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] %s is missing.\n", section_name, entry_name));
    return false;
  }

  strncpy(str_buf, (char *)entry_value, MAX_LINE_LENGTH);

  // The first shall be rootport_x
  ptr = str_buf;
  pos = find_char_in_str(ptr, ',');
  if(pos == -1 || pos < strlen("rootport_x")) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] %s is invalid(%s).\n", section_name, entry_name, entry_value));
    return false;
  }
  ptr[pos] = '\0';
  IDE_PORT *rootport = get_port_by_name(test_config, ptr);
  if(rootport == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] %s is invalid(%s).\n", section_name, entry_name, entry_value));
    return false;
  }
  top->root_port = rootport->id;

  ptr += (pos + 1);

  // switch_1:port_1-port_2,endpoint_1
  // endpoint_1
  if(top->connection == IDE_TEST_CONNECT_SWITCH || top->connection == IDE_TEST_CONNECT_P2P) {
    pos = revert_find_char_in_str(ptr, ',');
    if(pos == -1) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] %s is invalid(%s).\n", section_name, entry_name, entry_value));
      return false;
    }
    ptr[pos] = '\0';
    IDE_SWITCH_INTERNAL_CONNECTION *conn = ParseSwConnsString(test_config, ptr);
    if(conn == NULL) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] %s is invalid(%s).\n", section_name, entry_name, entry_value));
      return false;
    }
    if(path1or2) {
      top->sw_conn1 = conn;
    } else {
      top->sw_conn2 = conn;
    }
    ptr += (pos + 1);
  }

  // endpoint_1
  if(strlen(ptr) < strlen("endpoint_x")) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] %s is invalid(%s).\n", section_name, entry_name, entry_value));
    return false;
  }
  IDE_PORT *lower_port = get_port_by_name(test_config, ptr);
  if(lower_port == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] %s is invalid(%s).\n", section_name, entry_name, entry_value));
    return false;
  }

  if(top->connection == IDE_TEST_CONNECT_SWITCH || top->connection == IDE_TEST_CONNECT_DIRECT) {
    top->upper_port = rootport->id;
    top->lower_port = lower_port->id;
  } else if(top->connection == IDE_TEST_CONNECT_P2P) {
    if(path1or2) {
      top->upper_port = lower_port->id;
    } else {
      top->lower_port = lower_port->id;
    }
  } else {
    TEEIO_ASSERT(false);
  }

  return true;
}

bool ParseTopologySection(void *context, IDE_TEST_CONFIG *test_config, int index)
{
  char entry_name[MAX_ENTRY_NAME_LENGTH] = {0};
  char section_name[MAX_SECTION_NAME_LENGTH] = {0};
  uint8_t *entry_value = NULL;
  uint32_t data32 = 0;

  if (index <= 0)
  {
    return false;
  }

  sprintf(section_name, "Topology_%d", index);

  if(!IsValidSection(context, (uint8_t *)section_name)) {
    return false;
  }

  IDE_TEST_TOPOLOGY *topology = test_config->topologies.topologies + test_config->topologies.cnt;

  // enabled
  topology->enabled = true;
  sprintf(entry_name, "enabled");
  if (GetDecimalUint32FromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &data32))
  {
    topology->enabled = data32 == 1;
  }

  // type
  sprintf(entry_name, "type");
  if (!GetStringFromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &entry_value))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] type is missing.\n", section_name));
    return false;
  }
  if (!is_valid_topology_type(entry_value))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] Invalid type. %s\n", section_name, entry_value));
    return false;
  }
  topology->type = get_topology_type_from_name(entry_value);

  // connection
  sprintf(entry_name, "connection");
  if (!GetStringFromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &entry_value))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] connection is missing.\n", section_name));
    return false;
  }
  if (!is_valid_topology_connection(entry_value))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] Invalid connection. %s\n", section_name, entry_value));
    return false;
  }
  topology->connection = get_connection_from_name(entry_value);

  // stream_id
  sprintf(entry_name, "stream_id");
  if (!GetDecimalUint32FromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &data32))
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] stream_id is missing.\n", section_name));
  }
  if(data32 > MAX_STREAM_ID) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] stream_id(%d) shall be in [0, 255].\n", section_name, data32));
  }
  topology->stream_id = data32;

  // optional tdisp_function_id
  sprintf(entry_name, "tdisp_function_id");
  if (GetHexUint32FromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &data32))
  {
    // Refer to "TDISP Function ID" definition in PCIE/TDISP Spec.
    // Bit 15: 0  Requester ID
    // Bit 23:16  Requester Segment (Reserved if Requester Segment Valid is Clear)
    // Bit 24:    Requester Segment Valid
    if(data32 > 0x1ffffff) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] tdisp_function_id(%x) is not valid\n", section_name, data32));
      return false;
    }

    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "[%d] tdisp_function_id(%d) is available", section_name, data32));
    g_tdisp_interface_id.function_id = data32;
  }

  // segment
  if(g_scan_segment != INVALID_SCAN_SEGMENT) {
    topology->segment = g_scan_segment;
  } else {
    sprintf(entry_name, "segment");
    if (!GetHexUint32FromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &data32))
    {
      TEEIO_DEBUG((TEEIO_DEBUG_WARN, "[%s] segment is missing, use default segment.\n", section_name));
      topology->segment = 0;
    }
    else {
      if(data32 > 0xffff) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] segment(%x) shall be in [0, 0xffff].\n", section_name, data32));
        return false;
      }
      topology->segment = data32;
    }
  }

  // bus
  if(g_scan_bus != INVALID_SCAN_BUS) {
    topology->bus = g_scan_bus;
  } else {
    sprintf(entry_name, "bus");
    if (!GetHexUint32FromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &data32))
    {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] bus is missing.\n", section_name));
    }
    if(data32 > 0xff) {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] bus(%x) shall be in [0, 0xff].\n", section_name, data32));
    }
    topology->bus = data32;
  }

  // path
  sprintf(entry_name, "path1");
  if(!ParsePathInTopologySection(context, section_name, entry_name, topology, true, test_config)) {
    return false;
  }

  if(topology->connection == IDE_TEST_CONNECT_P2P) {
    sprintf(entry_name, "path2");
    if(!ParsePathInTopologySection(context, section_name, entry_name, topology, false, test_config)) {
      return false;
    }
  }
  if (topology->lower_port == topology->upper_port)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "[%s] upper and lower is same.\n", section_name));
    return false;
  }

  // id
  topology->id = index;

  test_config->topologies.cnt += 1;

  return true;
}

void ParseMainSection(void *context, IDE_TEST_CONFIG *test_config)
{
  char section_name[MAX_SECTION_NAME_LENGTH] = {0};
  char entry_name[MAX_ENTRY_NAME_LENGTH] = {0};
  uint32_t data32 = 0;
  uint8_t *entry_value = NULL;

  sprintf(section_name, "%s", MAIN_SECION);
  sprintf(entry_name, "pci_log");
  if (GetDecimalUint32FromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &data32))
  {
    test_config->main_config.pci_log = data32 == 1;
  }

  sprintf(entry_name, "libspdm_log");
  if (GetDecimalUint32FromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &data32))
  {
    test_config->main_config.libspdm_log = data32 == 1;
  }

  sprintf(entry_name, "doe_log");
  if (GetDecimalUint32FromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &data32))
  {
    test_config->main_config.doe_log = data32 == 1;
  }

  sprintf(entry_name, "debug_level");
  if (!GetStringFromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &entry_value))
  {
    test_config->main_config.debug_level = TEEIO_DEBUG_WARN;
  }
  else
  {
    test_config->main_config.debug_level = get_ide_log_level_from_string((const char*)entry_value);
  }

  sprintf(entry_name, "pcap_enable");
  if (GetDecimalUint32FromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &data32))
  {
    test_config->main_config.pcap_enable = data32 == 1;
  }
}

void ParsePortsSection(void *context, IDE_TEST_CONFIG *test_config, IDE_PORT_TYPE port_type)
{
  char entry_name[MAX_ENTRY_NAME_LENGTH] = {0};
  char section_name[MAX_SECTION_NAME_LENGTH] = {0};
  uint8_t *entry_value = NULL;
  int index = 1;
  IDE_PORT *port = NULL;

  while (true)
  {
    sprintf(entry_name, "%s_%d", IDE_PORT_TYPE_NAMES[port_type], index);
    sprintf(section_name, PORTS_SECTION);
    if (!GetStringFromDataFile(context, (uint8_t *)section_name, (uint8_t *)entry_name, &entry_value))
    {
      break;
    }
    if (!is_valid_dev_func(entry_value))
    {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid %s df. %s\n", IDE_PORT_TYPE_NAMES[port_type], entry_value));
      TEEIO_ASSERT(false);
      break;
    }

    port = test_config->ports_config.ports + test_config->ports_config.cnt;
    port->enabled = true;
    sprintf(port->bdf, "0000:00:%s", entry_value);
    parse_bdf_string((uint8_t *)port->bdf, &port->segment, &port->bus, &port->device, &port->function);
    strncpy(port->port_name, entry_name, PORT_NAME_LENGTH);
    port->port_type = port_type;
    test_config->ports_config.cnt += 1;
    port->id = test_config->ports_config.cnt;

    index++;
  }
}

char *print_array_to_buffer(char *buffer, int buffer_size, uint32_t *array, int array_size)
{
  if(buffer == NULL || array == NULL || buffer_size == 0) {
    TEEIO_ASSERT(false);
    return NULL;
  }

  if(array_size == 0) {
    buffer[0] = '\0';
    return buffer;
  }

  int offset = 0;
  char temp[16] = {0};

  for(int i = 0; i < array_size; i++) {
    sprintf(temp, "%d", array[i]);
    if(strlen(temp) + 1 + offset > buffer_size) {
      TEEIO_ASSERT(false);
      return NULL;
    }
    sprintf(buffer + offset, "%s,", temp);
    offset = strlen(buffer);
    memset(temp, 0, sizeof(temp));
  }

  buffer[offset - 1] = '\0';

  return buffer;
}

char *print_switch_conn_to_string(char* buffer, int buffer_size, IDE_TEST_CONFIG *test_config, IDE_SWITCH_INTERNAL_CONNECTION *switch_conn)
{
  if(switch_conn == NULL) {
    return buffer;
  }

  char* ptr = buffer;

  IDE_SWITCH_INTERNAL_CONNECTION *itr = switch_conn;
  do {
    IDE_SWITCH *sw = get_switch_by_id(test_config, itr->switch_id);
    TEEIO_ASSERT(sw != NULL);

    IDE_PORT *port1 = get_port_from_switch_by_id(sw, itr->ups_port);
    IDE_PORT *port2 = get_port_from_switch_by_id(sw, itr->dps_port);
    TEEIO_ASSERT(port1);
    TEEIO_ASSERT(port2);

    sprintf(ptr, "%s:%s-%s,", sw->name, port1->port_name, port2->port_name);
    ptr += strlen(ptr);

    itr = itr->next;
  } while(itr);

  ptr[strlen(ptr) - 1] = '\0';

  return buffer;
}

void dump_test_config(IDE_TEST_CONFIG *test_config) 
{
  if(test_config == NULL) {
    return;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "Dump test_ini:\n"));

  IDE_TEST_MAIN_CONFIG *main_config = &test_config->main_config;
  TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "[Main]\n"));
  TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  pci_log=%s\n", main_config->pci_log == 0 ? "false":"true"));
  TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  debug_level=%s\n", get_ide_log_level_string(main_config->debug_level)));
  TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  libspdm_log=%s\n", main_config->libspdm_log == 0 ? "false":"true"));
  TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  doe_log=%s\n", main_config->doe_log == 0 ? "false":"true"));
  TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  pcap_enable=%s\n", main_config->pcap_enable == 0 ? "false":"true"));
  TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "\n"));

  IDE_TEST_PORTS_CONFIG *ports = &test_config->ports_config;
  for(int i = 0; i < ports->cnt; i++) {
    IDE_PORT *port = ports->ports + i;
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "[Port_%d]\n", port->id));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  id      = %d\n", port->id));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  enabled = %d\n", port->enabled));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  bdf     = %s\n", port->bdf));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  name    = %s\n", port->port_name));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  type    = %s\n", IDE_PORT_TYPE_NAMES[port->port_type]));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "\n"));
  }

  IDE_TEST_SWITCHES_CONFIG *sws = &test_config->switches_config;
  for(int i = 0; i < sws->cnt; i++) {
    IDE_SWITCH *sw = sws->switches + i;
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "[Switch_%d]\n", sw->id));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  id      = %d\n", sw->id));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  name    = %s\n", sw->name));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  enabled = %d\n", sw->enabled));
    for(int j = 0; j < sw->ports_cnt; j++) {
      IDE_PORT *port = sw->ports + j;
      TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  %s  = %s\n", port->port_name, port->bdf + 3));
    }
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "\n"));
  }

  IDE_TEST_TOPOLOGYS *tops = &test_config->topologies;
  for(int i = 0; i < MAX_TOPOLOGY_NUM; i++) {
    IDE_TEST_TOPOLOGY *top = tops->topologies + i;
    if(top->id == 0) {
      continue;
    }
    char buffer[MAX_LINE_LENGTH] = {0};
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "[Topology_%d]\n", top->id));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  id        = %d\n", top->id));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  enabled   = %d\n", top->enabled));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  type      = %s\n", IDE_TEST_TOPOLOGY_TYPE_NAMES[top->type]));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  connection= %s\n", IDE_TEST_CONNECT_TYPE_NAMES[top->connection]));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  bus       = 0x%02x\n", top->bus));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  rootport  = %s\n", get_port_by_id(test_config, top->root_port)->port_name));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  upper     = %s\n", get_port_by_id(test_config, top->upper_port)->port_name));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  lower     = %s\n", get_port_by_id(test_config, top->lower_port)->port_name));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  stream_id = %d\n", top->stream_id));
    if(top->connection == IDE_TEST_CONNECT_DIRECT) {
      TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  path1     = %s,%s\n", get_port_by_id(test_config, top->root_port)->port_name,
                                                                     get_port_by_id(test_config, top->lower_port)->port_name));
    } else if(top->connection == IDE_TEST_CONNECT_SWITCH) {
      TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  path1     = %s,%s,%s\n", get_port_by_id(test_config, top->root_port)->port_name,
                                                                     print_switch_conn_to_string(buffer, MAX_LINE_LENGTH, test_config, top->sw_conn1),
                                                                     get_port_by_id(test_config, top->lower_port)->port_name));
    } else if(top->connection == IDE_TEST_CONNECT_P2P) {
      TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  path1     = %s,%s,%s\n", get_port_by_id(test_config, top->root_port)->port_name,
                                                                     print_switch_conn_to_string(buffer, MAX_LINE_LENGTH, test_config, top->sw_conn1),
                                                                     get_port_by_id(test_config, top->upper_port)->port_name));
      TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  path2     = %s,%s,%s\n", get_port_by_id(test_config, top->root_port)->port_name,
                                                                     print_switch_conn_to_string(buffer, MAX_LINE_LENGTH, test_config, top->sw_conn2),
                                                                     get_port_by_id(test_config, top->lower_port)->port_name));
    }

    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "\n"));
  }

  IDE_TEST_CONFIGURATIONS *cfgs = &test_config->configurations;
  for(int i = 0; i < MAX_CONFIGURATION_NUM; i++) {
    IDE_TEST_CONFIGURATION *cfg = cfgs->configurations + i;
    if(cfg->id == 0) {
      continue;
    }
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "[Configuration_%d]\n", cfg->id));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  id        = %d\n", cfg->id));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  enabled   = %d\n", cfg->enabled));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  type      = %s\n", IDE_TEST_TOPOLOGY_TYPE_NAMES[cfg->type]));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  bit_map   = %x\n", cfg->bit_map));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "\n"));
  }

  IDE_TEST_SUITES *suites = &test_config->test_suites;
  for(int i = 0; i < MAX_TEST_SUITE_NUM; i++) {
    IDE_TEST_SUITE *suite = suites->test_suites + i;
    if(suite->id == 0) {
      continue;
    }
    char buffer[MAX_LINE_LENGTH] = {0};
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "[TestSuite_%d]\n", i + 1));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  id        = %d\n", suite->id));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  enabled   = %d\n", suite->enabled));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  type      = %s\n", IDE_TEST_TOPOLOGY_TYPE_NAMES[suite->type]));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  category  = %s\n", TEEIO_TEST_CATEGORY_NAMES[suite->test_category]));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  top_id    = %d\n", suite->topology_id));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  configuration_id  = [%d]\n", suite->configuration_id));

    IDE_TEST_CASES *cases = &suite->test_cases;


    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  query        = [%s]\n", print_array_to_buffer(buffer, MAX_LINE_LENGTH,
                                          cases->cases[IDE_COMMON_TEST_CASE_QUERY].cases_id, cases->cases[IDE_COMMON_TEST_CASE_QUERY].cases_cnt)));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  keyprog      = [%s]\n", print_array_to_buffer(buffer, MAX_LINE_LENGTH,
                                          cases->cases[IDE_COMMON_TEST_CASE_KEYPROG].cases_id, cases->cases[IDE_COMMON_TEST_CASE_KEYPROG].cases_cnt)));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  ksetgo       = [%s]\n", print_array_to_buffer(buffer, MAX_LINE_LENGTH,
                                          cases->cases[IDE_COMMON_TEST_CASE_KSETGO].cases_id, cases->cases[IDE_COMMON_TEST_CASE_KSETGO].cases_cnt)));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  ksetstop     = [%s]\n", print_array_to_buffer(buffer, MAX_LINE_LENGTH,
                                          cases->cases[IDE_COMMON_TEST_CASE_KSETSTOP].cases_id, cases->cases[IDE_COMMON_TEST_CASE_KSETSTOP].cases_cnt)));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "  spdmsessioin = [%s]\n", print_array_to_buffer(buffer, MAX_LINE_LENGTH,
                                          cases->cases[IDE_COMMON_TEST_CASE_SPDMSESSION].cases_id, cases->cases[IDE_COMMON_TEST_CASE_SPDMSESSION].cases_cnt)));
    TEEIO_DEBUG((TEEIO_DEBUG_VERBOSE, "\n"));
  }

}

// This function will create several TestSuite with below configurations:
//    type=top->type        ; This is test suite for selective_ide
//    topology=@top_id      ; use topology_1. Note: the type shall be matched.
//    configuration=@config ; test configuration_1. Note: the type shall be matched.
//    Full=1                ; setup a ide_stream and wait for input from tester
bool update_test_config_with_given_top_config_id(IDE_TEST_CONFIG *test_config, int top_id, int config_id, const char* test_case, TEEIO_TEST_CATEGORY test_category)
{
  IDE_TEST_CONFIGURATION *config = get_configuration_by_id(test_config, config_id);
  IDE_TEST_TOPOLOGY *top = get_topology_by_id(test_config, top_id);

  if(config == NULL || top == NULL) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid top_id(%d) or config_id(%d)\n", top_id, config_id));
    return false;
  }

  int index = 0;
  ide_test_case_name_t* tcn = get_test_case_from_string(test_case, &index, test_category);
  if(tcn == NULL) {
    return false;
  }

  IDE_TEST_SUITE* suite = NULL;
  // clear the TestSuite
  for(int i = 0; i < test_config->test_suites.cnt; i++) {
    suite = test_config->test_suites.test_suites + i;
    suite->enabled = 0;
    suite->configuration_id = 0;
    suite->id = 0;
    for(int j = 0; j < IDE_COMMON_TEST_CASE_NUM; j++) {
      suite->test_cases.cases[j].cases_cnt = 0;
    }
    suite->type = 0;
  }
  test_config->test_suites.cnt = 1;

  // create TestSuite for full test case
  suite = &test_config->test_suites.test_suites[0];
  suite->configuration_id = config_id;
  suite->enabled = true;
  suite->id = 1;
  suite->topology_id = top_id;
  suite->type = top->type;
  suite->test_category = test_category;

  suite->test_cases.cases[tcn->class_id].cases_cnt = 1;
  suite->test_cases.cases[tcn->class_id].cases_id[0] = index + 1;

  return true;
}

/**
 * parse the ide_test_ini file and populate IDE_TEST_CONFIG
 */
bool parse_ide_test_init(IDE_TEST_CONFIG *test_config, const char *ide_test_ini)
{
  void *context = NULL;
  uint8_t *raw_data = NULL;
  bool ret = false;
  int index = 1;

  if (test_config == NULL || ide_test_ini == NULL)
  {
    TEEIO_ASSERT(false);
    return false;
  }

  // open the file and read the raw data
  int fp = open(ide_test_ini, O_RDONLY);
  if (fp == -1)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Error opening ide_test_ini file. (%s)\n", ide_test_ini));
    return false;
  }

  off_t size = lseek(fp, 0x0, SEEK_END);
  raw_data = (uint8_t *)malloc(size);
  if (raw_data == NULL)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to allocate memory to parse ini file.\n"));
    return false;
  }

  lseek(fp, 0, SEEK_SET);
  size_t bytes_read = read(fp, raw_data, size);
  TEEIO_ASSERT(bytes_read == size);

  context = OpenIniFile(raw_data, bytes_read);
  if (context == NULL)
  {
    TEEIO_ASSERT(false);
    goto ParseDone;
  }

  // [Main]
  ParseMainSection(context, test_config);

  // [Ports]
  // rootport_x and endpoint_x
  ParsePortsSection(context, test_config, IDE_PORT_TYPE_ROOTPORT);
  ParsePortsSection(context, test_config, IDE_PORT_TYPE_ENDPOINT);

  // [Switch_x]
  for(index = 0; index < MAX_SUPPORTED_SWITCHES_NUM; index++) {
    ParseSwitchesSection(context, test_config, index + 1);
  }

  // [Topology_x]
  for (index = 0; index < MAX_TOPOLOGY_NUM; index++)
  {
    ParseTopologySection(context, test_config, index + 1);
  }
  if (test_config->topologies.cnt == 0)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to parse topology.\n"));
    goto ParseDone;
  }

  // [Configuration_x]
  index = 0;
  for (index = 0; index < MAX_CONFIGURATION_NUM; index++)
  {
    ParseConfigurationSection(context, test_config, index + 1);
  }
  if (test_config->configurations.cnt == 0)
  {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to parse configuration.\n"));
    goto ParseDone;
  }

  // [TestSuite_x]
  if(g_run_test_suite)
  {
    index = 0;
    for (index = 0; index < MAX_TEST_SUITE_NUM; index++)
    {
      ParseTestSuiteSection(context, test_config, index + 1);
    }
    if (test_config->test_suites.cnt == 0)
    {
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Failed to parse test_suite.\n"));
      TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "TestSuite_x section is missing or incorrect.\n"));
      goto ParseDone;
    }
  }

  dump_test_config(test_config);

  ret = true;

ParseDone:
  if (context != NULL)
  {
    CloseIniFile(context);
  }

  if (raw_data != NULL)
  {
    free(raw_data);
  }

  close(fp);

  return ret;
}
