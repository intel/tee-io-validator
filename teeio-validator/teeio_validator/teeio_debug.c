/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <stdarg.h>
#include "utils.h"
#include "teeio_debug.h"

#define IDE_MAX_LOG_MESSAGE_LENGTH  1024

extern FILE* m_logfile;
extern TEEIO_DEBUG_LEVEL g_debug_level;

#define TIME_STAMP_LENGTH 64
char m_timestamp[TIME_STAMP_LENGTH];

char * current_time()
{
    char buf[TIME_STAMP_LENGTH/2];
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);

    // Extract milliseconds
    long milliseconds = currentTime.tv_usec / 1000;

    // Convert to local time
    time_t rawtime = currentTime.tv_sec;
    struct tm *localTime = localtime(&rawtime);

    strftime(buf, TIME_STAMP_LENGTH/2, "%Y-%m-%d %H:%M:%S", localTime);
    snprintf(m_timestamp, TIME_STAMP_LENGTH, "%s.%03ld", buf, milliseconds);
    return m_timestamp;
}

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

    char* timestamp = current_time();

    printf("[%s][%s] %s", timestamp, get_ide_log_level_string(debug_level), buffer);

    if(m_logfile) {
        fprintf(m_logfile, "[%s][%s] %s", timestamp, get_ide_log_level_string(debug_level), buffer);
        fflush(m_logfile);
    }
}

void teeio_print(const char *format, ...)
{
    char buffer[IDE_MAX_LOG_MESSAGE_LENGTH] = {0};
    va_list marker;

    va_start(marker, format);

    vsnprintf(buffer, sizeof(buffer), format, marker);

    va_end(marker);

    char* timestamp = current_time();
    printf("[%s] %s", timestamp, buffer);
    if(m_logfile) {
        fprintf(m_logfile, "[%s] %s", timestamp, buffer);
        fflush(m_logfile);
    }
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

    assert(false);
}
#endif /* TEEIO_ASSERT_ENABLE */