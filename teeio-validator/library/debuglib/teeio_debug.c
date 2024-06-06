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
#include "teeio_debug.h"
#include "helperlib.h"

#define IDE_MAX_LOG_MESSAGE_LENGTH  1024

extern FILE* m_logfile;
extern TEEIO_DEBUG_LEVEL g_debug_level;

const char* m_ide_log_level[] = {
  "error",
  "warn",
  "info",
  "verbose"
};

#define TIME_STAMP_LENGTH 64
char m_timestamp[TIME_STAMP_LENGTH];

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

void debug_lib_assert(const char* which_assert, const char *file_name, int line_number, const char *description)
{
    printf("%s: %s(%d): %s\n", which_assert, file_name, (int32_t)(uint32_t)line_number,
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

void teeio_assert(const char *file_name, int line_number, const char *description)
{
  debug_lib_assert("TEEIO_ASSERT", file_name, line_number, description);
}
#endif /* TEEIO_ASSERT_ENABLE */