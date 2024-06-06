/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __HELPER_INTERNAL__H__
#define __HELPER_INTERNAL_H__

#include <stdint.h>
#include "pcie.h"
#include "intel_keyp.h"
#include "ide_test.h"

typedef struct
{
    int fd;
    char device_name[MAX_NAME_LENGTH];
} IDE_TEST_DEVICES_INFO;

IDE_TEST_DEVICES_INFO *get_device_info_by_fd(int fp);
bool decimal_str_to_array(const char* str, int* array, int size);

#endif