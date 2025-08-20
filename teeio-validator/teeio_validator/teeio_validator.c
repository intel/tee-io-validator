/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include "teeio_validator.h"

#include <stdlib.h>
#include <ctype.h>
#include "ide_test.h"
#include "command.h"

char g_bdf[] = {'2','a',':','0','0','.','0','\0'};
char g_rp_bdf[] = {'2','9',':','0','2','.','0','\0'};
bool g_skip_tdisp = false;
uint8_t g_stream_id = 0;
bool g_pci_log = false;
bool g_ide_key_refresh = false;
TEST_IDE_TYPE g_test_ide_type = TEST_IDE_TYPE_SEL_IDE;
TEEIO_TEST_CATEGORY g_test_category = TEEIO_TEST_CATEGORY_MAX;
IDE_TEST_CONFIG ide_test_config = {0};
int g_top_id = -1;
int g_config_id = -1;
char g_test_case[MAX_CASE_NAME_LENGTH] = {0};
bool g_run_test_suite = true;
bool g_teeio_fixed_key = false;
pci_tdisp_interface_id_t g_tdisp_interface_id = {0};
int g_test_interval = 0;
int g_test_rounds = 0;

TEEIO_DEBUG_LEVEL g_debug_level = TEEIO_DEBUG_WARN;
bool g_libspdm_log = false;
bool g_doe_log = false;
uint16_t g_scan_segment = INVALID_SCAN_SEGMENT;
uint8_t g_scan_bus = INVALID_SCAN_BUS;

FILE* m_logfile = NULL;

bool log_file_init(const char* filepath);
bool pcap_file_init(const char* filepath, uint32_t transport_layer);
void log_file_close();
void pcap_file_close();
void teeio_init_test_funcs();
void teeio_clean_test_libs();

extern const char *IDE_TEST_IDE_TYPE_NAMES[];

bool parse_ide_test_init(IDE_TEST_CONFIG *test_config, const char* ide_test_ini);
bool parse_cmdline_option(int argc, char *argv[], char* file_name, IDE_TEST_CONFIG *ide_test_config, bool* print_usage, uint8_t* debug_level);
void print_usage();
bool run(IDE_TEST_CONFIG *test_config);
bool update_test_config_with_given_top_config_id(IDE_TEST_CONFIG *test_config, int top_id, int config_id, const char* test_case, TEEIO_TEST_CATEGORY test_category);
bool is_valid_test_case(const char* test_case_name, TEEIO_TEST_CATEGORY test_category);

static TEEIO_TEST_CATEGORY get_test_category_from_configuration(IDE_TEST_CONFIG* test_config, int config_id)
{
  TEEIO_ASSERT (test_config != NULL);

  for (int i = 0; i < test_config->configurations.cnt; i++) {
      if (test_config->configurations.configurations[i].id == config_id) {
          return test_config->configurations.configurations[i].test_category;
      }
  }

  return TEEIO_TEST_CATEGORY_MAX;
}

int main(int argc, char *argv[])
{
    char ide_test_ini_file[MAX_FILE_NAME] = {0};
    bool to_print_usage = false;
    int ret = -1;
    uint8_t debug_level = TEEIO_DEBUG_NUM;

    if (!log_file_init(LOGFILE)){
        TEEIO_PRINT(("Failed to init log file!\n"));
        return -1;
    }

    TEEIO_PRINT(("%s version %s\n", TEEIO_VALIDATOR_NAME, TEEIO_VALIDATOR_VERSION));

    teeio_init_test_funcs();

    // parse command line optioins
    if(!parse_cmdline_option(argc, argv, ide_test_ini_file, &ide_test_config, &to_print_usage, &debug_level)) {
        print_usage();
        goto MainDone;
    }

    if(debug_level != TEEIO_DEBUG_NUM) {
        g_debug_level = debug_level;
    }

    if(to_print_usage) {
        print_usage();
        ret = 0;
        goto MainDone;
    }

    if(ide_test_ini_file[0] == 0) {
        TEEIO_PRINT(("-f parameter is missing.\n"));
        print_usage();
        goto MainDone;
    }

    if(!parse_ide_test_init(&ide_test_config, ide_test_ini_file)) {
        TEEIO_PRINT(("Parse %s failed.\n", ide_test_ini_file));
        goto MainDone;
    }

    g_pci_log = ide_test_config.main_config.pci_log;
    g_libspdm_log = ide_test_config.main_config.libspdm_log;
    g_doe_log = ide_test_config.main_config.doe_log;

    if(debug_level == TEEIO_DEBUG_NUM) {
        g_debug_level = ide_test_config.main_config.debug_level;
    }

    // tester wants to run a specific test case instead of run test suite
    if(!g_run_test_suite) {
      // g_top_id and g_config_id shall be set in command line
      if(g_top_id == -1 || g_config_id == -1) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid -t or -c parameter.\n"));
        goto MainDone;
      }

      // g_test_case shall be set in command line
      if(g_test_case[0] == 0) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid -s parameter.\n"));
        goto MainDone;
      }

      // we need to find out the test category from configuration (g_config_id)
      g_test_category = get_test_category_from_configuration(&ide_test_config, g_config_id);
      if(g_test_category == TEEIO_TEST_CATEGORY_MAX) {
          TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid -c parameter. %d\n", g_config_id));
          goto MainDone;
      }

      // we need to check if the test_case is valid
      if(!is_valid_test_case(g_test_case, g_test_category)) {
        TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Invalid -s parameter. %s\n", g_test_case));
        goto MainDone;
      }

      // then update the test_config with given topology / configuration id / test case
      if(!update_test_config_with_given_top_config_id(&ide_test_config, g_top_id, g_config_id, g_test_case, g_test_category)) {
        goto MainDone;
      }
    }

    // Open pcap file
    if (ide_test_config.main_config.pcap_enable) {
       if (!pcap_file_init(PCAPFILE, SOCKET_TRANSPORT_TYPE_PCI_DOE)) {
           TEEIO_PRINT(("Failed to open pcap file!\n"));
           goto MainDone;
       }
    }

    srand((unsigned int)time(NULL));

    run(&ide_test_config);

    ret = 0;

    // Close pcap file
    if (ide_test_config.main_config.pcap_enable) {
       pcap_file_close();
    }

MainDone:
    log_file_close();
    teeio_clean_test_libs();

    return ret;
}
