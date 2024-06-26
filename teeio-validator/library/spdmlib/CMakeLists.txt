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
                    ${SPDM_EMU_DIR}/include
                    ${PROJECT_SOURCE_DIR}/include
                    ${TEEIO_VALIDATOR_LIB_DIR}/helperlib/include
)

SET(src_spdmlib
    pci_doe.c
    spdm.c
    support.c
)

SET(spdmlib_LIBRARY
    debuglib
    helperlib
)

ADD_LIBRARY(spdmlib STATIC ${src_spdmlib})
TARGET_LINK_LIBRARIES(spdmlib ${spdmlib_LIBRARY})
