## Test case for SPDM

PCI-SIG defined TEE-IO device requires following configuration:

1. SPDM version shall be `1.2` or above.
2. SPDM capability shall include `CERT_CAP`, `MEAS_CAP=10b`, `ENCRYPT_CAP`, `MAC_CAP`, `KEY_EX_CAP`.
3. SPDM algorithm shall support `MeasurementSpecification=DMTF`, `OtherParams=OpaqueDataFmt1`, `MeasurementHashAlgo=TPM_ALG_SHA_256|TPM_ALG_SHA_384`, `BaseAsymAlgo=TPM_ALG_RSASSA_3072|TPM_ALG_ECDSA_ECC_NIST_P256|TPM_ALG_ECDSA_ECC_NIST_P384` , `BaseHashAlgo=TPM_ALG_SHA_256|TPM_ALG_SHA_384`, `DHE=secp256r1|secp384r1`, `AEADCipherSuite=AES-256-GCM`, `KeySchedule=SPDM Key Schedule`.
4. SPDM response code shall support `VERSION`, `CAPABILITIES`, `ALGORITHMS`, `CERTIFICATE`, `MEASUREMENTS`, `KEY_EXCHANGE_RSP`, `FINISH_RSP`, `END_SESSION_ACK`.

The test case for SPDM is at https://github.com/DMTF/SPDM-Responder-Validator.
