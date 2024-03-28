/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __IDE_LOG_H__
#define __IDE_LOG_H__

#include <stdint.h>

typedef enum {
  TEEIO_DEBUG_ERROR = 0,
  TEEIO_DEBUG_WARN,
  TEEIO_DEBUG_INFO,
  TEEIO_DEBUG_VERBOSE,
  TEEIO_DEBUG_NUM
} TEEIO_DEBUG_LEVEL;

void teeio_debug_print(int debug_level, const char *format, ...);
void teeio_print(const char *format, ...);
void teeio_assert(const char *file_name, int line_number,
                                 const char *description);

#define TEEIO_ASSERT_ENABLE

#ifdef TEEIO_ASSERT_ENABLE
#define TEEIO_ASSERT(expression) \
    do { \
        if (!(expression)) { \
            teeio_assert(__FILE__, __LINE__, #expression); \
        } \
    } while (false)
#else
#define TEEIO_ASSERT(expression)
#endif

#define TEEIO_DEBUG(expression) \
    do { \
        TEEIO_DEBUG_INTERNAL(expression); \
    } while (false)

#define TEEIO_DEBUG_PRINT_INTERNAL(debug_level, ...) \
    do { \
        teeio_debug_print(debug_level, ## __VA_ARGS__); \
    } while (false)

#define TEEIO_DEBUG_INTERNAL(expression) TEEIO_DEBUG_PRINT_INTERNAL expression

#define TEEIO_PRINT(expression) \
    do { \
        TEEIO_PRINT_INTERNAL(expression); \
    } while (false)

#define TEEIO_PRINT_INFO_INTERNAL(format, ...) \
    do { \
        teeio_print(format, ## __VA_ARGS__); \
    } while (false)

#define TEEIO_PRINT_INTERNAL(expression) TEEIO_PRINT_INFO_INTERNAL expression


#endif
