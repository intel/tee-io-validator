/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

#include "hal/base.h"
#include "hal/library/debuglib.h"
#include "teeio_debug.h"
#include "utils.h"

#include "ide_test.h"

extern const char *IDE_PORT_TYPE_NAMES[];
extern const char *IDE_TEST_IDE_TYPE_NAMES[];
extern const char *IDE_TEST_CONNECT_TYPE_NAMES[];
extern const char *IDE_TEST_TOPOLOGY_TYPE_NAMES[];

extern char g_bdf[];
extern char g_rp_bdf[];
extern TEST_IDE_TYPE g_test_ide_type;
extern uint8_t g_stream_id;
extern bool g_pci_log;
extern bool g_skip_tdisp;
extern bool g_ide_key_refresh;
extern int g_top_id;
extern int g_config_id;
extern char g_test_case[MAX_CASE_NAME_LENGTH];
extern TEEIO_DEBUG_LEVEL g_debug_level;
extern uint8_t g_scan_bus;

bool is_valid_test_case(const char* test_case_name);

void print_usage()
{
  TEEIO_PRINT(("\nUsage:\n"));
  TEEIO_PRINT(("  teeio_validator -f ide_test.ini\n"));
  TEEIO_PRINT(("  teeio_validator -f ide_test.ini -t 1 -c 1 -s Test.IdeStream\n"));

  TEEIO_PRINT(("\nOptions:\n"));
  TEEIO_PRINT(("  -f <ide_test.ini>   : The file name of test configuration. For example ide_test.ini\n"));
  TEEIO_PRINT(("  -t <top_id>         : topology id which is to be tested. For example 1\n"));
  TEEIO_PRINT(("  -c <config_id>      : configuration id which is to be tested. For example 1\n"));
  TEEIO_PRINT(("  -s <test_case>      : Test case to be tested. For example Test.IdeStream\n"));
  TEEIO_PRINT(("  -l <debug_level>    : Set debug level. error/warn/info/verbose\n"));
  TEEIO_PRINT(("  -b <scan_bus>       : Bus number in hex format. For example 0x1a\n"));
  TEEIO_PRINT(("  -h                  : Display this usage\n"));
}

/**
 * parse the command line option
*/
bool parse_cmdline_option(int argc, char *argv[], char* file_name, IDE_TEST_CONFIG *ide_test_config, bool* print_usage)
{
  int opt, v;
  uint8_t data8;

  TEEIO_ASSERT(argc > 0);
  TEEIO_ASSERT(argv != NULL);
  TEEIO_ASSERT(file_name != NULL);
  TEEIO_ASSERT(ide_test_config != NULL);
  TEEIO_ASSERT(print_usage != NULL);

  while ((opt = getopt(argc, argv, "f:t:c:s:l:b:h")) != -1) {
      switch (opt) {
          case 'f':
              if(!validate_file_name(optarg)) {
                TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid -f parameter. %s\n", optarg));
                return false;
              }
              sprintf(file_name, "%s", optarg);
              break;

          case 's':
              if(!is_valid_test_case(optarg)) {
                TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid -f parameter. %s\n", optarg));
                return false;
              }
              sprintf(g_test_case, "%s", optarg);
              break;

          case 't':
              v = atoi(optarg);
              if(v <= 0 || v > MAX_TOPOLOGY_NUM) {
                TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid -t parameter. %s\n", optarg));
                return false;
              }
              g_top_id = v;
              break;

          case 'c':
              v = atoi(optarg);
              if(v <= 0 || v > MAX_CONFIGURATION_NUM) {
                TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid -c parameter. %s\n", optarg));
                return false;
              }
              g_config_id = v;
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

        case 'l':
            g_debug_level = get_ide_log_level_from_string(optarg);
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
