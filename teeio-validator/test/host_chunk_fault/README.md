# Fault.1 chunk regressions

Run `python3 test/host_chunk_fault/run.py` from the validator directory. The
optional first argument selects an existing requester build directory instead
of `build-pqc-current`. The test uses that build's actual compile definitions
and protocol archives; it does not configure CMake, rebuild firmware, or access
hardware. Generated objects and the executable remain in ignored `.build/`.
The campaign INI checks run when `TEEIO_FAULT_CATALOG_DIR` names the campaign
INI directory.

## What is executed

- The production Fault.1 source, not a copied selector implementation.
- Real fault-injection code and real libspdm PCI DOE encode/decode.
- Real libspdm CHUNK_SEND responder and error response generator, compiled
  directly from the current sources. A valid control chunk is accepted before
  testing the oversized variant. Both SPDM 1.2 and 1.4 wire layouts are covered.
- Oversized plaintext is 264 bytes for a 256-byte receive limit; encoded DOE
  length is 68 DWORDs. The responder must reject in the first CHUNK_SEND_ACK
  with EARLY_ERROR_DETECTED / embedded InvalidRequest 0x01. Terminal 0x05,
  late 0x01, wrong sequence, and short errors cannot satisfy that predicate.
- Dynamic final-chunk selection for 2 through 9 chunks in both wire versions.
- Partial CERTIFICATE response gating, ignoring unrelated chunked responses.
- Recovery order and failure propagation at each step, using **mocked**
  connection/certificate/session/heartbeat/end-session operations on the same
  allocated context. The original fault result must remain unchanged.
- B5's real run function with a rejecting positive-control callback: assertion
  fails, no mutation fires, and no certificate-verification provenance is claimed.
- Callback and application-context restoration, and strict B6 INI expectations.

## Final requester/DOE boundary

The separate `doe_send_test` executable uses production `libspdm_send_request`,
the actual `chunk_encode` driver, and production `device_doe_send_message`,
including its second FI call. Only PCI register access and the PCAP sink are
replaced. It checks each DWORD written before GO and byte-for-byte equality
between those writes and the PCAP buffer. SPDM 1.2/1.4, limits 256/476, and both
unmodified and oversized first chunks are covered (eight combinations).
For limit 476 the mutation must produce 492 bytes / 123 DWORDs and increase
ChunkSize from 460 to 468 with the next eight bytes of the same large request.
LargeMessageSize and original request contents remain unchanged; there are no
trailing bytes. Historical final6 appended zero bytes without changing ChunkSize.
Eight further cases exercise the real requester chunk loop with a 1607-byte
request and scripted first ACKs; an ordinary first ACK stops further chunks.
The second FI
must not fire: the plaintext rule selects internal type 0xfd, not DOE type 1.
The real decoder and CHUNK_SEND handler then consume the mailbox sink bytes.
This does not emulate the physical mailbox or a device receive/dispatch path.

For hardware diagnosis, `B6 encode` is the pre-send encoder observation;
`B6 DOE tx` is after the final FI and the DWORD write loop; `B6 DOE rx` describes
the actual received ACK before the response decoder. These log metadata only
and remain enabled with `libspdm_log=0`. `writes_completed` counts successful
full PCI32 syscall returns, not independent PCI bus completions. Failed or short
PCI32 operations terminate the tool with offset/count/errno; fourteen syscall
mock cases cover read/write success, EINTR, failures and short transfers.
The PCAP similarly records the callback buffer, not an independent bus capture.

## Integration requirements / limits

- No new production translation unit or CMake change is needed. Fault.1's
  existing setup/run/teardown entry points remain in the main driver. Separate
  B4 and B5 mutual-auth drivers are untouched and can be routed independently.
- The parent campaign must honor failed **driver assertions**, including
  recovery assertions, even when FI fired and its `actual` string matched.
- The B5 positive-control callback remains a responder integration dependency.
  Generic error 0x05 from an always-rejecting callback is not proof of corruption
  detection. The driver now stops and records a failed provenance assertion.
- B6 abandonment reports `chunk_transfer_abandoned`; missing-final reports
  `final_chunk_withheld`. These mean deliberate local omission, **not** observed
  peer errors. Successful recovery is checked separately by driver assertions.
- Incomplete-transfer recovery uses GET_VERSION via `libspdm_init_connection`
  on the same allocated context, certificate refetch, a fresh authenticated
  session, HEARTBEAT and END_SESSION. No old-session HEARTBEAT is required.
- The reset assertion checks all **local** session slots before starting the new
  session; it does not replay old protected packets to probe peer key retention.
- Host tests do not establish actual hardware sessions or verify hardware
  recovery. Real hardware execution and the B5 positive-control success remain
  outstanding. No allowlist was widened to hide these limitations.
