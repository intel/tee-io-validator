/**
 *  Copyright Notice:
 *  Copyright 024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "pcie.h"
#include "intel_keyp.h"
#include "ide_test.h"
#include "teeio_debug.h"
#include "helperlib.h"
#include "helper_internal.h"

extern const char *IDE_PORT_TYPE_NAMES[];
extern const char *IDE_TEST_IDE_TYPE_NAMES[];
extern const char *IDE_TEST_CONNECT_TYPE_NAMES[];
extern const char *IDE_TEST_TOPOLOGY_TYPE_NAMES[];
extern const char *IDE_TEST_CATEGORY_NAMES[];

bool is_valid_topology_connection(uint8_t *connection)
{
  if(connection == NULL) {
    return false;
  }
  for(int i = 0; i < IDE_TEST_CONNECT_NUM; i++) {
    if(strcmp(IDE_TEST_CONNECT_TYPE_NAMES[i], (const char *)connection) == 0) {
      return true;
    }
  }
  return false;
}

IDE_TEST_CONNECT_TYPE get_connection_from_name(uint8_t *name)
{
  if(name == NULL) {
    return IDE_TEST_CONNECT_NUM;
  }
  for(int i = 0; i < IDE_TEST_CONNECT_NUM; i++) {
    if(strcmp(IDE_TEST_CONNECT_TYPE_NAMES[i], (const char *)name) == 0) {
      return i;
    }
  }
  return IDE_TEST_CONNECT_NUM;
}

bool is_valid_port(IDE_TEST_PORTS_CONFIG *ports, uint8_t *port)
{
  if(ports == NULL || port == NULL) {
    return false;
  }

  for(int i = 0; i < ports->cnt; i++) {
    if(strcmp(ports->ports[i].port_name, (const char*)port) == 0) {
      return ports->ports[i].enabled;
    }
  }

  return false;
}
int get_port_id_from_name(IDE_TEST_PORTS_CONFIG *ports, uint8_t* port)
{
  if(ports == NULL || port == NULL) {
    return INVALID_PORT_ID;
  }

  for(int i = 0; i < ports->cnt; i++) {
    if(strcmp(ports->ports[i].port_name, (const char*)port) == 0) {
      return ports->ports[i].enabled ? ports->ports[i].id : INVALID_PORT_ID;
    }
  }

  return INVALID_PORT_ID;
}

// IDE_TEST_CATEGORY_NAMES
bool is_valid_ide_test_category(uint8_t *test_category)
{
  if(test_category == NULL) {
    return false;
  }
  for(int i = 0; i < IDE_TEST_CATEGORY_NUM; i++) {
    if(strcmp((const char*)test_category, IDE_TEST_CATEGORY_NAMES[i]) == 0) {
      return true;
    }
  }
  return false;
}

IDE_TEST_CATEGORY get_ide_test_category_from_name(const char* name)
{
  if(name == NULL) {
    return IDE_TEST_CATEGORY_NUM;
  }

  for(int i = 0; i < IDE_TEST_CATEGORY_NUM; i++) {
    if(strcmp(name, IDE_TEST_CATEGORY_NAMES[i]) == 0) {
      return i;
    }
  }
  return IDE_TEST_CATEGORY_NUM;
}

// IDE_TEST_TOPOLOGY_TYPE_NAMES
bool is_valid_topology_type(uint8_t *type)
{
  if(type == NULL) {
    return false;
  }
  for(int i = 0; i < IDE_TEST_TOPOLOGY_TYPE_NUM; i++) {
    if(strcmp((const char*)type, IDE_TEST_TOPOLOGY_TYPE_NAMES[i]) == 0) {
      return true;
    }
  }
  return false;
}

IDE_TEST_TOPOLOGY_TYPE get_topology_type_from_name(uint8_t* name)
{
  if(name == NULL) {
    return IDE_TEST_TOPOLOGY_TYPE_NUM;
  }

  for(int i = 0; i < IDE_TEST_TOPOLOGY_TYPE_NUM; i++) {
    if(strcmp((const char*)name, IDE_TEST_TOPOLOGY_TYPE_NAMES[i]) == 0) {
      return i;
    }
  }
  return IDE_TEST_TOPOLOGY_TYPE_NUM;
}

IDE_TEST_CONFIGURATION* get_configuration_by_id(IDE_TEST_CONFIG *test_config, int id)
{
  if(test_config == NULL || id <= 0) {
    return NULL;
  }

  for(int i = 0; i < MAX_CONFIGURATION_NUM; i++) {
    if(test_config->configurations.configurations[i].id == id && test_config->configurations.configurations[i].enabled) {
      return test_config->configurations.configurations + i;
    }
  }

  return NULL;  
}

IDE_TEST_TOPOLOGY* get_topology_by_id(IDE_TEST_CONFIG *test_config, int id)
{
  if(test_config == NULL || id <= 0) {
    return NULL;
  }

  for(int i = 0; i < MAX_TOPOLOGY_NUM; i++) {
    if(test_config->topologies.topologies[i].id == id && test_config->topologies.topologies[i].enabled) {
      return test_config->topologies.topologies + i;
    }
  }

  return NULL;
}

IDE_PORT* get_port_by_id(IDE_TEST_CONFIG *test_config, int id)
{
  if(test_config == NULL || id <= 0) {
    return NULL;
  }

  for(int i = 0; i < test_config->ports_config.cnt; i++) {
    if(test_config->ports_config.ports[i].id == id && test_config->ports_config.ports[i].enabled) {
      return test_config->ports_config.ports + i;
    }
  }

  return NULL;
}

IDE_PORT* get_port_by_name(IDE_TEST_CONFIG *test_config, const char* portname)
{
  if(test_config == NULL || portname == NULL) {
    return NULL;
  }

  for(int i = 0; i < test_config->ports_config.cnt; i++) {
    if(strcmp(test_config->ports_config.ports[i].port_name, portname) == 0 && test_config->ports_config.ports[i].enabled) {
      return test_config->ports_config.ports + i;
    }
  }

  return NULL;
}

IDE_SWITCH *get_switch_by_id(IDE_TEST_CONFIG *test_config, int switch_id)
{
  if(test_config == NULL || switch_id <= 0) {
    return NULL;
  }

  for(int i = 0; i < test_config->switches_config.cnt; i++) {
    if(test_config->switches_config.switches[i].id == switch_id && test_config->switches_config.switches[i].enabled) {
      return test_config->switches_config.switches + i;
    }
  }

  return NULL;
}

IDE_SWITCH *get_switch_by_name(IDE_TEST_CONFIG *test_config, const char* name)
{
  if(test_config == NULL || name == NULL) {
    return NULL;
  }

  for(int i = 0; i < test_config->switches_config.cnt; i++) {
    if(strcmp(test_config->switches_config.switches[i].name, name) == 0 && test_config->switches_config.switches[i].enabled) {
      return test_config->switches_config.switches + i;
    }
  }

  return NULL;
}


bool is_valid_switch_by_id(IDE_TEST_CONFIG *test_config, int switch_id)
{
  IDE_SWITCH *sw = get_switch_by_id(test_config, switch_id);
  return sw != NULL;
}

IDE_PORT* get_port_from_switch_by_id(IDE_SWITCH *sw, int port_id)
{
  if(sw == NULL || port_id <= 0) {
    return NULL;
  }

  for(int i = 0; i < sw->ports_cnt; i++) {
    if(sw->ports[i].enabled && sw->ports[i].id == port_id) {
      return sw->ports + i;
    }
  }

  return NULL;
}

bool is_valid_port_in_switch(IDE_SWITCH *sw, int port_id)
{
  return get_port_from_switch_by_id(sw, port_id) != NULL;
}

IDE_PORT *get_port_from_switch_by_name(IDE_SWITCH *sw, const char* name)
{
  if(sw == NULL || name == NULL) {
    return NULL;
  }

  for(int i = 0; i < sw->ports_cnt; i++) {
    if(sw->ports[i].enabled && strcmp(sw->ports[i].port_name, name) == 0) {
      return sw->ports + i;
    }
  }

  return NULL;
}

TEST_IDE_TYPE map_top_type_to_ide_type(IDE_TEST_TOPOLOGY_TYPE top_type)
{
  TEST_IDE_TYPE ide_type = TEST_IDE_TYPE_SEL_IDE;

  if(top_type == IDE_TEST_TOPOLOGY_TYPE_LINK_IDE) {
    ide_type = TEST_IDE_TYPE_LNK_IDE;
  } else if (top_type == IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE){
    NOT_IMPLEMENTED("selective_and_link_ide topology");
    ide_type = TEST_IDE_TYPE_NA;
  }

  return ide_type;
}
