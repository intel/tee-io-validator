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
)

SET(src_teeio_validator
    teeio_validator.c
    rp_pci.c
    cmdline.c
    ide_test_ini.c
    ide_test.c
    ## test cases
    test_case/test_case_query.c
    test_case/test_case_ksetgo_1.c
    test_case/test_case_ksetgo_2.c
    test_case/test_case_ksetgo_3.c
    test_case/test_case_ksetgo_4.c
    test_case/ide_km_common.c
    test_case/test_case_keyprog_1.c
    test_case/test_case_keyprog_2.c
    test_case/test_case_keyprog_3.c
    test_case/test_case_keyprog_4.c
    test_case/test_case_keyprog_5.c
    test_case/test_case_keyprog_6.c
    test_case/test_case_ksetstop_1.c
    test_case/test_case_ksetstop_2.c
    test_case/test_case_ksetstop_3.c
    test_case/test_case_ksetstop_4.c
    test_case/test_case_full.c
    test_case/test_case_full_keyrefresh.c
    ## test configs
    test_config/test_config_common.c
    test_config/test_config_pcrc.c
    test_config/test_config_default.c
    test_config/test_config_sel_ide_for_cfg_req.c
    ## test groups
    test_group/test_group.c
    scan_pci.c
    pci_ide.c
)

SET(teeio_validator_LIBRARY
    memlib
    debuglib
    helperlib
    teeio_spdmlib
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

ADD_EXECUTABLE(teeio_validator ${src_teeio_validator})
TARGET_LINK_LIBRARIES(teeio_validator ${teeio_validator_LIBRARY})
