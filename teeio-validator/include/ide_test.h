/**
 *  Copyright Notice:
 *  Copyright 2023-2025 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __IDE_TEST_H__
#define __IDE_TEST_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/param.h>
#include "pcie.h"
#include "cxl_ide.h"
#include "cxl_tsp.h"
#include "spdm_test.h"
#include "intel_keyp.h"
#include "teeio_debug.h"

#define NOT_IMPLEMENTED(msg) \
  TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Not implemented - %s\n", msg)); \
  TEEIO_ASSERT(false)

#define MAX_SUPPORTED_PORTS_NUM     16
#define MAX_SUPPORTED_SWITCHES_NUM  16
#define MAX_SUPPORTED_SWITCH_PORTS_NUM  16
#define PORT_NAME_LENGTH 32
#define SWITCH_NAME_LENGTH 32
#define BDF_LENGTH 13
#define DF_LENGTH 5
#define TEST_CASE_DESCRIPTION_LENGTH 32
#define MAX_SUPPORTED_TEST_CASE_NUM 32
#define TEST_CASE_DETAILED_RESULTS_LENGTH 256
#define MAX_FILE_NAME 256
#define MAX_ARRAY_NUMBER 256
#define MAX_TOPOLOGY_NUM 16
#define MAX_CONFIGURATION_NUM 32
#define MAX_TEST_SUITE_NUM 32
#define MAX_NAME_LENGTH 128
#define MAX_CASE_NAME_LENGTH  32
#define MAX_LINE_LENGTH 512

#define MAX_SECTION_NAME_LENGTH 32
#define MAX_ENTRY_NAME_LENGTH 32
#define MAX_ENTRY_STRING_LENGTH 128
#define MAX_STREAM_ID 255
#define MAX_RP_STREAM_INDEX 255

#define MAX_CASE_ID 32
#define MAX_TEST_CASE_NUM 32

#define INVALID_IDE_ID  0xFF
#define INVALID_RP_STREAM_INDEX 0xFF
#define INVALID_SCAN_SEGMENT 0xFFFF
#define INVALID_SCAN_BUS 0xFF

// Follow doc/IdeKmTestCase
#define MAX_QUERY_CASE_ID 2
#define MAX_KEYPROG_CASE_ID 6
#define MAX_KSETGO_CASE_ID 4
#define MAX_KSETSTOP_CASE_ID 4
#define MAX_SPDMSESSION_CASE_ID 2
#define MAX_FULL_CASE_ID 2

#define INVALID_PORT_ID 0

#define MAIN_SECION "Main"
#define MAIN_SECTION_PCI_LOG "pci_log"

#define PORTS_SECTION "Ports"
#define TOPOLOGY_SECTION "Topology_%d"
#define CONFIGURATION_SECTION "Configuration_%d"
#define TEST_SUITE_SECTION "TestSuite_%d"

typedef enum {
  TEEIO_TEST_CATEGORY_PCIE_IDE = 0,
  TEEIO_TEST_CATEGORY_CXL_IDE,
  TEEIO_TEST_CATEGORY_CXL_TSP,
  TEEIO_TEST_CATEGORY_TDISP,
  TEEIO_TEST_CATEGORY_SPDM,
  TEEIO_TEST_CATEGORY_MAX
} TEEIO_TEST_CATEGORY;

typedef enum {
    TEST_IDE_TYPE_SEL_IDE = 0,
    TEST_IDE_TYPE_LNK_IDE = 1,
    TEST_IDE_TYPE_NA
} TEST_IDE_TYPE;

typedef enum
{
  IDE_PORT_TYPE_ROOTPORT = 0,
  IDE_PORT_TYPE_ENDPOINT = 1,
  IDE_PORT_TYPE_SWITCH = 2,
  IDE_PORT_TYPE_NUM
} IDE_PORT_TYPE;

typedef enum {
    IDE_TEST_IDE_TYPE_SEL_IDE = 0,
    IDE_TEST_IDE_TYPE_LNK_IDE = 1,
    IDE_TEST_IDE_TYPE_NUM
} IDE_TEST_IDE_TYPE;

typedef enum {
  IDE_TEST_CONNECT_DIRECT = 0,
  IDE_TEST_CONNECT_SWITCH = 1,
  IDE_TEST_CONNECT_P2P = 2,
  IDE_TEST_CONNECT_NUM
} IDE_TEST_CONNECT_TYPE;

typedef enum {
  IDE_TEST_TOPOLOGY_TYPE_SEL_IDE = 0,
  IDE_TEST_TOPOLOGY_TYPE_LINK_IDE = 1,
  IDE_TEST_TOPOLOGY_TYPE_SEL_LINK_IDE = 2,
  IDE_TEST_TOPOLOGY_TYPE_NUM
} IDE_TEST_TOPOLOGY_TYPE;

typedef enum {
  IDE_COMMON_TEST_CASE_QUERY = 0,
  IDE_COMMON_TEST_CASE_KEYPROG,
  IDE_COMMON_TEST_CASE_KSETGO,
  IDE_COMMON_TEST_CASE_KSETSTOP,
  IDE_COMMON_TEST_CASE_SPDMSESSION,
  IDE_COMMON_TEST_CASE_TEST,
  IDE_COMMON_TEST_CASE_NUM
} IDE_COMMON_TEST_CASE;


#define TEEIO_TEST_FIXED_TX_KEY_BYTE_VALUE 0x11
#define TEEIO_TEST_FIXED_RX_KEY_BYTE_VALUE 0x22

typedef struct
{
  int id;
  bool enabled;
  IDE_PORT_TYPE port_type;
  char port_name[PORT_NAME_LENGTH];
  char bdf[BDF_LENGTH];
  uint16_t segment;
  uint8_t bus;
  uint8_t device;
  uint8_t function;
  uint8_t port_index;
} IDE_PORT;

typedef struct
{
  int cnt;
  IDE_PORT ports[MAX_SUPPORTED_PORTS_NUM];
} IDE_TEST_PORTS_CONFIG;

typedef struct {
  int id;
  bool enabled;
  char name[SWITCH_NAME_LENGTH];
  int ports_cnt;
  IDE_PORT ports[MAX_SUPPORTED_SWITCH_PORTS_NUM];
} IDE_SWITCH;

typedef struct {
  int cnt;
  IDE_SWITCH switches[MAX_SUPPORTED_SWITCHES_NUM];
} IDE_TEST_SWITCHES_CONFIG;

typedef struct _IDE_SWITCH_INTERNAL_CONNECTION IDE_SWITCH_INTERNAL_CONNECTION;
struct _IDE_SWITCH_INTERNAL_CONNECTION {
  IDE_SWITCH_INTERNAL_CONNECTION *next;
  int switch_id;
  int ups_port;
  int dps_port;
};

typedef struct
{
  bool pci_log;
  uint32_t debug_level;
  bool libspdm_log;
  bool doe_log;
  bool wo_tdisp;
  bool pcap_enable;
} IDE_TEST_MAIN_CONFIG;

typedef struct {
  int id;
  bool enabled;
  IDE_TEST_TOPOLOGY_TYPE type;
  IDE_TEST_CONNECT_TYPE connection;
  int root_port;
  int upper_port;
  int lower_port;
  IDE_SWITCH_INTERNAL_CONNECTION *sw_conn1;
  IDE_SWITCH_INTERNAL_CONNECTION *sw_conn2;
  uint16_t segment;
  uint8_t bus;
  uint8_t stream_id;
} IDE_TEST_TOPOLOGY;

typedef struct {
  int cnt;
  IDE_TEST_TOPOLOGY topologies[MAX_TOPOLOGY_NUM];
} IDE_TEST_TOPOLOGYS;

typedef union {
  CXL_IDE_PRIV_CONFIG_DATA cxl_ide;
} IDE_TEST_CONFIGURATION_PRIV_DATA;

typedef struct {
  int id;
  bool enabled;
  IDE_TEST_TOPOLOGY_TYPE type;
  TEEIO_TEST_CATEGORY test_category;
  uint32_t bit_map;
  IDE_TEST_CONFIGURATION_PRIV_DATA priv_data;
} IDE_TEST_CONFIGURATION;

typedef struct {
  int cnt;
  IDE_TEST_CONFIGURATION configurations[MAX_CONFIGURATION_NUM];
} IDE_TEST_CONFIGURATIONS;

typedef struct {
  uint32_t cases_cnt;
  uint32_t cases_id[MAX_CASE_ID];
} IDE_TEST_CASE;

typedef struct {
  IDE_TEST_CASE cases[MAX_TEST_CASE_NUM];
} IDE_TEST_CASES;

typedef struct {
  int id;
  bool enabled;
  IDE_TEST_TOPOLOGY_TYPE type;
  int topology_id;
  int configuration_id;
  TEEIO_TEST_CATEGORY test_category;
  IDE_TEST_CASES test_cases;
} IDE_TEST_SUITE;

typedef struct {
  int cnt;
  IDE_TEST_SUITE test_suites[MAX_TEST_SUITE_NUM];
} IDE_TEST_SUITES;

typedef struct {
  IDE_TEST_MAIN_CONFIG main_config;
  IDE_TEST_PORTS_CONFIG ports_config;
  IDE_TEST_SWITCHES_CONFIG switches_config;
  IDE_TEST_TOPOLOGYS topologies;
  IDE_TEST_CONFIGURATIONS configurations;
  IDE_TEST_SUITES test_suites;
} IDE_TEST_CONFIG;

/*
 * Definitions of ide_run_test_suite_t/test_group_t/test_case_t
*/
#define SIGNATURE_16(A, B)  ((A) | (B << 8))
#define SIGNATURE_32(A, B, C, D)  (SIGNATURE_16 (A, B) | (SIGNATURE_16 (C, D) << 16))

#define SUITE_CONTEXT_SIGNATURE SIGNATURE_32('S', 'U', 'I', 'T')
#define GROUP_CONTEXT_SIGNATURE SIGNATURE_32('G', 'R', 'O', 'P')
#define CASE_CONTEXT_SIGNATURE SIGNATURE_32('C', 'A', 'S', 'E')
#define CONFIG_CONTEXT_SIGNATURE SIGNATURE_32('C', 'N', 'F', 'G')

typedef enum {
  IDE_TEST_CONFIGURATION_TYPE_DEFAULT = 0,
  IDE_TEST_CONFIGURATION_TYPE_SWITCH,
  IDE_TEST_CONFIGURATION_TYPE_PARTIAL_HEADER_ENC,
  IDE_TEST_CONFIGURATION_TYPE_PCRC,
  IDE_TEST_CONFIGURATION_TYPE_AGGGEG,
  IDE_TEST_CONFIGURATION_TYPE_SELECTIVE_IDE_FOR_CONFIG,
  IDE_TEST_CONFIGURATION_TYPE_TEE_LIMITED_STREAM,
  IDE_TEST_CONFIGURATION_TYPE_FLIT_MODE_DISABLE,
  IDE_TEST_CONFIGURATION_TYPE_NUM
} IDE_TEST_CONFIGURATION_TYPE;

#define BIT_MASK(n) ((uint32_t)(1<<n))

#define SELECTIVE_IDE_CONFIGURATION_BITMASK \
  ((BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_DEFAULT)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_SWITCH)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_PARTIAL_HEADER_ENC)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_PCRC)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_AGGGEG)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_SELECTIVE_IDE_FOR_CONFIG)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_TEE_LIMITED_STREAM)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_FLIT_MODE_DISABLE)))

#define LINK_IDE_CONFIGURATION_BITMASK \
  ((BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_DEFAULT)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_SWITCH)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_PARTIAL_HEADER_ENC)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_PCRC)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_AGGGEG)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_FLIT_MODE_DISABLE)))

#define SELECTIVE_LINK_IDE_CONFIGURATION_BITMASK \
  ((BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_DEFAULT)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_PARTIAL_HEADER_ENC)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_PCRC)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_SELECTIVE_IDE_FOR_CONFIG)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_AGGGEG)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_FLIT_MODE_DISABLE)))

typedef enum {
  IDE_COMMON_TEST_ACTION_RUN = 0,
  IDE_COMMON_TEST_ACTION_SKIP = 1
} IDE_COMMON_TEST_ACTION;

typedef struct {
  IDE_PORT *port;

  // configuration space related data
  int cfg_space_fd;
  uint32_t ecap_offset;
  uint32_t doe_offset;
  PCIE_IDE_CAP ide_cap;

  // kcbar related data
  uint8_t *mapped_kcbar_addr;
  int kcbar_fd;
  INTEL_KEYP_PCIE_STREAM_CAP stream_cap;

  // ide_id
  uint8_t ide_id;

  // rid/addr assoc_reg_block
  PCIE_SEL_IDE_RID_ASSOC_REG_BLOCK rid_assoc_reg_block;
  PCIE_SEL_IDE_ADDR_ASSOC_REG_BLOCK addr_assoc_reg_block;

  // cxl related data
  CXL_PRIV_DATA cxl_data;
} ide_common_test_port_context_t;

typedef struct _ide_common_test_switch_internal_conn_context_t ide_common_test_switch_internal_conn_context_t;
struct _ide_common_test_switch_internal_conn_context_t {
  ide_common_test_switch_internal_conn_context_t *next;
  int switch_id;
  ide_common_test_port_context_t ups;
  ide_common_test_port_context_t dps;
};

typedef enum {
  TEEIO_TEST_RESULT_NOT_TESTED = 0,
  TEEIO_TEST_RESULT_PASS = 1,
  TEEIO_TEST_RESULT_FAILED = 2
} teeio_test_result_t;

typedef enum {
  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_NA = 0,
  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_TEST,
  IDE_COMMON_TEST_CASE_ASSERTION_TYPE_SEPARATOR
} ide_common_test_case_assertion_type_t;

typedef struct _ide_run_test_case_assertion_result_t ide_run_test_case_assertion_result_t;
struct _ide_run_test_case_assertion_result_t{
  ide_run_test_case_assertion_result_t* next;

  ide_common_test_case_assertion_type_t type;

  int class_id;
  int case_id;
  int assertion_id;

  teeio_test_result_t result;
  char extra_data[MAX_LINE_LENGTH];
};

typedef enum {
  TEEIO_TEST_CONFIG_FUNC_SUPPORT = 0,
  TEEIO_TEST_CONFIG_FUNC_ENABLE,
  TEEIO_TEST_CONFIG_FUNC_DISABLE,
  TEEIO_TEST_CONFIG_FUNC_CHECK,
  TEEIO_TEST_CONFIG_FUNC_MAX
} teeio_test_config_func_t;

typedef struct _ide_run_test_config_item_result_t ide_run_test_config_item_result_t;
struct _ide_run_test_config_item_result_t {
  ide_run_test_config_item_result_t* next;

  int config_item_id;

  teeio_test_result_t results[TEEIO_TEST_CONFIG_FUNC_MAX];
};

typedef struct _ide_run_test_case_result_t ide_run_test_case_result_t;
struct _ide_run_test_case_result_t {
  ide_run_test_case_result_t* next;

  char class[MAX_NAME_LENGTH];
  char name[MAX_NAME_LENGTH];
  int class_id;
  int case_id;

  int total_passed;
  int total_failed;

  ide_run_test_config_item_result_t *config_item_result;
  ide_run_test_case_assertion_result_t *assertion_result;
};

typedef struct {
  teeio_test_result_t result;
  char extra_data[MAX_LINE_LENGTH];
} teeio_test_group_func_result_t;

typedef enum {
  TEEIO_TEST_GROUP_FUNC_SETUP = 0,
  TEEIO_TEST_GROUP_FUNC_TEARDOWN,
  TEEIO_TEST_GROUP_FUNC_MAX
} teeio_test_group_func_t;

typedef struct _ide_run_test_group_result_t ide_run_test_group_result_t;
struct _ide_run_test_group_result_t {
  ide_run_test_group_result_t* next;

  char name[MAX_NAME_LENGTH];

  int total_passed;
  int total_failed;

  teeio_test_group_func_result_t func_results[TEEIO_TEST_GROUP_FUNC_MAX];

  ide_run_test_case_result_t* case_result;
};

typedef struct _ide_run_test_config_result_t ide_run_test_config_result_t;
struct _ide_run_test_config_result_t {
  ide_run_test_config_result_t* next;
  char name[MAX_NAME_LENGTH];
  int config_id;

  int total_passed;
  int total_failed;

  ide_run_test_group_result_t* group_result;
};

typedef struct {
  uint32_t signature;
  IDE_TEST_CONFIG *test_config;
  int test_suite_id;
  TEEIO_TEST_CATEGORY test_category;

  ide_run_test_config_result_t* result;

} ide_common_test_suite_context_t;

typedef struct {
    uint16_t slot_id[PCIE_IDE_STREAM_DIRECTION_NUM][PCIE_IDE_SUB_STREAM_NUM];
} ide_key_set_t;

typedef struct {
  // start of common part of test_group_context
  uint32_t signature;
  ide_common_test_suite_context_t *suite_context;
  IDE_TEST_TOPOLOGY *top;
  int config_id;
  int case_class;

  ide_common_test_port_context_t root_port;
  ide_common_test_port_context_t upper_port;
  ide_common_test_port_context_t lower_port;

  ide_common_test_switch_internal_conn_context_t* sw_conn1;
  ide_common_test_switch_internal_conn_context_t* sw_conn2;
  // end of common part of test_group_context
} teeio_common_test_group_context_t;

typedef struct {
  void *spdm_context;
  uint32_t session_id;
  void *doe_context;
} spdm_doe_context_t;

typedef struct {
  teeio_common_test_group_context_t common;
  spdm_doe_context_t spdm_doe;

  uint8_t stream_id;
  uint8_t rp_stream_index;
  ide_key_set_t k_set;
} pcie_ide_test_group_context_t;

typedef struct {
  teeio_common_test_group_context_t common;

  spdm_doe_context_t spdm_doe;

  uint8_t stream_id;
} cxl_ide_test_group_context_t;

typedef struct {
  teeio_common_test_group_context_t common;

  spdm_doe_context_t spdm_doe;
} cxl_tsp_test_group_context_t;

typedef struct {
  teeio_common_test_group_context_t common;
  spdm_doe_context_t spdm_doe;
} spdm_test_group_context_t;

typedef struct {
  uint32_t signature;
  ide_common_test_suite_context_t *suite_context;
  void *group_context;

  IDE_TEST_TOPOLOGY_TYPE top_type;
} ide_common_test_config_context_t;

// test config enable function which is to enable some function
typedef bool(*ide_common_test_config_enable_func_t) (void *test_context);
// test config disable function which is to disable some function
typedef bool(*ide_common_test_config_disable_func_t) (void *test_context);
// test config support function which is to check if some funciton is supported
typedef bool(*ide_common_test_config_support_func_t) (void *test_context);
// test config support function which is to check if some funciton is successfully enabled/disabled
typedef bool(*ide_common_test_config_check_func_t) (void *test_context);

typedef struct _ide_run_test_config_item_t ide_run_test_config_item_t;
struct _ide_run_test_config_item_t {
  ide_run_test_config_item_t *next;

  TEEIO_TEST_CATEGORY test_category;
  uint8_t type;

  ide_common_test_config_enable_func_t enable_func;
  ide_common_test_config_disable_func_t disable_func;
  ide_common_test_config_support_func_t support_func;
  ide_common_test_config_check_func_t check_func;
};

typedef struct _ide_run_test_config ide_run_test_config_t;
struct _ide_run_test_config {
  ide_run_test_config_t *next;
  int config_id;
  char name[MAX_NAME_LENGTH];
  void *test_context;

  ide_run_test_config_item_t* config_item;
};

typedef struct {
  ide_common_test_config_enable_func_t enable;
  ide_common_test_config_disable_func_t disable;
  ide_common_test_config_support_func_t support;
  ide_common_test_config_check_func_t check;
} ide_test_config_funcs_t;


// test case setup function
typedef bool(*ide_common_test_case_setup_func_t) (void *test_context);
// test case teardown function
typedef void(*ide_common_test_case_teardown_func_t) (void *test_context);
// test case run function
typedef void(*ide_common_test_case_run_func_t) (void *test_context);

typedef struct _ide_run_test_case ide_run_test_case_t;
struct _ide_run_test_case {
  ide_run_test_case_t *next;
  char class[MAX_NAME_LENGTH];
  char name[MAX_NAME_LENGTH];

  int class_id;
  int case_id;

  void *test_context;
  // indicates if the config_check is required in the test case
  bool config_check_required;

  ide_common_test_case_run_func_t run_func;
  ide_common_test_case_setup_func_t setup_func;
  ide_common_test_case_teardown_func_t teardown_func;
};

typedef struct {
  uint32_t signature;
  void *group_context;

  ide_run_test_case_t* test_case;

  IDE_COMMON_TEST_ACTION action;
} ide_common_test_case_context_t;

typedef struct {
  ide_common_test_case_setup_func_t setup;
  ide_common_test_case_run_func_t run;
  ide_common_test_case_teardown_func_t teardown;
  bool config_check_required;
} ide_test_case_funcs_t;

typedef struct {
  char *class;
  char *names;
  int class_id;
} ide_test_case_name_t;

typedef bool(*ide_common_test_group_setup_func_t) (void *test_context);
typedef bool(*ide_common_test_group_teardown_func_t) (void *test_context);

typedef struct _ide_run_test_group ide_run_test_group_t;
struct _ide_run_test_group {
  ide_run_test_group_t *next;
  char name[MAX_NAME_LENGTH];
  void *test_context;

  ide_run_test_case_t *test_case;
  ide_common_test_group_setup_func_t setup_func;
  ide_common_test_group_teardown_func_t teardown_func;
};

typedef struct {
  ide_common_test_group_setup_func_t setup;
  ide_common_test_group_teardown_func_t teardown;
} ide_test_group_funcs_t;

typedef const char*(*teeio_get_test_configuration_name_func_t) (int configuration_type);
typedef const char**(*teeio_get_test_configuration_priv_names_func_t) ();
typedef bool(*teeio_parse_test_configuration_priv_name_func_t) (const char* key, const char* value, IDE_TEST_CONFIGURATION* config);
typedef uint32_t(*teeio_get_test_configuration_bitmask_func_t) (int top_tpye);
typedef ide_test_config_funcs_t*(*teeio_get_test_configuration_funcs_func_t) (int top_type, int configuration_type);
typedef ide_test_group_funcs_t*(*teeio_get_test_group_funcs_func_t) (int top_type);
typedef ide_test_case_funcs_t*(*teeio_get_test_case_funcs_func_t) (int case_class, int case_id);
typedef ide_test_case_name_t*(*teeio_get_test_case_name_func_t) (int case_class);
typedef void*(*teeio_alloc_test_group_context_func_t)(void);
typedef bool(*teeio_check_configuration_bitmap_func_t) (uint32_t* bitmask);

typedef struct {
  teeio_get_test_configuration_name_func_t get_configuration_name_func;
  teeio_get_test_configuration_priv_names_func_t get_configuration_priv_names_func;
  teeio_parse_test_configuration_priv_name_func_t parse_configuration_priv_name_func;
  teeio_get_test_configuration_bitmask_func_t get_configuration_bitmask_func;
  teeio_get_test_configuration_funcs_func_t get_configuration_funcs_func;
  teeio_get_test_group_funcs_func_t get_group_funcs_func;
  teeio_get_test_case_funcs_func_t get_case_funcs_func;
  teeio_get_test_case_name_func_t get_case_name_func;
  teeio_alloc_test_group_context_func_t alloc_test_group_context_func;
  teeio_check_configuration_bitmap_func_t check_configuration_bitmap_func;
} teeio_test_funcs_t;

typedef struct _ide_run_test_suite ide_run_test_suite_t;
struct _ide_run_test_suite {
  ide_run_test_suite_t *next;
  char name[MAX_NAME_LENGTH];
  void *test_context;

  ide_run_test_group_t *test_group;
  ide_run_test_config_t *test_config;
};

typedef struct {
  ide_test_case_funcs_t* funcs;
  int cnt;
} TEEIO_TEST_CASES;

#endif
