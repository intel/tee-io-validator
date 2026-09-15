#!/usr/bin/env python3
"""Actual C catalog parsing and result/exit policy tests with no hardware calls."""
from pathlib import Path
import configparser
import os
import shlex
import subprocess
import sys

here = Path(__file__).resolve().parent
validator = here.parents[1]
build = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else validator / "build-pqc-driver"
out = here / ".build"
out.mkdir(exist_ok=True)
flags = []
for line in (build / "teeio_validator/CMakeFiles/teeio_validator.dir/flags.make").read_text().splitlines():
    if line.startswith(("C_DEFINES =", "C_INCLUDES =", "C_FLAGS =")):
        flags.extend(shlex.split(line.split("=", 1)[1]))
flags = [f for f in flags if f not in ("-flto", "-DUSING_LTO")]
obj = out / "integration_test.o"
subprocess.run(["gcc", *flags, "-Werror", "-c", str(here / "integration_test.c"), "-o", str(obj)], check=True)
command = shlex.split((build / "teeio_validator/CMakeFiles/teeio_validator.dir/link.txt").read_text())
command = [str(obj) if part == "CMakeFiles/teeio_validator.dir/teeio_validator.c.o" else part
           for part in command if part != "CMakeFiles/teeio_validator.dir/ide_test.c.o"]
binary = out / "integration_test"
command[command.index("-o") + 1] = str(binary)
subprocess.run(command, cwd=build / "teeio_validator", check=True)
# Campaign INIs live with the campaign, outside the validator.
if "TEEIO_FAULT_CATALOG_DIR" not in os.environ:
    raise SystemExit("set TEEIO_FAULT_CATALOG_DIR to the campaign INI directory")
catalog = Path(os.environ["TEEIO_FAULT_CATALOG_DIR"])

def run(ini, driver, mode=None, expected=255):
    env = dict(os.environ)
    env.pop("TEEIO_HOST_MODE", None)
    if mode is not None:
        env["TEEIO_HOST_MODE"] = mode
    result = subprocess.run([str(binary), "-f", str(ini), "-t", "1", "-c", "1",
                             "-s", driver, "-l", "verbose"],
                            cwd=out, env=env, text=True, capture_output=True)
    assert "HOST: parsed catalog; PCI execution replaced" in result.stdout, result.stdout + result.stderr
    assert result.returncode == expected, (ini, mode, result.returncode, result.stdout, result.stderr)
    return result.stdout

files = sorted(catalog.glob("*.ini"))
assert len(files) == 34
for ini in files:
    config = configparser.ConfigParser()
    config.read(ini)
    driver = config.get("Campaign", "driver", fallback="Version.1")
    run(ini, driver)
print("PASS: all 34 INIs through actual C parser/CLI/driver registration; no PCI execution")
control = catalog / "B4_key_exchange_before_certificate.ini"
run(control, "Fault.1", "pass", 0)
for mode in ("skipped", "no-primary", "failed", "not-tested", "teardown",
             "wrong-class", "missing-driver", "non-spdm", "two-controls", "extra-rule", "empty"):
    run(control, "Fault.1", mode)
    print(f"PASS: B4 zero-fire cannot pass with {mode}")
for scenario in ("B4_duplicate_key_exchange", "B5_invalid_finish_signature",
                 "B5_certificate_chunk_bit_flip", "B6_chunk_exceeds_transfer_size",
                 "B6_abandon_then_fresh_request", "B6_missing_final_chunk"):
    run(catalog / (scenario + ".ini"), "Fault.1", "pass")
    print(f"PASS: zero-fire exemption denied to {scenario}")
mut_auth = catalog / "B5_invalid_finish_signature.ini"
run(mut_auth, "Fault.1", "fired-failed")
run(mut_auth, "Fault.1", "fired-pass", 0)
print("PASS: real FI firing cannot hide failed recovery assertions; passing driver + fire exits 0")
print("PASS: 20 exit-policy checks (synthetic records, not protocol/DUT evidence)")