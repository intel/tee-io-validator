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
                    ${TEEIO_VALIDATOR_LIB_DIR}/cxl_tsp_test_lib/include
)

SET(src_cxl_tsp_test_lib
  # test_case
  test_case/test_case_get_version_1.c
  test_case/test_case_get_caps_1.c
  test_case/test_case_set_configuration_1.c
  test_case/test_case_get_configuration_1.c
  test_case/test_case_get_configuration_report_1.c
  test_case/test_case_lock_configuration_1.c
  test_case/test_case_lock_configuration_2.c
  ## test configs
  test_config/test_config.c
  ## test groups
  test_group/test_group.c
  cxl_tsp_common.c
)

SET(cxl_tsp_test_lib_LIBRARY
  memlib
  debuglib
  cxl_ide_lib
  helperlib
  spdmlib
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
  cxl_tsp_requester_lib
  platform_lib
)

ADD_LIBRARY(cxl_tsp_test_lib STATIC ${src_cxl_tsp_test_lib})
TARGET_LINK_LIBRARIES(cxl_tsp_test_lib ${cxl_tsp_test_lib_LIBRARY})
