cmake_minimum_required(VERSION 2.8.12)

ADD_COMPILE_OPTIONS(-Wno-unused-result -Werror=format-overflow -Werror)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    ADD_COMPILE_OPTIONS(-Wno-error=unused-variable -Wno-error=unused-but-set-variable)
endif()

SET(BUILD_SHARED_LIBS OFF)
SET(CMAKE_EXE_LINKER_FLAGS "-static")

INCLUDE_DIRECTORIES(${SPDM_DEVICE_DIR}/include
                    ${LIBSPDM_DIR}/os_stub/spdm_device_secret_lib_sample
                    ${LIBSPDM_DIR}/include
                    ${LIBSPDM_DIR}/os_stub/include
                    ${LIBSPDM_DIR}/os_stub
                    ${SPDM_EMU_DIR}/include
                    ${TEEIO_VALIDATOR_LIB_DIR}/pcie_ide_test_lib/include
)

SET(src_pcie_ide_test_lib
  pcie_ide_common.c
  # test_case
  test_case/test_case_query_1.c
  test_case/test_case_query_2.c
  test_case/test_case_ksetgo_1.c
  test_case/test_case_ksetgo_2.c
  test_case/test_case_ksetgo_3.c
  test_case/test_case_ksetgo_4.c
  test_case/ide_km_common.c
  test_case/test_case_keyprog_1.c
  test_case/test_case_keyprog_2.c
  test_case/test_case_keyprog_3.c
  test_case/test_case_keyprog_4.c
  test_case/test_case_keyprog_5.c
  test_case/test_case_keyprog_6.c
  test_case/test_case_ksetstop_1.c
  test_case/test_case_ksetstop_2.c
  test_case/test_case_ksetstop_3.c
  test_case/test_case_ksetstop_4.c
  test_case/test_case_full.c
  test_case/test_case_full_keyrefresh.c
  test_case/test_case_spdm_session_1.c
  test_case/test_case_spdm_session_2.c
  ## test configs
  test_config/test_config_common.c
  test_config/test_config_pcrc.c
  test_config/test_config_default.c
  test_config/test_config_sel_ide_for_cfg_req.c
  test_config/test_config_flit_mode_disable.c
  ## test groups
  test_group/test_group.c
)

SET(pcie_ide_test_lib_LIBRARY
  memlib
  debuglib
  helperlib
  spdmlib
  pcie_ide_lib
  spdm_requester_lib
  spdm_common_lib
  ${CRYPTO_LIB_PATHS}
  rnglib
  cryptlib_${CRYPTO}
  malloclib
  spdm_crypt_lib
  spdm_crypt_ext_lib
  spdm_secured_message_lib
  spdm_transport_pcidoe_lib
  spdm_device_secret_lib_sample
  pci_doe_requester_lib
  pci_ide_km_requester_lib
  pci_tdisp_requester_lib
  platform_lib
)

ADD_LIBRARY(pcie_ide_test_lib STATIC ${src_pcie_ide_test_lib})
TARGET_LINK_LIBRARIES(pcie_ide_test_lib ${pcie_ide_test_lib_LIBRARY})
