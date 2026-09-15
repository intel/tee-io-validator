#!/usr/bin/env python3
"""Real libspdm B4 host integration tests; no hardware, firmware or CMake edits."""
from pathlib import Path
import concurrent.futures
import configparser
import os
import shlex
import subprocess
import sys

here = Path(__file__).resolve().parent
validator = here.parents[1]
lib = validator.parent / "spdm-emu/libspdm"
build = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else validator / "build-pqc-current"
out = here / ".build"
out.mkdir(exist_ok=True)
flags = []
for line in (build / "out/spdm_test_lib.lib/CMakeFiles/spdm_test_lib.dir/flags.make").read_text().splitlines():
    if line.startswith(("C_DEFINES =", "C_INCLUDES =", "C_FLAGS =")):
        flags.extend(shlex.split(line.split("=", 1)[1]))
flags = [f for f in flags if f not in ("-flto", "-DUSING_LTO")]
flags += ["-I" + str(lib / "include/hal"), "-DLIBSPDM_DEBUG_LIBSPDM_ASSERT_CONFIG=3",
          "-DLIBSPDM_DEBUG_LEVEL_CONFIG=LIBSPDM_DEBUG_ERROR"]
sources = []
for folder in ("library/spdm_common_lib", "library/spdm_crypt_lib",
               "library/spdm_requester_lib", "library/spdm_responder_lib",
               "os_stub/spdm_device_secret_lib_sample"):
    sources += sorted((lib / folder).glob("*.c"))
sources += [lib / "os_stub/debuglib/debuglib.c",
            validator / "library/helperlib/fault_injection.c",
            validator / "library/spdm_test_lib/test_case/test_case_fault_key_exchange.c",
            here / "key_exchange_test.c"]

def compile_source(source):
    obj = out / (source.stem + ".o")
    subprocess.run(["gcc", *flags, "-c", str(source), "-o", str(obj)], check=True)
    return str(obj)

with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
    objects = list(pool.map(compile_source, sources))
names = ("cryptlib_openssl", "memlib", "malloclib", "rnglib", "platform_lib_null",
         "spdm_crypt_ext_lib", "spdm_secured_message_lib", "spdm_transport_pcidoe_lib")
crypto = next((build / "out/openssllib.lib/openssl-install").glob("lib*/libcrypto.a"))
binary = out / "key_exchange_test"
subprocess.run(["gcc", "-Wl,--gc-sections", *objects, "-Wl,--start-group",
                *[str(build / "lib" / ("lib" + n + ".a")) for n in names],
                str(crypto), "-Wl,--end-group", "-lpthread", "-ldl", "-o", str(binary)], check=True)
cases = ("cache", "duplicate", "missing-trust", "bad-signature", "same-id",
         "duplicate-bad-signature", "truncated-first", "wrong-version", "session-limit",
         "no-fire", "wrong-action", "second-timeout", "recovery-fail", "setup-fail")
for version in ("12", "14"):
    for case in cases:
        log = out / (version + "-" + case + ".log")
        with log.open("w") as stream:
            result = subprocess.run([str(binary), version, case],
                                    cwd=lib / "unit_test/sample_key", stdout=stream, stderr=subprocess.STDOUT)
        if result.returncode:
            print(log.read_text()[-12000:])
            raise SystemExit(f"FAIL: SPDM {version} {case} (see {log})")
        print(f"PASS: SPDM {version} {case}")
# Campaign INIs live with the campaign, outside the validator.
catalog = os.environ.get("TEEIO_FAULT_CATALOG_DIR")
if catalog is None:
    print("SKIP: campaign INI checks (set TEEIO_FAULT_CATALOG_DIR)")
else:
    configs = Path(catalog)
    for name in ("B4_duplicate_key_exchange", "B4_key_exchange_before_certificate"):
        config = configparser.ConfigParser()
        assert config.read(configs / (name + ".ini"))
        assert config["Campaign"]["driver"] == "Fault.1"
        assert config["TestSuite_1"]["Fault"] == "1"
        rule = config["FaultRule_1"]
        assert rule["scenario"] == name and rule["doe_type"] == "plain_spdm"
        assert rule["action"] == "duplicate" and rule["spdm_code"] == "0xe4"
        assert rule["occurrence"] == "1" and "runnable" not in rule
        assert "blocked_reason" not in rule
    print("PASS: two B4 configurations (dedicated driver and explicit positive control)")