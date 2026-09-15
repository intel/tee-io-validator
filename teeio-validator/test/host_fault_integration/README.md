# Offline requester integration and exit-policy tests

Run [run.py](run.py) with Python 3. An optional first argument selects the
existing complete OpenSSL CMake build; the default is `build-pqc-driver`.
`TEEIO_FAULT_CATALOG_DIR` must name the campaign INI directory.
The script uses its actual compiler flags, CLI/parser objects and link inputs.
Artifacts and logs remain in the ignored local build directory.

[integration_test.c](integration_test.c) includes the production entry point
and result evaluator, replacing only the entry's `run()` call. This prevents
PCI enumeration, device opening, and hardware writes. The standalone catalog
shell test normally attempts group/PCI setup, so it is deliberately not used
against the hardware-capable binary here.

Verified coverage:

- All 34 catalog INIs pass the actual C parser, CLI selection, and registered
  driver validation and reach the execution replacement. This is parsing
  evidence, **not 34 passing protocol scenarios**.
- Twenty result/exit checks: only a single explicit B4 cached-certificate
  Fault.1 with the primary assertion and all assertions passing qualifies
  for zero-fire success. Skip, missing primary, failed/not-tested assertions,
  failed group teardown, wrong assertion class, missing driver, wrong category,
  multiple control instances/rules and empty runs fail.
- All six other dedicated scenarios remain subject to the fire requirement.
- A real FI fire cannot hide a failed recovery assertion; fire plus a passing
  synthetic dedicated record produces the expected successful entry exit.

Synthetic records are intentional policy unit-test inputs, never emitted as
DUT evidence. Protocol evidence comes separately from the B4 and chunk host
harnesses.

The production `run()` now evaluates its saved per-case result tree **after
teardown and before freeing it**, and returns an explicit control boolean.
It does not read the already-cleared `g_current_case_result` or fabricate an
FI outcome for the positive control. Ordinary legacy negative-test oracles
remain unchanged; strict assertion gating applies to dedicated Fault.1 drivers.