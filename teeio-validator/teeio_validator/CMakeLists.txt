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
                    ${SPDM_EMU_DIR}/spdm_emu/spdm_emu_common
                    ${TEEIO_VALIDATOR_DIR}/include
)

SET(src_teeio_validator
    teeio_validator.c
    cmdline.c
    ide_test_ini.c
    ide_test.c
    )

SET(teeio_validator_LIBRARY
    debuglib
    helperlib
    spdmlib
    pcie_ide_test_lib
    cxl_ide_test_lib
    cxl_tsp_test_lib
    )

ADD_EXECUTABLE(teeio_validator ${src_teeio_validator})
TARGET_LINK_LIBRARIES(teeio_validator ${teeio_validator_LIBRARY})
