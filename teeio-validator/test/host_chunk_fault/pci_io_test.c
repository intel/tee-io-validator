/* Copyright 2026 Intel. SPDX-License-Identifier: BSD-3-Clause */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static jmp_buf failed_transfer;
static ssize_t transfer_result;
static unsigned calls;
static bool writing, interrupted, expect_failure;
static uint32_t test_word = 0xa5b6c7d8;

static ssize_t transfer(int fd, size_t count, off_t offset, bool write_op)
{
    assert(fd == 7 && count == 4 && offset == 0x110 && write_op == writing);
    calls++;
    if (interrupted && calls == 1) {
        errno = EINTR;
        return -1;
    }
    errno = EIO;
    return transfer_result;
}

static ssize_t test_pread(int fd, void *buffer, size_t count, off_t offset)
{
    ssize_t result = transfer(fd, count, offset, false);
    if (result == 4) memcpy(buffer, &test_word, 4);
    return result;
}

static ssize_t test_pwrite(int fd, const void *buffer, size_t count, off_t offset)
{
    assert(memcmp(buffer, &test_word, 4) == 0);
    return transfer(fd, count, offset, true);
}

static _Noreturn void test_exit(int code)
{
    assert(expect_failure && code == EXIT_FAILURE);
    longjmp(failed_transfer, 1);
}

#define pread test_pread
#define pwrite test_pwrite
#define exit test_exit
#include "../../library/helperlib/pcie_helper.c"
#undef exit
#undef pwrite
#undef pread

bool g_pci_log = false;
void teeio_debug_print(int level, const char *format, ...) {}
void teeio_assert(const char *file, int line, const char *description) { abort(); }

static void check(bool write_op, ssize_t result, bool interrupt)
{
    writing = write_op;
    transfer_result = result;
    interrupted = interrupt;
    expect_failure = result != 4;
    calls = 0;
    if (setjmp(failed_transfer) == 0) {
        if (writing) device_pci_write_32(0x110, test_word, 7);
        else assert(device_pci_read_32(0x110, 7) == test_word);
        assert(!expect_failure);
    } else {
        assert(expect_failure);
    }
    assert(calls == (interrupted ? 2 : 1));
}

int main(void)
{
    for (unsigned op = 0; op < 2; op++) {
        for (ssize_t result = -1; result <= 4; result++) check(op, result, false);
        check(op, 4, true);
    }
    puts("PASS: PCI32 read/write success, EINTR, errors and short transfers (14 cases)");
    return 0;
}