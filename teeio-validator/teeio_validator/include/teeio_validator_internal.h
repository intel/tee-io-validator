/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __TEEIO_VAILDATOR_INTERNAL_H__
#define __TEEIO_VAILDATOR_INTERNAL_H__

// ide_test_case_name_t* get_test_case_from_string(const char* test_case_name, int* index, IDE_HW_TYPE ide_type);
bool is_valid_test_case(const char* test_case_name, IDE_HW_TYPE ide_type);

#endif