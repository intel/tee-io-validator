cmake_minimum_required(VERSION 2.8.12)

ADD_COMPILE_OPTIONS(-Wno-unused-result -Werror=format-overflow -Werror)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    ADD_COMPILE_OPTIONS(-Wno-error=unused-variable -Wno-error=unused-but-set-variable)
endif()

SET(BUILD_SHARED_LIBS OFF)
SET(CMAKE_EXE_LINKER_FLAGS "-static")

INCLUDE_DIRECTORIES(
                    ${LIBSPDM_DIR}/include
                    ${LIBSPDM_DIR}/include/hal
                    ${LIBSPDM_DIR}/os_stub/include/
                    ${PROJECT_SOURCE_DIR}/include
                    ${TEEIO_VALIDATOR_LIB_DIR}/helperlib/include
                    ${SPDM_EMU_DIR}/spdm_emu/spdm_emu_common
)

SET(src_helperlib
    ide_ini_helper.c
    pcie_helper.c
    utils.c
    pcap.c
    teeio_common.c
)

SET(helperlib_LIBRARY
    debuglib
)

ADD_LIBRARY(helperlib STATIC ${src_helperlib})
TARGET_LINK_LIBRARIES(helperlib ${helperlib_LIBRARY})
