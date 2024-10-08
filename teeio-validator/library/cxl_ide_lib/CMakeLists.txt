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
                    ${TEEIO_VALIDATOR_LIB_DIR}/cxl_ide_lib/include
)

SET(src_cxl_ide_lib
  cxl_ide.c
)

SET(cxl_ide_lib_LIBRARY
  debuglib
  helperlib
  pcie_ide_lib
)

ADD_LIBRARY(cxl_ide_lib STATIC ${src_cxl_ide_lib})
TARGET_LINK_LIBRARIES(cxl_ide_lib ${cxl_ide_lib_LIBRARY})
