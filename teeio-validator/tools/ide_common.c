/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <ctype.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hal/base.h"
#include "hal/library/debuglib.h"
#include "ide_tools.h"
#include "teeio_debug.h"
#include "helperlib.h"
#include "pcie_ide_lib.h"

ide_common_test_switch_internal_conn_context_t *alloc_switch_internal_conn_context(IDE_TEST_CONFIG *test_config, IDE_TEST_TOPOLOGY *top, IDE_SWITCH_INTERNAL_CONNECTION *conn)
{
    ide_common_test_switch_internal_conn_context_t *conn_context = NULL;
    ide_common_test_switch_internal_conn_context_t *conn_header = NULL;

    while (conn != NULL)
    {
        conn_context = (ide_common_test_switch_internal_conn_context_t *)malloc(sizeof(ide_common_test_switch_internal_conn_context_t));
        TEEIO_ASSERT(conn_context);
        memset(conn_context, 0, sizeof(ide_common_test_switch_internal_conn_context_t));

        IDE_SWITCH *sw = get_switch_by_id(test_config, conn->switch_id);
        TEEIO_ASSERT(sw);
        IDE_PORT *ups_port = get_port_from_switch_by_id(sw, conn->ups_port);
        IDE_PORT *dps_port = get_port_from_switch_by_id(sw, conn->dps_port);
        TEEIO_ASSERT(ups_port && dps_port);

        conn_context->switch_id = conn->switch_id;
        conn_context->ups.port = ups_port;
        conn_context->dps.port = dps_port;

        if (conn_header == NULL)
        {
            conn_header = conn_context;
        }
        else
        {
            ide_common_test_switch_internal_conn_context_t *itr = conn_header;
            while (itr->next)
            {
                itr = itr->next;
            }
            itr->next = conn_context;
        }

        conn = conn->next;
    }

    return conn_header;
}

bool scan_open_devices_in_top(IDE_TEST_CONFIG *test_config, int top_id, DEVCIES_CONTEXT *devices_context)
{
    bool ret = false;
    ide_common_test_switch_internal_conn_context_t *sw_conn1 = NULL;
    ide_common_test_switch_internal_conn_context_t *sw_conn2 = NULL;
    ide_common_test_port_context_t *root_port_context = NULL;
    ide_common_test_port_context_t *upper_port_context = NULL;
    ide_common_test_port_context_t *lower_port_context = NULL;

    IDE_TEST_TOPOLOGY *top = get_topology_by_id(test_config, top_id);
    if (top == NULL)
    {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "topology is not found. (top_id = %d)\n", top_id));
        return false;
    }

    IDE_PORT *root_port = get_port_by_id(test_config, top->root_port);
    IDE_PORT *upper_port = get_port_by_id(test_config, top->upper_port);
    IDE_PORT *lower_port = get_port_by_id(test_config, top->lower_port);

    if (top->connection == IDE_TEST_CONNECT_SWITCH)
    {
        // root_port and upper_port are the same
        TEEIO_ASSERT(root_port->id == upper_port->id);
        sw_conn1 = alloc_switch_internal_conn_context(test_config, top, top->sw_conn1);
    }
    else if (top->connection == IDE_TEST_CONNECT_P2P)
    {
        // root_port and upper_port are not the same
        TEEIO_ASSERT(root_port->id != upper_port->id);
        sw_conn1 = alloc_switch_internal_conn_context(test_config, top, top->sw_conn1);
        sw_conn2 = alloc_switch_internal_conn_context(test_config, top, top->sw_conn2);
    }
    else if (top->connection == IDE_TEST_CONNECT_DIRECT)
    {
        // root_port and upper_port are the same
        TEEIO_ASSERT(root_port->id == upper_port->id);
    }

    // now scan devices
    if (top->connection == IDE_TEST_CONNECT_DIRECT || top->connection == IDE_TEST_CONNECT_SWITCH)
    {
        // root_port and upper_port is the same port
        ret = scan_devices_at_bus(root_port, lower_port, sw_conn1, top->segment, top->bus);
    }
    else if (top->connection == IDE_TEST_CONNECT_P2P)
    {
        // root_port and upper_port is not the same port
        ret = scan_devices_at_bus(root_port, upper_port, sw_conn1, top->segment, top->bus);
        if (ret)
        {
            ret = scan_devices_at_bus(root_port, lower_port, sw_conn2, top->segment, top->bus);
        }
    }

    if (!ret)
    {
        goto ScanOpenDevicesFail;
    }

    // then open root_port
    root_port_context = (ide_common_test_port_context_t *)malloc(sizeof(ide_common_test_port_context_t));
    memset(root_port_context, 0, sizeof(ide_common_test_port_context_t));
    root_port_context->port = root_port;
    if (!open_root_port(root_port_context))
    {
        goto ScanOpenDevicesFail;
    }

    // then open upper_port
    if (top->connection == IDE_TEST_CONNECT_P2P)
    {
        // upper_port is in a device
        upper_port_context = (ide_common_test_port_context_t *)malloc(sizeof(ide_common_test_port_context_t));
        memset(upper_port_context, 0, sizeof(ide_common_test_port_context_t));
        upper_port_context->port = root_port;
        if (!open_dev_port(upper_port_context))
        {
            goto ScanOpenDevicesFail;
        }
    }
    else
    {
        upper_port_context = root_port_context;
    }

    // then open lower_port
    lower_port_context = (ide_common_test_port_context_t *)malloc(sizeof(ide_common_test_port_context_t));
    memset(lower_port_context, 0, sizeof(ide_common_test_port_context_t));
    lower_port_context->port = lower_port;
    if (!open_dev_port(lower_port_context))
    {
        goto ScanOpenDevicesFail;
    }

    devices_context->root_port_context = root_port_context;
    devices_context->upper_port_context = upper_port_context;
    devices_context->lower_port_context = lower_port_context;
    devices_context->sw_conn1 = sw_conn1;
    devices_context->sw_conn2 = sw_conn2;

    return true;

ScanOpenDevicesFail:
    if (root_port_context)
        free(root_port_context);
    if (upper_port_context && upper_port != root_port)
        free(upper_port);
    if (lower_port_context)
        free(lower_port_context);

    return false;
}

bool read_ide_cap_ctrl_register(IDE_PORT* port, uint32_t *ide_cap, uint32_t *ide_ctrl)
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
        goto ReadIdeCapCtrlFailed;
    }

    uint32_t offset = ecap_offset + 4;
    *ide_cap = device_pci_read_32(offset, fd);
    offset += 4;
    *ide_ctrl = device_pci_read_32(offset, fd);

    ret = true;

ReadIdeCapCtrlFailed:
    close(fd);

    return ret;
}

ide_test_case_name_t *get_test_case_from_string(const char *test_case_name, int *index, TEEIO_TEST_CATEGORY test_category)
{
    return NULL;
}

bool pcie_doe_init_request(uint8_t doe_discovery_version)
{
    return true;
}

void trigger_doe_abort()
{
}

bool is_doe_error_asserted()
{
    return false;
}

void libspdm_sleep(uint64_t microseconds)
{
}

const char* get_test_configuration_name(int configuration_type, TEEIO_TEST_CATEGORY test_category)
{
  return NULL;
}

bool parse_test_configuration_priv_names(const char* key, const char* value, TEEIO_TEST_CATEGORY test_category, IDE_TEST_CONFIGURATION* config)
{
    return false;
}

const char** get_test_configuration_priv_names(TEEIO_TEST_CATEGORY test_category)
{
    return NULL;
}

ide_test_case_name_t* get_test_case_name(int case_class, TEEIO_TEST_CATEGORY test_category)
{
    return NULL;
}

bool teeio_check_configuration_bitmap(uint32_t* bitmap, TEEIO_TEST_CATEGORY test_category)
{
    return true;
}