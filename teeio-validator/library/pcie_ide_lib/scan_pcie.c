/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#include "assert.h"
#include "hal/base.h"
#include "hal/library/debuglib.h"
#include "library/spdm_requester_lib.h"
#include "library/spdm_crypt_lib.h"
#include "ide_test.h"
#include "teeio_debug.h"
#include "helperlib.h"
#include "pcie_ide_internal.h"
int open_configuration_space(char *bdf);

typedef union {
  struct {
    uint8_t byte0;
    uint8_t byte1;
    uint8_t byte2;
    uint8_t byte3;
  };
  uint32_t raw;
} PCI_READ_DATA32;

#define PCI_HEADER_TYPE0  0
#define PCI_HEADER_TYPE1  1
#define PCI_HEADER_LAYOUT_MASK  0x7f

bool scan_rp_switch_internal_port_at_bus(IDE_PORT* port, uint16_t segment, uint8_t bus, uint8_t* SecBus, uint8_t* SubBus)
{
  bool res = false;
  PCI_READ_DATA32 data32 = { .raw = 0 };
  int fd = -1;

  port->segment = segment;
  port->bus = bus;
  snprintf(port->bdf, BDF_LENGTH, "%04x:%02x:%02x.%x", segment, bus, port->device, port->function&0xf);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Scan %s...\n", port->bdf));

  fd = open_configuration_space(port->bdf);
  if(fd == -1) {
    goto ScanSwitchInternalPortDone;
  }

  // PCIe Spec 6.1 Section 7.5.1.1.9
  // check header layout (bit 0:6) shall be 1
  data32.raw = device_pci_read_32(0x0c, fd);
  if((data32.byte2 & PCI_HEADER_LAYOUT_MASK) != PCI_HEADER_TYPE1) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "rootport/switch_internal_port(%s) HEADER_TYPE is not 1. (%x)\n", port->bdf, data32.byte2));
    goto ScanSwitchInternalPortDone;
  }

  // check class shall be bridge
  data32.raw = device_pci_read_32(0x08, fd);
  if(data32.byte3 != 0x06 && data32.byte2 != 0x04) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "rootport/switch_internal_port class/subclass is not 0x0604. (%02x, %02x)\n", port->bdf, data32.byte3, data32.byte2));
    goto ScanSwitchInternalPortDone;
  }

  // read the PriBus/SecBus/SubBus
  // PriBus shall be same as the port->bus
  // Bus number of Devices/Switches connected is in the range of [SecBus, SubBus]
  // Usually if SecBus==SubBus, it is the device connected, otherwise it is the Switch connected
  data32.raw = device_pci_read_32(0x18, fd);
  if(data32.byte0 != port->bus) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "rootport/switch_internal_port(%s) PrimaryBusNumber is not correct. (%02x, %02x)\n", port->bdf, data32.byte0, port->bus));
    goto ScanSwitchInternalPortDone;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "rootport/switch_internal_port(%s) SecBus=%02x, SubBus=%02x\n", port->bdf, data32.byte1, data32.byte2));

  *SecBus = data32.byte1;
  *SubBus = data32.byte2;
  TEEIO_ASSERT(*SecBus <= *SubBus);

  res = true;

ScanSwitchInternalPortDone:
  if(fd > 0) {
    close(fd);
  }

return res;
}

bool scan_switch_internal_conn_at_bus(uint16_t segment, uint8_t bus, uint8_t* SecBus, uint8_t *SubBus, IDE_PORT* ups_port, IDE_PORT* dps_port)
{
  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Start to scan switch internal_conn at bus: %02x\n", bus));

  if(!scan_rp_switch_internal_port_at_bus(ups_port, segment, bus, SecBus, SubBus)) {
    return false;
  }

  bus = *SecBus;
  if(!scan_rp_switch_internal_port_at_bus(dps_port, segment, bus, SecBus, SubBus)) {
    return false;
  }

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "End of scan switch internal_conn at bus: %02x\n", bus));
  return true;
}

bool scan_endpoint_at_bus(uint16_t segment, uint8_t bus, IDE_PORT* port)
{
  bool res = false;
  PCI_READ_DATA32 data32 = { .raw = 0 };
  int fd = -1;

  port->segment = segment;
  port->bus = bus;
  snprintf(port->bdf, BDF_LENGTH, "%04x:%02x:%02x.%x", segment, bus, port->device, port->function&0xf);

  TEEIO_DEBUG((TEEIO_DEBUG_INFO, "Scan endpoint(%s)...\n", port->bdf));

  fd = open_configuration_space(port->bdf);
  if(fd == -1) {
    goto ScanEndpointDone;
  }

  // PCIe Spec 6.1 Section 7.5.1.1.9
  // check header layout (bit 0:6) shall be 0
  data32.raw = device_pci_read_32(0x0c, fd);
  if((data32.byte2 & PCI_HEADER_LAYOUT_MASK) != PCI_HEADER_TYPE0) {
    TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "endpoint(%s) HEADER_TYPE is not 0. (%x)\n", port->bdf, data32.byte2));
    goto ScanEndpointDone;
  }

  res = true;

ScanEndpointDone:
  if(fd > 0) {
    close(fd);
  }

  TEEIO_ASSERT(res);

  return res;
}

// scan the devices from upper_port@bus
// TODO we assume the SecBus is the bus number of next device
// After the scanning, the bus field in IDE_PORT of the chain are all set.
// Here is an example:
//   upper=rootport_1
//   lower=endpoint_1
//   switches=1:port_1-port_2,2:port_1-port_2
bool scan_devices_at_bus(IDE_PORT* rp, IDE_PORT* ep, ide_common_test_switch_internal_conn_context_t* conn, uint16_t segment, uint8_t bus)
{
  uint8_t SecBus, SubBus;

  // set the rp's bus
  if(!scan_rp_switch_internal_port_at_bus(rp, segment, bus, &SecBus, &SubBus)) {
    return false;
  }

  bus = SecBus;
  if(conn != NULL) {
    while(conn) {
      if(!scan_switch_internal_conn_at_bus(segment, bus, &SecBus, &SubBus, conn->ups.port, conn->dps.port)) {
        return false;
      }
      bus = SecBus;
      conn = conn->next;
    }
  }

  if(!scan_endpoint_at_bus(segment, bus, ep)) {
    return false;
  }

  return true;
}
