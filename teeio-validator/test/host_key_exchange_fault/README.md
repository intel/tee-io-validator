# B4 KEY_EXCHANGE host regression and integration handoff

**Integration update (2026-09-15):** production dispatch/CMake and the strict
zero-fire control exit policy are now integrated. The campaign's explicit
control oracle is also present. The original handoff notes below describe the
earlier driver-only scope, not outstanding dispatch work. DUT-root provisioning
remains an integration requirement.
Use `build-pqc-driver` as the optional build argument for the verified current
requester configuration.

Run [run.py](run.py) with Python 3 from any directory. An optional first argument
selects an existing validator build; the default is `build-pqc-current`.
Requires GCC and that build's OpenSSL/static support libraries. The script
compiles the current requester/responder/common/crypto/sample-secret sources,
FI, and the owned driver with the build's flags including `-Werror`. It neither
configures CMake nor modifies other sources. Artifacts go under ignored .build/.
The campaign INI checks run when `TEEIO_FAULT_CATALOG_DIR` names the campaign
INI directory.
Sample certificates/private keys are **host fixtures only**, not DUT trust.

## Results

28 checks pass: SPDM 1.2 and 1.4, each with cached-certificate success,
duplicate success, missing trust, corrupted cached-flow response signature,
repeated full session ID, corrupted second duplicate response signature,
truncated first response, wrong second response version, second response
SESSION_LIMIT_EXCEEDED, non-firing occurrence, wrong FI action, duplicate
receive timeout, recovery failure, and setup failure. Two INI checks also pass.

Successful duplicate: requester half `0xffff`; composed IDs `0xffffffff` and
`0xfffeffff`. The harness verifies byte-identical DOE requests and two live
remote sessions at the second RSP. The driver returns the second response to
the real requester, which validates its signature and verify_data and completes
FINISH/END_SESSION. The first response is checked for complete expected wire
shape, not independently cryptographically authenticated. Explicit GET_VERSION
recovery clears the orphan handshake; a fresh authenticated session plus a
final VERSION reset checks recovery. Both peer tables are empty afterwards.

The cached-certificate control retrieves and verifies the responder chain in
setup, requires a nonempty trust anchor (unprovisioned/default acceptance is
insufficient), and completes an initial authenticated handshake. GET_VERSION
then renegotiates the same context. The target connection sends no
GET_CERTIFICATE, completes authenticated KEY_EXCHANGE/FINISH/END_SESSION, and
fires **zero** faults. A bad KEY_EXCHANGE_RSP signature fails this control.
FINISH does not contain a responder signature: KEY_EXCHANGE_RSP does.

## Original integration handoff (now completed; retained for design context)

The new [driver](../../library/spdm_test_lib/test_case/test_case_fault_key_exchange.c)
exports these exact signatures:

- `bool spdm_test_case_fault_key_exchange_setup(void *test_context);`
- `void spdm_test_case_fault_key_exchange_run(void *test_context);`
- `void spdm_test_case_fault_key_exchange_teardown(void *test_context);`

Add the source to the [library build](../../library/spdm_test_lib/CMakeLists.txt).
Declare those prototypes in the owning integration header and route **all three**
Fault.1 entry points to them only for `B4_duplicate_key_exchange` and
`B4_key_exchange_before_certificate`. Do not call the generic fault setup first.
The new teardown restores saved callbacks/app data, frees its buffers, then calls
common teardown. Invoke it on setup failures too. Do not use common teardown
alone: it would skip VERSION recovery and callback/buffer cleanup.

Both owned INIs now select Fault.1. They are not executable through the production
binary until that dispatch/build integration is made. P384/SHA384, no mutual
authentication, no measurement summary, and encrypted handshakes are intentional
constraints. Duplicate success requires capacity for two handshaking sessions;
SESSION_LIMIT_EXCEEDED does not pass this two-RSP coverage test, but is **not**
reported as a protocol violation.

### Positive-control campaign metadata is still required

The campaign currently requires `fired=true` and a fault `actual`. FI's actual
recorder ignores writes without a fired rule. Therefore the cached control must
not fabricate a fire or call that recorder to impersonate a negative test.
Its INI rule is only the existing scenario selector and is never applied.

Recommended separate integration:

1. Add an explicit campaign classification such as `kind=positive_control`
   and `required_fire_count=0`; retain negative/robustness defaults for other
   scenarios. The present INI comments describe this; unsupported keys were
   deliberately not invented as though the campaign already implemented them.
2. Emit/parse a dedicated control result with scenario, pass/fail, authenticated
   handshake outcome, target GET_CERTIFICATE count, actual fire count, and
   VERSION recovery outcome. Require validator success and the control assertion;
   never treat absence of injection alone as a pass.
3. Exclude this case from negative-injection coverage totals and baseline catalog
   expectations. Include it in positive-control totals. Reconcile old B4 blocked
   entries/UnexpectedRequest oracles in campaign documentation/catalog fixtures.

## Verified libspdm contracts and responder recommendation

The checked-out [common API](../../../spdm-emu/libspdm/include/library/spdm_common_lib.h)
has **no** `LIBSPDM_DATA_PEER_PUBLIC_CERT_CHAIN` enum. The
[changelog](../../../spdm-emu/libspdm/doc/changelog.md) marks it unsupported.
The supported setter is `LIBSPDM_DATA_PEER_USED_CERT_CHAIN_BUFFER`, CONNECTION
location, `additional_data[0]=0` for slot zero. It consumes the SPDM-formatted
chain and copies it (record-transcript mode) or caches its hash and parsed leaf
key (current configuration). It does not establish trust. Only the chain
already authenticated in setup is supplied, after renegotiating pinned algorithms.
`libspdm_get_certificate_ex` takes an extra block-length argument; zero selects
the default and the last two output arguments expose the validated trust anchor.

[Session composition/allocation](../../../spdm-emu/libspdm/library/spdm_common_lib/libspdm_com_context_data_session.c)
uses `(rsp_session_id << 16) | req_session_id`; zero is INVALID_SESSION_ID.
The [requester allocator](../../../spdm-emu/libspdm/library/spdm_requester_lib/libspdm_req_common.c)
chooses a requester half from a free table slot. The
[responder allocator](../../../spdm-emu/libspdm/library/spdm_responder_lib/libspdm_rsp_common.c)
independently chooses a responder half from a free slot. The common assignment
routine rejects a duplicate **full composed ID**. The
[KEX responder](../../../spdm-emu/libspdm/library/spdm_responder_lib/libspdm_rsp_key_exchange_rsp.c)
requires NEGOTIATED, not AFTER_CERTIFICATE, and does not reject an already-used
requester half. Thus another RSP with a fresh responder half is supported;
UnexpectedRequest is not the duplicate oracle.

The actual duplicate host exchange produces distinct IDs; no same-composed-ID
allocation defect was reproduced in these sources. No responder change is
justified solely because a second RSP was returned. The `same-id` host case
deliberately changes response bytes to test detection; it is not evidence that
a responder actually allocates the same ID.

If DUT evidence independently confirms concurrent allocation of the same full
ID, the minimal responder-side correction is to fix responder-half allocation
to avoid every live upper-16 value, preserve the existing full-ID guard before
assignment, and use the handler's SessionLimitExceeded path if allocation cannot
succeed. Do not reject all repeated lower-16 values or add an invented
UnexpectedRequest rule. Confirm that the first handshake has not legitimately
expired/reset before interpreting reused IDs. Keep that fix in a separate
responder-owned change; none was applied here.

No hardware, flash, commits, main fault driver, headers, or CMake edits.