/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hal/base.h"
#include "hal/library/debuglib.h"
#include "helperlib.h"
#include "ide_tools.h"
#include "teeio_debug.h"
#include "pcie_ide_lib.h"
#include <industry_standard/pci_tdisp.h>

bool g_pci_log = false;
int g_top_id = 0;
int g_config_id = 0;
IDE_TEST_CONFIG lside_test_config = {0};
DEVCIES_CONTEXT m_devices_context = {0};
IDE_OPERATION g_ide_operation = IDE_OPERATION_CLEAR;
bool g_run_test_suite = false;
pci_tdisp_interface_id_t g_tdisp_interface_id = {0};

TEEIO_DEBUG_LEVEL g_debug_level = TEEIO_DEBUG_WARN;
bool g_libspdm_log = false;
bool g_doe_log = false;
uint16_t g_scan_segment = INVALID_SCAN_SEGMENT;
uint8_t g_scan_bus = INVALID_SCAN_BUS;
FILE* m_logfile = NULL;

void print_usage()
{
    TEEIO_PRINT(( "\n"));
    TEEIO_PRINT(( "Usage:\n"));
    TEEIO_PRINT(( "  setide -f ide_test.ini [-t <top_id>] [-c]\n"));

    TEEIO_PRINT(( "\n"));
    TEEIO_PRINT(( "Options:\n"));
    TEEIO_PRINT(( "  -f <ide_test.ini>   : The file name of test configuration. For example ide_test.ini\n"));
    TEEIO_PRINT(( "  -t <top_id>         : topology id which is to be listed or cleared. For example 1\n"));
    TEEIO_PRINT(( "  -l <debug_level>    : Set debug level. error/warn/info/verbose\n"));
    TEEIO_PRINT(( "  -b <scan_bus>       : Bus number in hex format. For example 0x1a\n"));
    TEEIO_PRINT(( "  -c                  : Clear the ide registers of the devices in top_id\n"));
    TEEIO_PRINT(( "  -h                  : Display this usage\n"));
}

/**
 * parse the command line option
 */
bool parse_cmdline_option(int argc, char *argv[], char *file_name, IDE_TEST_CONFIG *ide_test_config, bool *print_usage)
{
    int opt, v;
    uint8_t data8;

    TEEIO_ASSERT(argc > 0);
    TEEIO_ASSERT(argv != NULL);
    TEEIO_ASSERT(file_name != NULL);
    TEEIO_ASSERT(ide_test_config != NULL);
    TEEIO_ASSERT(print_usage != NULL);

    while ((opt = getopt(argc, argv, "f:t:l:b:ch")) != -1)
    {
        switch (opt)
        {
        case 'f':
            if (!validate_file_name(optarg))
            {
                TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid -f parameter. %s\n", optarg));
                return false;
            }
            sprintf(file_name, "%s", optarg);
            break;

        case 't':
            v = atoi(optarg);
            if (v <= 0 || v > MAX_TOPOLOGY_NUM)
            {
                TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid -t parameter. %s\n", optarg));
                return false;
            }
            g_top_id = v;
            break;

        case 'l':
            g_debug_level = get_ide_log_level_from_string(optarg);
            break;

        case 'b':
            data8 = 0;
            if(convert_hex_str_to_uint8(optarg, &data8)) {
            g_scan_bus = data8;
            } else {
            TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid -b parameter. %s\n", optarg));
            return false;
            }
            break;

        case 'c':
            g_ide_operation = IDE_OPERATION_CLEAR;
            break;

        case 'h':
            *print_usage = true;
            break;

        default:
            return false;
        }
    }

    return true;
}

bool clear_ft_supported_in_ide_ctrl(IDE_PORT* port)
{
    TEEIO_ASSERT(port);

    bool ret = false;
    int fd = open_configuration_space(port->bdf);
    if(fd == -1) {
        return false;
    }

    uint32_t ecap_offset = get_extended_cap_offset(fd, PCI_IDE_EXT_CAPABILITY_ID);

    if (ecap_offset == 0) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "ECAP Offset of %s is NOT found\n", port->bdf));
        goto ClearFtSupportedFailed;
    }

    uint32_t offset = ecap_offset + 8;
    PCIE_IDE_CTRL ide_ctrl = {.raw = device_pci_read_32(offset, fd)};
    ide_ctrl.ft_supported = 0;
    device_pci_write_32(offset, ide_ctrl.raw, fd);

    ret = true;

ClearFtSupportedFailed:
    close(fd);

    return ret;
}


bool clear_ecap(ide_common_test_port_context_t *port_context)
{
    int i = 0;

    // clear ide_ecap_regs
    PCIE_SEL_IDE_STREAM_CTRL stream_ctrl_reg = {.raw = 0};
    PCIE_SEL_IDE_RID_ASSOC_1 rid_assoc_1 = {.raw = 0};
    PCIE_SEL_IDE_RID_ASSOC_2 rid_assoc_2 = {.raw = 0};
    PCIE_SEL_IDE_ADDR_ASSOC_1 addr_assoc_1 = {.raw = 0};
    PCIE_SEL_IDE_ADDR_ASSOC_2 addr_assoc_2 = {.raw = 0};
    PCIE_SEL_IDE_ADDR_ASSOC_3 addr_assoc_3 = {.raw = 0};

    int num_lnk_ide = port_context->ide_cap.lnk_ide_supported == 1 ? port_context->ide_cap.num_lnk_ide + 1 : 0;
    for (i = 0; i < num_lnk_ide; i++)
    {
        TEEIO_PRINT(( "  Clear ide_id %d (LinkIDE)      in IDE Ecap ... ... ", i));
        if (!setup_ide_ecap_regs(
                port_context->cfg_space_fd,
                TEST_IDE_TYPE_LNK_IDE,
                i,
                port_context->ecap_offset,
                stream_ctrl_reg,
                rid_assoc_1, rid_assoc_2,
                addr_assoc_1, addr_assoc_2, addr_assoc_3))
        {
            TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "failed\n"));
        }
        else
        {
            TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "success\n"));
        }
    }

    int num_sel_ide = port_context->ide_cap.sel_ide_supported == 1 ? port_context->ide_cap.num_sel_ide + 1 : 0;
#ifdef NUM_SEL_IDE_ISSUE
    num_sel_ide = num_sel_ide > 4 ? num_sel_ide - 1 : num_sel_ide;
#endif

    for (i = 0; i < num_sel_ide; i++)
    {
        TEEIO_PRINT(( "  Clear ide_id %d (SelectiveIDE) in IDE Ecap ... ... ", i + num_lnk_ide));
        if (!setup_ide_ecap_regs(
                port_context->cfg_space_fd,
                TEST_IDE_TYPE_SEL_IDE,
                i + num_lnk_ide,
                port_context->ecap_offset,
                stream_ctrl_reg,
                rid_assoc_1, rid_assoc_2,
                addr_assoc_1, addr_assoc_2, addr_assoc_3))
        {
            TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "failed\n"));
        }
        else
        {
            TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "success\n"));
        }
    }

    return true;
}

bool clear_kcbar(ide_common_test_port_context_t *port_context)
{
    int i = 0;

    // clear kcbar registers
    for (i = 0; i < port_context->stream_cap.num_stream_supported + 1; i++)
    {
        TEEIO_PRINT(( "  Clear stream_%c in Intel Key Configuration Unit Register Block ... ... ", i + 'a'));
        if (!initialize_kcbar_registers(
                (INTEL_KEYP_ROOT_COMPLEX_KCBAR *)port_context->mapped_kcbar_addr,
                0, i))
        {
            TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "failed\n"));
            return false;
        }
        else
        {
            TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "success\n"));
        }
    }

    return true;
}

bool clear_root_port(ide_common_test_port_context_t *port_context)
{
    TEEIO_PRINT(( "RootPort - %s (%s)\n", port_context->port->port_name, port_context->port->bdf));
    if(!clear_kcbar(port_context)) {
        return false;
    }

    if(!clear_ecap(port_context)) {
        return false;
    }

    TEEIO_PRINT(( "\n"));
    return true;
}

bool clear_dev_port(ide_common_test_port_context_t *port_context)
{
    TEEIO_PRINT(( "Device - %s (%s)\n", port_context->port->port_name, port_context->port->bdf));

    if(!clear_ecap(port_context)) {
        return false;
    }

    TEEIO_PRINT(( "\n"));
    return true;

    return true;
}

bool clear_ft_supported_in_sw_conn(ide_common_test_switch_internal_conn_context_t *sw_conn)
{
    ide_common_test_switch_internal_conn_context_t *conn = sw_conn;
    TEEIO_PRINT(( "Clear ft_supported in Switch_%d\n", sw_conn->switch_id));

    while(conn) {
        TEEIO_PRINT(( "  Clear ft_supported in %s(%s) ... ...", conn->ups.port->port_name, conn->ups.port->bdf));
        if(clear_ft_supported_in_ide_ctrl(conn->ups.port)) {
            TEEIO_PRINT(( "success\n"));
        } else {
            TEEIO_PRINT(( "failed\n"));
        }

        TEEIO_PRINT(( "  Clear ft_supported in %s(%s) ... ...", conn->dps.port->port_name, conn->dps.port->bdf));
        if(clear_ft_supported_in_ide_ctrl(conn->dps.port)) {
            TEEIO_PRINT(( "success\n"));
        } else {
            TEEIO_PRINT(( "failed\n"));
        }

        conn = conn->next;
    }

    return true;
}

bool clear_devices_in_top(IDE_TEST_CONFIG *test_config, int top_id)
{
    // first rootport
    clear_root_port(m_devices_context.root_port_context);

    // then sw_conn1 if exists
    if(m_devices_context.sw_conn1) {
        clear_ft_supported_in_sw_conn(m_devices_context.sw_conn1);
    }

    // then sw_conn2 if exists
    if(m_devices_context.sw_conn2) {
        clear_ft_supported_in_sw_conn(m_devices_context.sw_conn2);
    }

    // then upper_port if it is different from root_port
    if (m_devices_context.root_port_context != m_devices_context.upper_port_context)
    {
        clear_dev_port(m_devices_context.upper_port_context);
    }

    // then lower_port
    clear_dev_port(m_devices_context.lower_port_context);

    return true;
}

int main(int argc, char *argv[])
{
    TEEIO_PRINT(( "%s version %s\n", SET_IDE_NAME, SET_IDE_VERSION));

    char ide_test_ini_file[MAX_FILE_NAME] = {0};
    bool to_print_usage = false;

    // parse command line optioins
    if (!parse_cmdline_option(argc, argv, ide_test_ini_file, &lside_test_config, &to_print_usage))
    {
        print_usage();
        return -1;
    }

    if (to_print_usage)
    {
        print_usage();
        return 0;
    }

    if(ide_test_ini_file[0] == 0) {
        print_usage();
        return 0;
    }

    if (!parse_ide_test_init(&lside_test_config, ide_test_ini_file))
    {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Parse %s failed.\n", ide_test_ini_file));
        return -1;
    }

    if (!scan_open_devices_in_top(&lside_test_config, g_top_id, &m_devices_context))
    {
        return false;
    }

    if (g_ide_operation == IDE_OPERATION_CLEAR)
    {
        clear_devices_in_top(&lside_test_config, g_top_id);
    }

    return 0;
}
