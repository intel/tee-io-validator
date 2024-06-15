/**
 *  Copyright Notice:
 *  Copyright 2021-2022 DMTF. All rights reserved.
 *  License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/spdm-emu/blob/main/LICENSE.md
 **/

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <industry_standard/pcap.h>
#include <industry_standard/link_type_ex.h>
#include "pcap.h"
#include "command.h"
#include "teeio_debug.h"

#define PCAP_PACKET_MAX_SIZE 0x00010000

static FILE *m_pcap_file;

bool open_pcap_packet_file(const char *pcap_file_name, uint32_t transport_layer)
{
    pcap_global_header_t pcap_global_header;

    if (pcap_file_name == NULL) {
        return false;
    }

    pcap_global_header.magic_number = PCAP_GLOBAL_HEADER_MAGIC;
    pcap_global_header.version_major = PCAP_GLOBAL_HEADER_VERSION_MAJOR;
    pcap_global_header.version_minor = PCAP_GLOBAL_HEADER_VERSION_MINOR;
    pcap_global_header.this_zone = 0;
    pcap_global_header.sig_figs = 0;
    pcap_global_header.snap_len = PCAP_PACKET_MAX_SIZE;
    if (transport_layer == SOCKET_TRANSPORT_TYPE_MCTP) {
        pcap_global_header.network = LINKTYPE_MCTP;
    } else if (transport_layer == SOCKET_TRANSPORT_TYPE_PCI_DOE) {
        pcap_global_header.network = LINKTYPE_PCI_DOE;
    } else {
        return false;
    }

    if ((m_pcap_file = fopen(pcap_file_name, "wb")) == NULL) {
        TEEIO_DEBUG ((TEEIO_DEBUG_ERROR, "!!!Unable to open pcap file %s!!!\n", pcap_file_name));
        return false;
    }

    if ((fwrite(&pcap_global_header, 1, sizeof(pcap_global_header),
                m_pcap_file)) != sizeof(pcap_global_header)) {
        TEEIO_DEBUG ((TEEIO_DEBUG_ERROR, "!!!Write pcap file error!!!\n"));
        close_pcap_packet_file();
        return false;
    }

    fflush(m_pcap_file);
    return true;
}

void close_pcap_packet_file(void)
{
    if (m_pcap_file != NULL) {
        fclose(m_pcap_file);
        m_pcap_file = NULL;
    }
}

void append_pcap_packet_data(const void *header, size_t header_size,
                             const void *data, size_t size)
{
    pcap_packet_header_t pcap_packet_header;
    size_t total_size;

    total_size = header_size + size;

    if (m_pcap_file != NULL) {
        time_t rawtime;
        time(&rawtime);

        pcap_packet_header.ts_sec = (uint32_t)rawtime;
        pcap_packet_header.ts_usec = 0;

        pcap_packet_header.incl_len =
            (uint32_t)((total_size > PCAP_PACKET_MAX_SIZE) ?
                       PCAP_PACKET_MAX_SIZE :
                       total_size);
        pcap_packet_header.orig_len = (uint32_t)total_size;

        if ((fwrite(&pcap_packet_header, 1, sizeof(pcap_packet_header),
                    m_pcap_file)) != sizeof(pcap_packet_header)) {
            TEEIO_DEBUG ((TEEIO_DEBUG_ERROR, "!!!Write pcap file error!!!\n"));
            close_pcap_packet_file();
            return;
        }

        if (total_size > PCAP_PACKET_MAX_SIZE) {
            total_size = PCAP_PACKET_MAX_SIZE;
        }

        if (header_size != 0) {
            if ((fwrite(header, 1, header_size, m_pcap_file)) !=
                header_size) {
                TEEIO_DEBUG ((TEEIO_DEBUG_ERROR, "!!!Write pcap file error!!!\n"));
                close_pcap_packet_file();
                return;
            }
        }

        if ((fwrite(data, 1, size, m_pcap_file)) != size) {
            TEEIO_DEBUG ((TEEIO_DEBUG_ERROR, "!!!Write pcap file error!!!\n"));
            close_pcap_packet_file();
            return;
        }

        fflush(m_pcap_file);
    }
}
