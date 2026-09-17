#!/usr/bin/env python3
"""Compile the owned driver with real FI/DOE/chunk-responder sources.

Requires an existing requester build (default: build-pqc-current). Does not
configure CMake, rebuild firmware, or access hardware. Outputs stay in .build.
"""
from pathlib import Path
import configparser
import os
import shlex
import subprocess
import sys

here = Path(__file__).resolve().parent
validator = here.parents[1]
libspdm = validator.parent / "spdm-emu/libspdm"
build = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else validator / "build-pqc-current"
flags_file = build / "out/spdm_test_lib.lib/CMakeFiles/spdm_test_lib.dir/flags.make"
flags = []
for line in flags_file.read_text().splitlines():
    if line.startswith(("C_DEFINES =", "C_INCLUDES =", "C_FLAGS =")):
        flags.extend(shlex.split(line.split("=", 1)[1]))
flags = [flag for flag in flags if flag not in ("-flto", "-DUSING_LTO")]
out = here / ".build"
out.mkdir(exist_ok=True)
sources = [
    here / "chunk_fault_test.c",
    validator / "library/helperlib/fault_injection.c",
    libspdm / "library/spdm_responder_lib/libspdm_rsp_chunk_send_ack.c",
    libspdm / "library/spdm_responder_lib/libspdm_rsp_error.c",
]
objects = []
for source in sources:
    obj = out / (source.stem + ".o")
    subprocess.run(["gcc", *flags, "-c", str(source), "-o", str(obj)], check=True)
    objects.append(str(obj))
libs = [str(build / "lib" / ("lib" + name + ".a")) for name in (
    "spdm_transport_pcidoe_lib", "spdm_secured_message_lib", "spdm_common_lib",
    "spdm_crypt_lib", "spdm_crypt_ext_lib", "cryptlib_openssl", "memlib",
    "debuglib", "malloclib", "rnglib", "platform_lib_null",
    "spdm_device_secret_lib_null",
)]
crypto = build / "out/openssllib.lib/openssl-install/lib64/libcrypto.a"
if not crypto.exists():
    crypto = build / "out/openssllib.lib/openssl-install/lib/libcrypto.a"
binary = out / "chunk_fault_test"
subprocess.run(["gcc", "-Wl,--gc-sections", *objects, "-Wl,--start-group", *libs,
                str(crypto), "-Wl,--end-group", "-ldl", "-lpthread", "-o", str(binary)], check=True)
subprocess.run([str(binary)], check=True, timeout=60)

# Campaign INIs live with the campaign, outside the validator.
catalog = os.environ.get("TEEIO_FAULT_CATALOG_DIR")
if catalog is None:
    print("SKIP: campaign INI checks (set TEEIO_FAULT_CATALOG_DIR)")
else:
    configs = Path(catalog)
    config = configparser.ConfigParser()
    assert config.read(configs / "B5_certificate_chunk_bit_flip.ini")
    assert config["Campaign"]["driver"] == "Fault.1"
    assert config["Campaign"]["allowed_actual"] == "spdm_error_0x05"
    rule = config["FaultRule_1"]
    assert rule["occurrence"] == "1" and int(rule["offset"], 0) == 12
    assert rule["action"] == "xor" and rule["pattern"] == "01"
    print("PASS: B5 candidate-slice configuration")

    for name, actual, action, code in [
        ("B6_abandon_then_fresh_request", "chunk_transfer_abandoned", "abandon", "0x86"),
        ("B6_chunk_exceeds_transfer_size", "spdm_error_0x01", "extend", "0x85"),
        ("B6_missing_final_chunk", "final_chunk_withheld", "drop", "0x85"),
    ]:
        config = configparser.ConfigParser()
        assert config.read(configs / (name + ".ini"))
        assert config["Campaign"]["driver"] == "Fault.1"
        assert config["Campaign"]["allowed_actual"] == actual
        assert config["TestSuite_1"]["Fault"] == "1"
        rule = config["FaultRule_1"]
        assert rule["doe_type"] == "plain_spdm"
        assert rule["occurrence"] == "1"
        assert rule["action"] == action and rule["spdm_code"] == code
        if name == "B6_chunk_exceeds_transfer_size":
            assert rule["size"] == "8"
            assert rule["pattern"] == "0000000000000000"
        assert "blocked_reason" not in rule and "runnable" not in rule
        assert rule.get("recovery", "none") == "none"
    print("PASS: three strict B6 configurations")
