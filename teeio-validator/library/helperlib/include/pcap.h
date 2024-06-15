/**
 *  Copyright Notice:
 *  Copyright 2021-2022 DMTF. All rights reserved.
 *  License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/spdm-emu/blob/main/LICENSE.md
 **/

#ifndef __SPDM_PCAP_H__
#define __SPDM_PCAP_H__

bool open_pcap_packet_file(const char *pcap_file_name, uint32_t transport_layer);

void close_pcap_packet_file(void);

void append_pcap_packet_data(const void *header, size_t header_size,
                             const void *data, size_t size);


#endif
