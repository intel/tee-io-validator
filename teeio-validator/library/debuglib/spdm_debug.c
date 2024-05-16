/**
 *  Copyright Notice:
 *  Copyright 2021-2022 DMTF. All rights reserved.
 *  License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/libspdm/blob/main/LICENSE.md
 **/

#include <base.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "library/debuglib.h"
#include "teeio_debug.h"

extern bool g_libspdm_log;
extern FILE* m_logfile;
extern TEEIO_DEBUG_LEVEL g_debug_level;

#define LIBSPDM_MAX_LOG_MESSAGE_LENGTH  1024

#ifndef LIBSPDM_DEBUG_LEVEL_CONFIG
#define LIBSPDM_DEBUG_LEVEL_CONFIG (LIBSPDM_DEBUG_INFO | LIBSPDM_DEBUG_ERROR)
#endif

void debug_lib_assert(const char* which_assert, const char *file_name, int line_number, const char *description);

void libspdm_debug_assert(const char *file_name, size_t line_number,
                          const char *description)
{
    debug_lib_assert("LIBSPDM_ASSERT", file_name, line_number, description);
}

void libspdm_debug_print(size_t error_level, const char *format, ...)
{
    if(!g_libspdm_log) {
        return;
    }

    if ((error_level & LIBSPDM_DEBUG_LEVEL_CONFIG) == 0) {
        return;
    }

    char buffer[LIBSPDM_MAX_LOG_MESSAGE_LENGTH];
    va_list marker;

    if ((error_level & LIBSPDM_DEBUG_LEVEL_CONFIG) == 0) {
        return;
    }

    va_start(marker, format);

    vsnprintf(buffer, sizeof(buffer), format, marker);

    va_end(marker);

    printf("%s", buffer);

    if(m_logfile) {
        fprintf(m_logfile, "%s", buffer);
        fflush(m_logfile);
    }
}
