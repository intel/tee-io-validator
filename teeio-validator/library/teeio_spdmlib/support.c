/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/


#include "teeio_validator.h"

void libspdm_dump_hex_str(const uint8_t *buffer, size_t buffer_size)
{
    size_t index;

    for (index = 0; index < buffer_size; index++) {
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%02x", buffer[index]));
    }
}

void dump_data(const uint8_t *buffer, size_t buffer_size)
{
    size_t index;

    for (index = 0; index < buffer_size; index++) {
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%02x ", buffer[index]));
    }
}

void dump_hex(const uint8_t *data, size_t size)
{
    size_t index;
    size_t count;
    size_t left;

#define COLUME_SIZE (16 * 2)

    count = size / COLUME_SIZE;
    left = size % COLUME_SIZE;
    for (index = 0; index < count; index++) {
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%04x: ", (uint32_t)(index * COLUME_SIZE)));
        dump_data(data + index * COLUME_SIZE, COLUME_SIZE);
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "\n"));
    }

    if (left != 0) {
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "%04x: ", (uint32_t)(index * COLUME_SIZE)));
        dump_data(data + index * COLUME_SIZE, left);
        TEEIO_DEBUG((TEEIO_DEBUG_INFO, "\n"));
    }
}

bool libspdm_read_input_file(const char *file_name, void **file_data,
                             size_t *file_size)
{
    FILE *fp_in;
    size_t temp_result;

    if ((fp_in = fopen(file_name, "rb")) == NULL) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Unable to open file %s\n", file_name));
        *file_data = NULL;
        return false;
    }

    fseek(fp_in, 0, SEEK_END);
    *file_size = ftell(fp_in);
    if (*file_size == -1) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Unable to get the file size %s\n", file_name));
        *file_data = NULL;
        fclose(fp_in);
        return false;
    }

    *file_data = (void *)malloc(*file_size);
    if (NULL == *file_data) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "No sufficient memory to allocate %s\n", file_name));
        fclose(fp_in);
        return false;
    }

    fseek(fp_in, 0, SEEK_SET);
    temp_result = fread(*file_data, 1, *file_size, fp_in);
    if (temp_result != *file_size) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Read input file error %s", file_name));
        free((void *)*file_data);
        fclose(fp_in);
        return false;
    }

    fclose(fp_in);

    return true;
}

bool libspdm_write_output_file(const char *file_name, const void *file_data,
                       size_t file_size)
{
    FILE *fp_out;

    if ((fp_out = fopen(file_name, "w+b")) == NULL) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Unable to open file %s\n", file_name));
        return false;
    }

    if (file_size != 0) {
        if ((fwrite(file_data, 1, file_size, fp_out)) != file_size) {
            TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Write output file error %s\n", file_name));
            fclose(fp_out);
            return false;
        }
    }

    fclose(fp_out);

    return true;
}
