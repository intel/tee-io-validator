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
                    ${PROJECT_SOURCE_DIR}/include
                    ${TEEIO_VALIDATOR_LIB_DIR}/pcie_ide_lib/include
)

SET(src_pcie_ide_lib
  intel_rp_pcie.c
  pcie_ide.c
  scan_pcie.c
)

SET(pcie_ide_test_lib_LIBRARY
  debuglib
  helperlib
  # spdmlib
  # spdm_requester_lib
  # spdm_common_lib
  # ${CRYPTO_LIB_PATHS}
  # rnglib
  # cryptlib_${CRYPTO}
  # malloclib
  # spdm_crypt_lib
  # spdm_crypt_ext_lib
  # spdm_secured_message_lib
  # spdm_transport_pcidoe_lib
  # spdm_device_secret_lib_sample
  # pci_doe_requester_lib
  # pci_ide_km_requester_lib
  # pci_tdisp_requester_lib
  # platform_lib
)

ADD_LIBRARY(pcie_ide_lib STATIC ${src_pcie_ide_lib})
TARGET_LINK_LIBRARIES(pcie_ide_lib ${pcie_ide_lib_LIBRARY})
