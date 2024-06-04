cmake_minimum_required(VERSION 2.8.12)

ADD_COMPILE_OPTIONS(-Wno-unused-result -Werror)

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
)

SET(src_lside
    lside.c
    ide_common.c
    ${TEEIO_VALIDATOR_LIB_DIR}/pcie_ide_test_lib/pci_ide.c
    ${TEEIO_VALIDATOR_LIB_DIR}/pcie_ide_test_lib/scan_pci.c
    ${TEEIO_VALIDATOR_DIR}/ide_test_ini.c
    ${TEEIO_VALIDATOR_LIB_DIR}/pcie_ide_test_lib/rp_pci.c)

SET(lside_LIBRARY
    debuglib
    helperlib)

SET(src_setide
    setide.c
    ide_common.c
    ${TEEIO_VALIDATOR_LIB_DIR}/pcie_ide_test_lib/pci_ide.c
    ${TEEIO_VALIDATOR_LIB_DIR}/pcie_ide_test_lib/scan_pci.c
    ${TEEIO_VALIDATOR_DIR}/ide_test_ini.c
    ${TEEIO_VALIDATOR_LIB_DIR}/pcie_ide_test_lib/rp_pci.c)

SET(setide_LIBRARY
    debuglib
    helperlib)

ADD_EXECUTABLE(lside ${src_lside})
TARGET_LINK_LIBRARIES(lside ${lside_LIBRARY})

ADD_EXECUTABLE(setide ${src_setide})
TARGET_LINK_LIBRARIES(setide ${setide_LIBRARY})