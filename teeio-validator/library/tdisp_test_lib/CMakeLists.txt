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
                    ${TEEIO_VALIDATOR_LIB_DIR}/pcie_ide_lib/include
                    ${TEEIO_VALIDATOR_LIB_DIR}/tdisp_test_lib/include
)

SET(src_tdisp_test_lib
  tdisp_common.c
  # test_case
  test_case/ide_test_common.c
  test_case/tdisp_test_common.c
  test_case/test_case_version_1.c
  test_case/test_case_capabilities_1.c
  test_case/test_case_lock_interface_1.c
  test_case/test_case_lock_interface_2.c
  test_case/test_case_lock_interface_3.c
  test_case/test_case_lock_interface_4.c
  test_case/test_case_interface_report_1.c
  test_case/test_case_interface_report_2.c
  test_case/test_case_interface_report_3.c
  test_case/test_case_interface_report_4.c
  test_case/test_case_interface_report_5.c
  test_case/test_case_interface_state_1.c
  test_case/test_case_interface_state_2.c
  test_case/test_case_interface_state_3.c
  test_case/test_case_interface_state_4.c
  test_case/test_case_start_interface_1.c
  test_case/test_case_start_interface_2.c
  test_case/test_case_start_interface_3.c
  test_case/test_case_start_interface_4.c
  test_case/test_case_stop_interface_1.c
  test_case/test_case_stop_interface_2.c
  test_case/test_case_stop_interface_3.c
  ## test configs
  test_config/test_config_default.c
  ## test groups
  test_group/test_group.c
)

SET(tdisp_test_lib_LIBRARY
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

ADD_LIBRARY(tdisp_test_lib STATIC ${src_tdisp_test_lib})
TARGET_LINK_LIBRARIES(tdisp_test_lib ${tdisp_test_lib_LIBRARY})
