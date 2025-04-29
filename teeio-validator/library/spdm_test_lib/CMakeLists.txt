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
                    ${COMMON_TEST_FRAMEWORK_DIR}/include
                    ${SPDM_RESPONDER_VALIDATOR_DIR}/include
                    ${TEEIO_VALIDATOR_LIB_DIR}/spdm_test_lib/include
)

SET(src_spdm_test_lib
  spdm_test_common.c
  spdm_test_case.c
  ## test configs
  test_config/test_config.c
  ## test groups
  test_group/test_group.c
)

SET(spdm_test_lib_LIBRARY
  memlib
  debuglib
  helperlib
  spdmlib
  pcie_ide_lib
  spdm_requester_lib
  spdm_common_lib
  spdm_responder_conformance_test_lib
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
  platform_lib
)

ADD_LIBRARY(spdm_test_lib STATIC ${src_spdm_test_lib})
TARGET_LINK_LIBRARIES(spdm_test_lib ${spdm_test_lib_LIBRARY})
