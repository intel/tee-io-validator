/**
 *  Copyright Notice:
 *  Copyright 2023-2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#ifndef __IDE_TEST_H__
#define __IDE_TEST_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/param.h>
#include "pcie.h"
#include "cxl.h"
#include "intel_keyp.h"

#define NOT_IMPLEMENTED(msg) \
  TEEIO_DEBUG((TEEIO_DEBUG_ERROR, "Not implemented - %s\n", msg)); \
  TEEIO_ASSERT(false)

#define MAX_SUPPORTED_PORTS_NUM     16
#define MAX_SUPPORTED_SWITCHES_NUM  16
#define MAX_SUPPORTED_SWITCH_PORTS_NUM  16
#define PORT_NAME_LENGTH 32
#define SWITCH_NAME_LENGTH 32
#define BDF_LENGTH 8
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

#define INVALID_IDE_ID  0xFF
#define INVALID_RP_STREAM_INDEX 0xFF
#define INVALID_SCAN_BUS 0xFF

// Follow doc/IdeKmTestCase
#define MAX_QUERY_CASE_ID 2
#define MAX_KEYPROG_CASE_ID 6
#define MAX_KSETGO_CASE_ID 4
#define MAX_KSETSTOP_CASE_ID 4
#define MAX_SPDMSESSION_CASE_ID 2
#define MAX_FULL_CASE_ID 1
#define MAX_PCIE_CASE_ID \
  (MAX(MAX_QUERY_CASE_ID, MAX(MAX_KEYPROG_CASE_ID, MAX(MAX_KSETGO_CASE_ID, MAX(MAX_KSETSTOP_CASE_ID, MAX(MAX_SPDMSESSION_CASE_ID, MAX_FULL_CASE_ID))))))

#define MAX_CXL_QUERY_CASE_ID 2
#define MAX_CXL_KEYPROG_CASE_ID 9
#define MAX_CXL_KSETGO_CASE_ID 1
#define MAX_CXL_KSETSTOP_CASE_ID 1
#define MAX_CXL_GETKEY_CASE_ID 1
#define MAX_CXL_FULL_CASE_ID 2
#define MAX_CXL_CASE_ID \
  (MAX(MAX_CXL_QUERY_CASE_ID, MAX(MAX_CXL_KEYPROG_CASE_ID, MAX(MAX_CXL_KSETGO_CASE_ID, MAX(MAX_CXL_KSETSTOP_CASE_ID, MAX(MAX_CXL_GETKEY_CASE_ID, MAX_CXL_FULL_CASE_ID))))))

#define MAX_CASE_ID \
  (MAX(MAX_PCIE_CASE_ID, MAX_CXL_CASE_ID))

#define INVALID_PORT_ID 0

#define MAIN_SECION "Main"
#define MAIN_SECTION_PCI_LOG "pci_log"

#define PORTS_SECTION "Ports"
#define TOPOLOGY_SECTION "Topology_%d"
#define CONFIGURATION_SECTION "Configuration_%d"
#define TEST_SUITE_SECTION "TestSuite_%d"

typedef enum {
  IDE_TEST_CATEGORY_PCIE = 0,
  IDE_TEST_CATEGORY_CXL_MEMCACHE,
  IDE_TEST_CATEGORY_NUM
} IDE_TEST_CATEGORY;

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

typedef enum {
  CXL_MEM_IDE_TEST_CASE_QUERY = 0,
  CXL_MEM_IDE_TEST_CASE_KEYPROG,
  CXL_MEM_IDE_TEST_CASE_KSETGO,
  CXL_MEM_IDE_TEST_CASE_KSETSTOP,
  CXL_MEM_IDE_TEST_CASE_GETKEY,
  CXL_MEM_IDE_TEST_CASE_TEST,
  CXL_MEM_IDE_TEST_CASE_NUM
} CXL_MEM_IDE_TEST_CASE;

typedef struct
{
  int id;
  bool enabled;
  IDE_PORT_TYPE port_type;
  char port_name[PORT_NAME_LENGTH];
  char bdf[BDF_LENGTH];
  uint8_t bus;
  uint8_t device;
  uint8_t function;
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
  // IDE_TEST_CATEGORY test_category;
  IDE_TEST_TOPOLOGY_TYPE type;
  IDE_TEST_CONNECT_TYPE connection;
  int root_port;
  int upper_port;
  int lower_port;
  IDE_SWITCH_INTERNAL_CONNECTION *sw_conn1;
  IDE_SWITCH_INTERNAL_CONNECTION *sw_conn2;
  uint8_t bus;
  uint8_t stream_id;
  uint8_t rp_stream_index;
} IDE_TEST_TOPOLOGY;

typedef struct {
  int cnt;
  IDE_TEST_TOPOLOGY topologies[MAX_TOPOLOGY_NUM];
} IDE_TEST_TOPOLOGYS;

typedef struct {
  int id;
  bool enabled;
  IDE_TEST_TOPOLOGY_TYPE type;
  uint32_t bit_map;
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
  IDE_TEST_CASE cases[IDE_COMMON_TEST_CASE_NUM];
} IDE_TEST_CASES;

typedef struct {
  int id;
  bool enabled;
  IDE_TEST_TOPOLOGY_TYPE type;
  int topology_id;
  int configuration_id;
  IDE_TEST_CATEGORY test_category;
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
  IDE_TEST_CONFIGURATION_TYPE_NUM
} IDE_TEST_CONFIGURATION_TYPE;

typedef enum {
  CXL_IDE_CONFIGURATION_TYPE_DEFAULT = 0,
  CXL_IDE_CONFIGURATION_TYPE_PCRC,
  CXL_IDE_CONFIGURATION_TYPE_IDE_STOP,
  CXL_IDE_CONFIGURATION_TYPE_NUM
} CXL_IDE_CONFIGURATION_TYPE;

#define BIT_MASK(n) ((uint32_t)(1<<n))

#define SELECTIVE_IDE_CONFIGURATION_BITMASK \
  ((BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_DEFAULT)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_SWITCH)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_PARTIAL_HEADER_ENC)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_PCRC)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_AGGGEG)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_SELECTIVE_IDE_FOR_CONFIG)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_TEE_LIMITED_STREAM)))

#define LINK_IDE_CONFIGURATION_BITMASK \
  ((BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_DEFAULT)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_SWITCH)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_PARTIAL_HEADER_ENC)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_PCRC)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_AGGGEG)))

#define SELECTIVE_LINK_IDE_CONFIGURATION_BITMASK \
  ((BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_DEFAULT)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_PARTIAL_HEADER_ENC)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_PCRC)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_SELECTIVE_IDE_FOR_CONFIG)) | \
  (BIT_MASK(IDE_TEST_CONFIGURATION_TYPE_AGGGEG)))

typedef enum {
  IDE_COMMON_TEST_ACTION_RUN = 0,
  IDE_COMMON_TEST_ACTION_SKIP = 1
} IDE_COMMON_TEST_ACTION;

typedef enum {
  IDE_COMMON_TEST_CASE_RESULT_SKIPPED = 0,
  IDE_COMMON_TEST_CASE_RESULT_SUCCESS = 1,
  IDE_COMMON_TEST_CASE_RESULT_FAILED = 2  
} IDE_COMMON_TEST_CASE_RESULT;

typedef enum {
  IDE_COMMON_TEST_CONFIG_RESULT_NA = 0,
  IDE_COMMON_TEST_CONFIG_RESULT_SUCCESS = 1,
  IDE_COMMON_TEST_CONFIG_RESULT_FAILED = 2  
} IDE_COMMON_TEST_CONFIG_RESULT;

typedef struct {
  INTEL_KEYP_PCIE_STREAM_CAP stream_cap;

  // ecap related data
  PCIE_IDE_CAP ide_cap;
  // ide_id
  uint8_t ide_id;
  // rid/addr assoc_reg_block
  PCIE_SEL_IDE_RID_ASSOC_REG_BLOCK rid_assoc_reg_block;
  PCIE_SEL_IDE_ADDR_ASSOC_REG_BLOCK addr_assoc_reg_block;
} PCIE_PRIV_DATA;

#define MAX_IDE_TEST_DVSEC_COUNT  16
typedef struct {
  uint32_t offset;          // offset in configuration space
  CXL_DVSEC_ID dvsec_id;    // DVSEC ID. 0xff is invalid DVSEC
} IDE_TEST_CXL_PCIE_DVSEC;

typedef struct {
  // DVSECs in Configuration Space
  IDE_TEST_CXL_PCIE_DVSEC dvsecs[MAX_IDE_TEST_DVSEC_COUNT];
  int dvsec_cnt;

  CXL_DEV_CAPABILITY cap;
  CXL_DEV_CAPABILITY2 cap2;
  CXL_DEV_CAPABILITY3 cap3;
} CXL_PRIV_DATA_ECAP;

typedef struct {
  // KCBar data
  INTEL_KEYP_CXL_LINK_ENC_GLOBAL_CONFIG link_enc_global_config;
} CXL_PRIV_DATA_KCBAR;

typedef struct {
  int mapped_fd;
  uint8_t* mapped_memcache_reg_block;

  CXL_CAPABILITY_XXX_HEADER cap_headers[CXL_CAPABILITY_ID_NUM];
  int cap_headers_cnt;

  CXL_IDE_CAPABILITY ide_cap;
} CXL_PRIV_DATA_MEMCACHE_REG_DATA;

typedef struct {
  CXL_PRIV_DATA_ECAP ecap;
  CXL_PRIV_DATA_KCBAR kcbar;
  CXL_PRIV_DATA_MEMCACHE_REG_DATA memcache;
} CXL_PRIV_DATA;

typedef union {
  PCIE_PRIV_DATA  pcie;
  CXL_PRIV_DATA cxl;
} CXL_PCIE_PRIV_DATA;

typedef struct {
  IDE_PORT *port;

  // configuration space related data
  int cfg_space_fd;
  uint32_t ecap_offset;
  uint32_t doe_offset;

  // kcbar related data
  uint8_t *mapped_kcbar_addr;
  int kcbar_fd;
    
  CXL_PCIE_PRIV_DATA priv_data;
} ide_common_test_port_context_t;

typedef struct _ide_common_test_switch_internal_conn_context_t ide_common_test_switch_internal_conn_context_t;
struct _ide_common_test_switch_internal_conn_context_t {
  ide_common_test_switch_internal_conn_context_t *next;
  int switch_id;
  ide_common_test_port_context_t ups;
  ide_common_test_port_context_t dps;
};

typedef struct {
  uint32_t signature;
  IDE_TEST_CONFIG *test_config;
  int test_suite_id;
  IDE_TEST_CATEGORY test_category;
} ide_common_test_suite_context_t;

typedef struct {
    uint16_t slot_id[PCIE_IDE_STREAM_DIRECTION_NUM][PCIE_IDE_SUB_STREAM_NUM];
} ide_key_set_t;

typedef struct {
  uint32_t signature;
  ide_common_test_suite_context_t *suite_context;
  IDE_TEST_TOPOLOGY *top;

  void *spdm_context;
  uint32_t session_id;
  void *doe_context;

  ide_common_test_port_context_t root_port;
  ide_common_test_port_context_t upper_port;
  ide_common_test_port_context_t lower_port;

  ide_common_test_switch_internal_conn_context_t* sw_conn1;
  ide_common_test_switch_internal_conn_context_t* sw_conn2;

  uint8_t stream_id;
  uint8_t rp_stream_index;
  ide_key_set_t k_set[PCIE_IDE_STREAM_KS_NUM];
} ide_common_test_group_context_t;

typedef struct {
  uint32_t signature;
  ide_common_test_suite_context_t *suite_context;
  ide_common_test_group_context_t *group_context;

  IDE_TEST_TOPOLOGY_TYPE top_type;
  IDE_COMMON_TEST_CONFIG_RESULT test_result;
} ide_common_test_config_context_t;

typedef struct {
  uint32_t signature;
  ide_common_test_group_context_t *group_context;
  IDE_COMMON_TEST_ACTION action;
  IDE_COMMON_TEST_CASE_RESULT result;
} ide_common_test_case_context_t;

typedef struct _ide_run_test_case_result_t ide_run_test_case_result_t;
struct _ide_run_test_case_result_t {
  ide_run_test_case_result_t* next;

  char class[MAX_NAME_LENGTH];
  char name[MAX_NAME_LENGTH];
  IDE_COMMON_TEST_CASE_RESULT case_result;
  IDE_COMMON_TEST_CONFIG_RESULT config_result;
};

typedef struct _ide_run_test_group_result_t ide_run_test_group_result_t;
struct _ide_run_test_group_result_t {
  ide_run_test_group_result_t* next;

  char name[MAX_NAME_LENGTH];
  ide_run_test_case_result_t* case_result;
};

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

  IDE_TEST_CONFIGURATION_TYPE type;

  ide_common_test_config_enable_func_t enable_func;
  ide_common_test_config_disable_func_t disable_func;
  ide_common_test_config_support_func_t support_func;
  ide_common_test_config_check_func_t check_func;
};

typedef struct _ide_run_test_config ide_run_test_config_t;
struct _ide_run_test_config {
  ide_run_test_config_t *next;
  char name[MAX_NAME_LENGTH];
  void *test_context;

  ide_run_test_group_result_t* group_result;
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
typedef bool(*ide_common_test_case_teardown_func_t) (void *test_context);
// test case run function
typedef bool(*ide_common_test_case_run_func_t) (void *test_context);

typedef struct _ide_run_test_case ide_run_test_case_t;
struct _ide_run_test_case {
  ide_run_test_case_t *next;
  char class[MAX_NAME_LENGTH];
  char name[MAX_NAME_LENGTH];
  // IDE_COMMON_TEST_ACTION action;
  void *test_context;
  // indicates if the ide_stream is completed in the test case
  bool complete_ide_stream;

  ide_common_test_case_run_func_t run_func;
  ide_common_test_case_setup_func_t setup_func;
  ide_common_test_case_teardown_func_t teardown_func;
};

typedef struct {
  ide_common_test_case_setup_func_t setup;
  ide_common_test_case_run_func_t run;
  ide_common_test_case_teardown_func_t teardown;
  bool complete_ide_stream;
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

typedef struct _ide_run_test_suite ide_run_test_suite_t;
struct _ide_run_test_suite {
  ide_run_test_suite_t *next;
  char name[MAX_NAME_LENGTH];
  void *test_context;

  ide_run_test_group_t *test_group;
  ide_run_test_config_t *test_config;
};

typedef struct _teeio_test_config_funcs_t teeio_test_config_funcs_t;
struct _teeio_test_config_funcs_t {
  teeio_test_config_funcs_t* next;

  bool head;
  int cnt;

  IDE_TEST_CATEGORY test_category;
  IDE_TEST_TOPOLOGY_TYPE top_type;
  IDE_TEST_CONFIGURATION_TYPE config_type;

  ide_test_config_funcs_t funcs;
};

typedef struct _teeio_test_group_funcs_t teeio_test_group_funcs_t;
struct _teeio_test_group_funcs_t {
  teeio_test_group_funcs_t* next;

  bool head;
  int cnt;

  IDE_TEST_CATEGORY test_category;
  IDE_TEST_TOPOLOGY_TYPE top_type;

  ide_test_group_funcs_t funcs;
};

typedef struct _teeio_test_case_funcs_t teeio_test_case_funcs_t;
struct _teeio_test_case_funcs_t {
  teeio_test_case_funcs_t* next;

  bool head;
  int cnt;

  IDE_TEST_CATEGORY test_category;

  int test_case;
  int case_id;

  ide_test_case_funcs_t funcs;
};

#endif
