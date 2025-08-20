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
#include "teeio_debug.h"
#include "ide_tools.h"
#include "pcie_ide_lib.h"
#include <industry_standard/pci_tdisp.h>

bool g_pci_log = false;
int g_top_id = 0;
int g_config_id = 0;
IDE_OPERATION g_ide_operation = IDE_OPERATION_LIST;
IDE_TEST_CONFIG lside_test_config = {0};
DEVCIES_CONTEXT devices_context = {0};
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
    TEEIO_PRINT(( "  lside -f ide_test.ini [-t <top_id>] [-l]\n"));

    TEEIO_PRINT(( "\n"));
    TEEIO_PRINT(( "Options:\n"));
    TEEIO_PRINT(( "  -f <ide_test.ini>   : The file name of test configuration. For example ide_test.ini\n"));
    TEEIO_PRINT(( "  -t <top_id>         : topology id which is to be listed or cleared. For example 1\n"));
    TEEIO_PRINT(( "  -l <debug_level>    : Set debug level. error/warn/info/verbose\n"));
    TEEIO_PRINT(( "  -b <scan_bus>       : Bus number in hex format. For example 0x1a\n"));
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

    while ((opt = getopt(argc, argv, "f:t:l:b:h")) != -1)
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

        case 'h':
            *print_usage = true;
            break;

        default:
            return false;
        }
    }

    return true;
}

void lside_dump_ecap(int fd, uint32_t ide_ecap_offset)
{
    int i = 0;
    uint32_t offset = ide_ecap_offset;
    TEEIO_PRINT(( "  IDE Extended Cap:\n"));

    // refer to PCIE_IDE_ECAP
    PCIE_CAP_ID cap_id = {.raw = device_pci_read_32(offset, fd)};
    TEEIO_PRINT(( "    cap_id        : %08x\n", cap_id.raw));

    offset += 4;
    PCIE_IDE_CAP ide_cap = {.raw = device_pci_read_32(offset, fd)};
    TEEIO_PRINT(( "    ide_cap       : %08x\n", ide_cap.raw));
    TEEIO_PRINT(( "                  : lnk_ide=%x, sel_ide=%x, ft=%x, aggr=%x, pcrc=%x, alog=%x, sel_ide_cfg_req=%x\n",
                   ide_cap.lnk_ide_supported, ide_cap.sel_ide_supported,
                   ide_cap.ft_supported, ide_cap.aggr_supported,
                   ide_cap.pcrc_supported, ide_cap.supported_algo,
                   ide_cap.sel_ide_cfg_req_supported));
    TEEIO_PRINT(( "                  : num_lnk_ide=%x, num_sel_ide=%x\n",
                   ide_cap.num_lnk_ide, ide_cap.num_sel_ide));

    offset += 4;
    PCIE_IDE_CTRL ide_ctrl = {.raw = device_pci_read_32(offset, fd)};
    TEEIO_PRINT(( "    ide_ctrl      : %08x (ft_supported=%x)\n", ide_ctrl.raw, ide_ctrl.ft_supported));

    TEEIO_PRINT(( "\n"));
    int num_lnk_ide = ide_cap.lnk_ide_supported == 1 ? ide_cap.num_lnk_ide + 1 : 0;
    for (i = 0; i < num_lnk_ide; i++)
    {
        offset += 4;
        PCIE_SEL_IDE_STREAM_CTRL stream_ctrl = {.raw = device_pci_read_32(offset, fd)};
        TEEIO_PRINT(( "    ide_id %d      : LinkIDE\n", i));
        TEEIO_PRINT(( "      stream_ctrl : %08x (enabled=%x, pcrc_en=%x, cfg_sel_ide=%x, stream_id=%x)\n",
                       stream_ctrl.raw, stream_ctrl.enabled, stream_ctrl.pcrc_en, stream_ctrl.cfg_sel_ide, stream_ctrl.stream_id));

        offset += 4;
        PCIE_SEL_IDE_STREAM_STATUS stream_status = {.raw = device_pci_read_32(offset, fd)};
        TEEIO_PRINT(( "      stream_stats: %08x (state=%x, recv_intg_check_fail_msg=%x)\n",
                       stream_status.raw, stream_status.state, stream_status.recv_intg_check_fail_msg));
    }
    TEEIO_PRINT(( "\n"));

    int num_sel_ide = ide_cap.sel_ide_supported == 1 ? ide_cap.num_sel_ide + 1 : 0;
#ifdef NUM_SEL_IDE_ISSUE
    num_sel_ide = num_sel_ide > 4 ? num_sel_ide - 1 : num_sel_ide;
#endif
    for (i = 0; i < num_sel_ide; i++)
    {
        TEEIO_PRINT(( "    ide_id %d      : SelectiveIDE\n", i + num_lnk_ide));

        offset += 4;
        PCIE_SEL_IDE_STREAM_CAP stream_cap = {.raw = device_pci_read_32(offset, fd)};
        TEEIO_PRINT(( "      stream_cap  : %08x (num_addr_assoc_reg_blocks=%d)\n", stream_cap.raw, stream_cap.num_addr_assoc_reg_blocks));

        offset += 4;
        PCIE_SEL_IDE_STREAM_CTRL stream_ctrl = {.raw = device_pci_read_32(offset, fd)};
        TEEIO_PRINT(( "      stream_ctrl : %08x (enabled=%x, pcrc_en=%x, cfg_sel_ide=%x, stream_id=%x)\n",
                       stream_ctrl.raw, stream_ctrl.enabled, stream_ctrl.pcrc_en, stream_ctrl.cfg_sel_ide, stream_ctrl.stream_id));

        offset += 4;
        PCIE_SEL_IDE_STREAM_STATUS stream_status = {.raw = device_pci_read_32(offset, fd)};
        TEEIO_PRINT(( "      stream_stats: %08x (state=%x, recv_intg_check_fail_msg=%x)\n",
                       stream_status.raw, stream_status.state, stream_status.recv_intg_check_fail_msg));
        TEEIO_PRINT(( "\n"));

        offset += (4 * 5);
        // offset += 4;
        // PCIE_SEL_IDE_RID_ASSOC_1 rid_assoc1 = {.raw = device_pci_read_32(offset, fd)};
        // TEEIO_PRINT(( "    rid_assoc1    : %08x\n", rid_assoc1.raw));

        // offset += 4;
        // PCIE_SEL_IDE_RID_ASSOC_2 rid_assoc2 = {.raw = device_pci_read_32(offset, fd)};
        // TEEIO_PRINT(( "    rid_assoc2    : %08x\n", rid_assoc2.raw));

        // offset += 4;
        // PCIE_SEL_IDE_ADDR_ASSOC_1 addr_assoc1 = {.raw = device_pci_read_32(offset, fd)};
        // TEEIO_PRINT(( "    addr_assoc1   : %08x\n", addr_assoc1.raw));

        // offset += 4;
        // PCIE_SEL_IDE_ADDR_ASSOC_2 addr_assoc2 = {.raw = device_pci_read_32(offset, fd)};
        // TEEIO_PRINT(( "    addr_assoc2   : %08x\n", addr_assoc2.raw));

        // offset += 4;
        // PCIE_SEL_IDE_ADDR_ASSOC_3 addr_assoc3 = {.raw = device_pci_read_32(offset, fd)};
        // TEEIO_PRINT(( "    addr_assoc3   : %08x\n", addr_assoc3.raw));
    }
}

void lside_dump_kcbar(INTEL_KEYP_ROOT_COMPLEX_KCBAR *const kcbar_ptr)
{
    int rp_stream_index = 0;

    TEEIO_PRINT(( "  Intel IDE Key Configuration Unit Register Block:\n"));

    INTEL_KEYP_PCIE_STREAM_CAP *stream_cap_ptr = &kcbar_ptr->capabilities;
    INTEL_KEYP_PCIE_STREAM_CAP stream_cap = {.raw = mmio_read_reg32(stream_cap_ptr)};
        TEEIO_PRINT(( "    stream_cap       : %08x (num_stream_supported=%d, num_tx_key_slots=%d, num_rx_key_slots=%d)\n",
                   stream_cap.raw,
                   stream_cap.num_stream_supported,
                   stream_cap.num_tx_key_slots,
                   stream_cap.num_rx_key_slots));

    TEEIO_PRINT(( "\n"));

    for (rp_stream_index = 0; rp_stream_index < stream_cap.num_stream_supported + 1; rp_stream_index++)
    {
        TEEIO_PRINT(( "    stream_%c         :\n", 'a' + rp_stream_index));
        INTEL_KEYP_STREAM_CONFIG_REG_BLOCK *stream_cfg_reg_ptr = (&kcbar_ptr->stream_config_reg_block) + rp_stream_index;

        INTEL_KEYP_STREAM_CONTROL stream_control = {.raw = mmio_read_reg32(&stream_cfg_reg_ptr->control)};
        TEEIO_PRINT(( "      stream_control : %08x (enable=%x, stream_id=%x)\n", stream_control.raw, stream_control.en, stream_control.stream_id));

        INTEL_KEYP_STREAM_TXRX_CONTROL tx_control = {.raw = mmio_read_reg32(&stream_cfg_reg_ptr->tx_ctrl)};
        TEEIO_PRINT(( "      tx_control     : %08x (key_set_select=%x, prime_key_set_0=%x, prime_key_set_1=%x)\n",
                       tx_control.raw,
                       tx_control.stream_tx_control.key_set_select,
                       tx_control.stream_tx_control.prime_key_set_0,
                       tx_control.stream_tx_control.prime_key_set_1));

        INTEL_KEYP_STREAM_TXRX_STATUS tx_status = {.raw = mmio_read_reg32(&stream_cfg_reg_ptr->tx_status)};
        TEEIO_PRINT(( "      tx_status      : %08x (key_set_status=%x, ready_key_set_0=%x, ready_key_set_1=%x)\n",
                       tx_status.raw,
                       tx_status.stream_tx_status.key_set_status,
                       tx_status.stream_tx_status.ready_key_set_0,
                       tx_status.stream_tx_status.ready_key_set_1));

        INTEL_KEYP_STREAM_TXRX_CONTROL rx_control = {.raw = mmio_read_reg32(&stream_cfg_reg_ptr->rx_ctrl)};
        TEEIO_PRINT(( "      rx_control     : %08x (prime_key_set_0=%x, prime_key_set_1=%x)\n",
                       rx_control.raw,
                       rx_control.stream_rx_control.prime_key_set_0,
                       rx_control.stream_rx_control.prime_key_set_1));

        INTEL_KEYP_STREAM_TXRX_STATUS rx_status = {.raw = mmio_read_reg32(&stream_cfg_reg_ptr->rx_status)};
        TEEIO_PRINT(( "      rx_status      : %08x (ready_key_set_0=%x, ready_key_set_1=%x)\n",
                       rx_status.raw,
                       rx_status.stream_rx_status.ready_key_set_0,
                       rx_status.stream_rx_status.ready_key_set_1));

        // INTEL_KEYP_STREAM_KEYSET_SLOT_ID tx_ks0 = {.raw = mmio_read_reg32(&stream_cfg_reg_ptr->tx_key_set_0)};
        // TEEIO_PRINT(( "      tx_keyset_0    : pr=%x, npr=%x, cpl=%x\n", tx_ks0.pr, tx_ks0.npr, tx_ks0.cpl));

        // INTEL_KEYP_STREAM_KEYSET_SLOT_ID rx_ks0 = {.raw = mmio_read_reg32(&stream_cfg_reg_ptr->rx_key_set_0)};
        // TEEIO_PRINT(( "      rx_keyset_0    : pr=%x, npr=%x, cpl=%x\n", rx_ks0.pr, rx_ks0.npr, rx_ks0.cpl));

        // INTEL_KEYP_STREAM_KEYSET_SLOT_ID tx_ks1 = {.raw = mmio_read_reg32(&stream_cfg_reg_ptr->tx_key_set_1)};
        // TEEIO_PRINT(( "      tx_keyset_1    : pr=%x, npr=%x, cpl=%x\n", tx_ks1.pr, tx_ks1.npr, tx_ks1.cpl));

        // INTEL_KEYP_STREAM_KEYSET_SLOT_ID rx_ks1 = {.raw = mmio_read_reg32(&stream_cfg_reg_ptr->rx_key_set_1)};
        // TEEIO_PRINT(( "      rx_keyset_1    : pr=%x, npr=%x, cpl=%x\n", rx_ks1.pr, rx_ks1.npr, rx_ks1.cpl));

        TEEIO_PRINT(( "\n"));
    }
}

bool dump_root_port(ide_common_test_port_context_t *port_context)
{
    TEEIO_PRINT(( "\n"));
    TEEIO_PRINT(( "RootPort - %s(%s).\n", port_context->port->port_name, port_context->port->bdf));

    lside_dump_ecap(port_context->cfg_space_fd, port_context->ecap_offset);
    lside_dump_kcbar((INTEL_KEYP_ROOT_COMPLEX_KCBAR *)port_context->mapped_kcbar_addr);

    return true;
}

bool dump_sw_conn(ide_common_test_switch_internal_conn_context_t *sw_conn)
{
    PCIE_IDE_CAP ide_cap = {0};
    PCIE_IDE_CTRL ide_ctrl = {0};

    ide_common_test_switch_internal_conn_context_t *conn = sw_conn;
    while(conn)
    {
        TEEIO_PRINT(( "Switch_%d\n", conn->switch_id));

        // conn->ups.port
        if(!read_ide_cap_ctrl_register(conn->ups.port, &ide_cap.raw, &ide_ctrl.raw)) {
            return false;
        }
        TEEIO_PRINT(( "  Ups             : %s (%s)\n", conn->ups.port->port_name, conn->ups.port->bdf));
        TEEIO_PRINT(( "    ide_cap       : %08x\n", ide_cap.raw));
        TEEIO_PRINT(( "                  : lnk_ide=%x, sel_ide=%x, ft=%x, aggr=%x, pcrc=%x, alog=%x, sel_ide_cfg_req=%x, num_lnk_ide=%x, num_sel_ide=%x\n",
                                                            ide_cap.lnk_ide_supported, ide_cap.sel_ide_supported,
                                                            ide_cap.ft_supported, ide_cap.aggr_supported,
                                                            ide_cap.pcrc_supported, ide_cap.supported_algo,
                                                            ide_cap.sel_ide_cfg_req_supported,
                                                            ide_cap.num_lnk_ide, ide_cap.num_sel_ide));

        TEEIO_PRINT(( "    ide_ctrl      : %08x (ft_supported=%x)\n", ide_ctrl.raw, ide_ctrl.ft_supported));

        TEEIO_PRINT(( "\n"));

        // conn->dps.port
        if(!read_ide_cap_ctrl_register(conn->dps.port, &ide_cap.raw, &ide_ctrl.raw)) {
            return false;
        }
        TEEIO_PRINT(( "  Dps             : %s (%s)\n", conn->dps.port->port_name, conn->dps.port->bdf));
        TEEIO_PRINT(( "    ide_cap       : %08x\n", ide_cap.raw));
        TEEIO_PRINT(( "                  : lnk_ide=%x, sel_ide=%x, ft=%x, aggr=%x, pcrc=%x, alog=%x, sel_ide_cfg_req=%x, num_lnk_ide=%x, num_sel_ide=%x\n",
                                                            ide_cap.lnk_ide_supported, ide_cap.sel_ide_supported,
                                                            ide_cap.ft_supported, ide_cap.aggr_supported,
                                                            ide_cap.pcrc_supported, ide_cap.supported_algo,
                                                            ide_cap.sel_ide_cfg_req_supported,
                                                            ide_cap.num_lnk_ide, ide_cap.num_sel_ide));

        TEEIO_PRINT(( "    ide_ctrl      : %08x (ft_supported=%x)\n", ide_ctrl.raw, ide_ctrl.ft_supported));

        TEEIO_PRINT(( "\n"));

        conn = conn->next;
    }

    return true;
}

bool list_devices_in_top(IDE_TEST_CONFIG *test_config, int top_id)
{
    ide_common_test_port_context_t *port_context = NULL;

    // first rootport
    dump_root_port(devices_context.root_port_context);

    // then sw_conn1 if exists
    if(devices_context.sw_conn1) {
        dump_sw_conn(devices_context.sw_conn1);
    }

    // then sw_conn2 if exists
    if(devices_context.sw_conn2) {
        dump_sw_conn(devices_context.sw_conn2);
    }

    // then upper_port if it is different from root_port
    if (devices_context.root_port_context != devices_context.upper_port_context)
    {
        port_context = devices_context.upper_port_context;
        TEEIO_PRINT(( "\n"));
        TEEIO_PRINT(( "Device - %s(%s).\n", port_context->port->port_name, port_context->port->bdf));
        lside_dump_ecap(devices_context.upper_port_context->cfg_space_fd, devices_context.upper_port_context->ecap_offset);
    }

    // then lower_port
    port_context = devices_context.lower_port_context;
    TEEIO_PRINT(( "\n"));
    TEEIO_PRINT(( "Device - %s(%s).\n", port_context->port->port_name, port_context->port->bdf));
    lside_dump_ecap(devices_context.lower_port_context->cfg_space_fd, devices_context.lower_port_context->ecap_offset);

    return true;
}

int main(int argc, char *argv[])
{
    TEEIO_PRINT(( "%s version %s\n", LS_IDE_NAME, LS_IDE_VERSION));

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

    if (!scan_open_devices_in_top(&lside_test_config, g_top_id, &devices_context))
    {
        return false;
    }

    if (g_ide_operation == IDE_OPERATION_LIST)
    {
        list_devices_in_top(&lside_test_config, g_top_id);
    }

    return 0;
}
