/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include "utils.h"
#include "teeio_debug.h"

#define IDE_MAX_LOG_MESSAGE_LENGTH  1024

extern TEEIO_DEBUG_LEVEL g_debug_level;

void teeio_debug_print(int debug_level, const char *format, ...)
{
    char buffer[IDE_MAX_LOG_MESSAGE_LENGTH] = {0};
    va_list marker;

    if (debug_level > g_debug_level) {
        return;
    }

    va_start(marker, format);

    vsnprintf(buffer, sizeof(buffer), format, marker);

    va_end(marker);

    printf("[%s]%s", get_ide_log_level_string(debug_level), buffer);
}

void teeio_print(const char *format, ...)
{
    char buffer[IDE_MAX_LOG_MESSAGE_LENGTH] = {0};
    va_list marker;

    va_start(marker, format);

    vsnprintf(buffer, sizeof(buffer), format, marker);

    va_end(marker);

    printf("%s", buffer);
}


#ifdef TEEIO_ASSERT_ENABLE
#define TEEIO_ASSERT_NATIVE 0
#define TEEIO_ASSERT_DEADLOOP 1
#define TEEIO_ASSERT_BREAKPOINT 2

#ifndef IDE_ASSERT_CONFIG
#define IDE_ASSERT_CONFIG TEEIO_ASSERT_DEADLOOP
#endif

void teeio_assert(const char *file_name, int line_number, const char *description)
{
    printf("TEEIO_ASSERT: %s(%d): %s\n", file_name, (int32_t)(uint32_t)line_number,
           description);

#if (IDE_ASSERT_CONFIG == TEEIO_ASSERT_DEADLOOP)
    {
        volatile int32_t ___i = 1;
        while (___i)
            ;
    }
#elif (IDE_ASSERT_CONFIG == TEEIO_ASSERT_BREAKPOINT)
#if defined(_MSC_EXTENSIONS)
    __debugbreak();
#endif
#if defined(__GNUC__)
    __asm__ __volatile__ ("int $3");
#endif
#endif

    TEEIO_ASSERT(false);
}
#endif /* TEEIO_ASSERT_ENABLE */