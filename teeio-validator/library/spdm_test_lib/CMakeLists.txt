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
  # test_case
  test_case/spdm_test_algorithms_1.c
  test_case/spdm_test_algorithms_2.c
  test_case/spdm_test_algorithms_3.c
  test_case/spdm_test_algorithms_4.c
  test_case/spdm_test_algorithms_5.c
  test_case/spdm_test_algorithms_6.c
  test_case/spdm_test_algorithms_7.c
  test_case/spdm_test_capabilities_1.c
  test_case/spdm_test_capabilities_2.c
  test_case/spdm_test_capabilities_3.c
  test_case/spdm_test_capabilities_4.c
  test_case/spdm_test_capabilities_5.c
  test_case/spdm_test_capabilities_6.c
  test_case/spdm_test_certificate_1.c
  test_case/spdm_test_certificate_2.c
  test_case/spdm_test_certificate_3.c
  test_case/spdm_test_certificate_4.c
  test_case/spdm_test_end_session_1.c
  test_case/spdm_test_end_session_2.c
  test_case/spdm_test_end_session_3.c
  test_case/spdm_test_end_session_4.c
  test_case/spdm_test_finish_1.c
  test_case/spdm_test_finish_2.c
  test_case/spdm_test_finish_3.c
  test_case/spdm_test_finish_4.c
  test_case/spdm_test_finish_5.c
  test_case/spdm_test_finish_6.c
  test_case/spdm_test_finish_7.c
  test_case/spdm_test_finish_8.c
  test_case/spdm_test_finish_9.c
  test_case/spdm_test_finish_10.c
  test_case/spdm_test_finish_11.c
  test_case/spdm_test_key_exchange_1.c
  test_case/spdm_test_key_exchange_2.c
  test_case/spdm_test_key_exchange_3.c
  test_case/spdm_test_key_exchange_4.c
  test_case/spdm_test_key_exchange_5.c
  test_case/spdm_test_key_exchange_6.c
  test_case/spdm_test_key_exchange_7.c
  test_case/spdm_test_key_exchange_8.c
  test_case/spdm_test_measurements_1.c
  test_case/spdm_test_measurements_2.c
  test_case/spdm_test_measurements_3.c
  test_case/spdm_test_measurements_4.c
  test_case/spdm_test_measurements_5.c
  test_case/spdm_test_measurements_6.c
  test_case/spdm_test_measurements_7.c
  test_case/spdm_test_measurements_8.c
  test_case/spdm_test_measurements_9.c
  test_case/spdm_test_measurements_10.c
  test_case/spdm_test_version_1.c
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
