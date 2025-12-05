# TEE-IO Device Validation Utility FAQ

This FAQ is for anyone interested in TEE-IO Device Validation Utility. The goal is for answers to be short & correct, but not necessarily complete. The [doc](../doc/) contains more in-depth articles.

If you find anything missing or incorrect in the FAQ, feel free to create an issue or PR.

## Questions List
- [What is TEE-IO Device Validation Utility?](#what-is-tee-io-device-validation-utility)

- [What should I do if some issue is found?](#what-should-i-do-if-some-issue-is-found)

- [What should I do if I want some new feature?](#what-should-i-do-if-i-want-some-new-feature)

- [How to setup build environment?](#how-to-setup-build-environment)

- [How to build binaries?](#how-to-build-binaries)

- [How to support DiceTcbInfo extension?](#how-to-support-dicetcbinfo-extension)

- [What is ide_test.ini in TEE-IO Device Validation Utility?](#what-is-ide_testini-in-tee-io-device-validation-utility)

- [Can I setup multiple Link/Selective IDE Streams with one ide_test.ini?](#can-i-setup-multiple-linkselective-ide-streams-with-one-ide_testini)

- [How I know if the IDE is properly set up](#how-i-know-if-the-ide-is-properly-set-up)

- [What I should do if I find test fail?](#what-i-should-do-if-i-find-test-fail)

- [How I know if the test is pass or fail?](#how-i-know-if-the-test-is-pass-or-fail)

- [Where to check the ouput log?](#where-to-check-the-ouput-log)

- [How to set debug level?](#how-to-set-debug-level)

- [How to enable spdm log?](#how-to-enable-spdm-log)

- [How to enable doe log?](#how-to-enable-doe-log)

- [How to do KeyRefresh stress test?](#how-to-do-keyrefresh-stress-test)

- [How to run traffic in IDE Stream?](#how-to-run-traffic-in-ide-stream)

- [How to setup a CXL-IDE stream between host and device?](#how-to-setup-a-cxl-ide-stream-between-host-and-device)

- [How to enable support for segment different than 0?](#how-to-enable-support-for-segment-different-than-0)

### What is TEE-IO Device Validation Utility?
Intel TDX Connect adds “device” to TEE scope. “Device” need to follow standard protocols (SPDM, IDE, TDISP) to communicate with Intel Root Port and Intel TDX TSM. 
TEE-IO Device Validation Utility is proposed to validate the interoperability of standard protocols (SPDM/IDE/TDISP) between the device and Intel component.

### What should I do if some issue is found?
1. Seach at [Issues](https://github.com/intel/tee-io-validator/issues) to see if same or similar issue is submitted.
2. If that is not the case, submit an issue and we will investigate it.

### What should I do if I want some new feature?
1. Seach at [Issues](https://github.com/intel/tee-io-validator/issues) to see if same or similar new feature is submitted.
2. If that is not the case, submit the feature request and we will investigate it. Please add [Feature Request] as the prefix in the title.

### How to setup build environment?
Refer to [teeio_validator_build](./teeio_validator_build.md/#setup-build-environment) for detailed steps.

### How to build binaries?
Refer to [teeio_validator_build](./teeio_validator_build.md/#build-binaries) for detailed steps.

### How to support DiceTcbInfo extension?
Refer to [teeio_validator_build](./teeio_validator_build.md/#DiceTcbInfo-extension-support) for detailed CMake command.

### How to run TEE-IO Device Validation Utility?
Refer to [teeio_validator_usage](./teeio_validator_usage.md) for detailed steps.

### What is ide_test.ini in TEE-IO Device Validation Utility?
TEE-IO Device Validation Utility reads ide_test.ini to get the information, such as root port dev/func, endpoint dev/func, topology, etc. Refer to [ide_test.ini](./ide_test_ini.md) for detailed information.

### Can I setup multiple Link/Selective IDE Streams with one ide_test.ini?
Refer to [Issue#21](https://github.com/intel/tee-io-validator/issues/21)

### How I know if the IDE is properly set up?
#### For Link IDE

According to PCIE Specification 7.9.26.4.2 Link IDE Stream Status Register
> Bit[3:0] Link IDE Stream State
>
> When Link IDE Stream Enable is Set, this field indicates the state of the Port.
>
> | Encodings | Description |
> | - | - |
> | 0000b  | Insecure |
> | 0010b  | Secure |
> | Others | Reserved – Software must handle reserved values as indicating unknown state |

When Link IDE Stream Enabled, and the Link IDE Stream State is `0010b` (Secure state), means Link IDE is properly set up.

#### For Selective IDE

It is similar with Link IDE accroding to 7.9.26.5.3 Selective IDE Stream Status Register

### What I should do if I find test fail?
- Check if the tool is running with root privileges.
- Check if the command options are correctly set. Refer to [teeio_validator_usage](./teeio_validator_usage.md) for detailed information.
- Check if the ide_test.ini is correctly set. Refer to [ide_test.ini](./ide_test_ini.md) for detailed information.
- Check if the devices are correctly connected. Use **lspci -t** to list the PCIe device tree.
- Check if the devices has enabled DiceTcbInfo. If it does, refer to [How to support DiceTcbInfo extension?](#how-to-support-dicetcbinfo-extension).
- Set "**debug_level=info**" in ide_test.ini and run **TEE-IO Device Validtion Utility** again. Then check the output log to see what happened.
- If all above cannot solve the problem, refer to [What should I do if some issue is found?](#what-should-i-do-if-some-issue-is-found). Remeber to provide the **ide_test.ini/command line/logs** to help us to investigate the issue.

### How I know if the test is pass or fail?
TEE-IO Device Validation Utility prints out the test result in 2 parts: details and summary. 
Below is an example which run the test of CXL-IDE.Query and CXL-IDE.GetKey.
```
Print detailed results.
TestSuite_1 (cxl-ide)
  Configuration_1 (default)
    TestGroup (Query) - setup pass
      TestCase Query.1: pass
          Assertion1.1.1: - pass sizeof(CxlIdeKmMessage) = 0x28
          Assertion1.1.2: - pass CxlIdeKmMessage.ObjectID = 0x1
          Assertion1.1.3: - pass CxlIdeKmMessage.PortIndex = 0x0
          Assertion1.1.4: - pass CxlIdeKmMessage.MaxPortIndex = 0x0
          Assertion1.1.5: - pass CxlIdeKmMessage.DevFunc = 0x0 && CxlIdeKmMessage.Bus = 0x20 && CxlIdeKmMessage.Segment = 0x0
          Assertion1.1.6: - pass CxlIdeKmMessage.CXL_IDE_Capability_Version = 0x1
      TestCase Query.2: skipped
    TestGroup (Query) - teardown pass

    TestGroup (GetKey) - setup pass
      TestCase GetKey.1: pass
        port_index = 0x00
          Assertion5.1.1: - pass sizeof(CxlIdeKmMessage) = 0x33
          Assertion5.1.2: - pass CxlIdeKmMessage.ObjectID = 0x8
          Assertion5.1.3: - pass CxlIdeKmMessage.PortIndex = 0x0
          Assertion5.1.4: - pass CxlIdeKmMessage.StreamID = 0x0
          Assertion5.1.5: - pass CxlIdeKmMessage.key_sub_stream = 0x80
    TestGroup (GetKey) - teardown pass

Print summary results.
TestSuite_1 (cxl-ide) - pass: 11, fail: 0
  Configuration_1 (default)
    TestGroup (Query) - pass: 6, fail: 0
      TestCase Query.1: pass (pass: 6, fail: 0)
      TestCase Query.2: skipped (pass: 0, fail: 0)

    TestGroup (GetKey) - pass: 5, fail: 0
      TestCase GetKey.1: pass (pass: 5, fail: 0)
```

### Where to check the ouput log?
TEE-IO Device Validation Utility prints information and debug log in both console and **teeio_log_\<timestamp\>.txt** (e.g. teeio_log_2024-04-17_21-23-38.txt). Refer to [Issue#30](https://github.com/intel/tee-io-validator/issues/30#issuecomment-2046467274)

### How to set debug level?
TEE-IO Device Validation Utility set debug_level in 2 ways: ide_test.ini and command option. Refer to [Issue#30](https://github.com/intel/tee-io-validator/issues/30#issuecomment-2046469418)

### How to enable spdm log?
TEE-IO Device Validation Utility enables spdm log output by setting ide_test.ini. Refer to [Issue#57](https://github.com/intel/tee-io-validator/issues/57#issuecomment-2116503580)

### How to enable doe log?
TEE-IO Device Validation Utility enables doe log output by setting ide_test.ini. Refer to [Issue#94](https://github.com/intel/tee-io-validator/issues/94#issuecomment-2192973993)

### How to do KeyRefresh stress test?
TEE-IO Device Validation Utility provides the stress test feature for KeyRefresh. Refer to [Issue#254](https://github.com/intel/tee-io-validator/issues/254#issuecomment-2722989688)

### How to run traffic in IDE Stream?
Refer to [run_traffic](./run_traffic.md)

### How to setup a CXL-IDE stream between host and device?
Refer to [Run CXL-IDE Stream cases](./teeio_validator_usage.md#run-cxl-ide-stream-cases)

### How to enable support for segment different than 0?
Refer to `[Topology_x]` field description in [ide_test_ini](./ide_test_ini.md#pcie-ide-section-settings), for example:

Add `segment` field in `[Topology_x]`
```
[Topology_1]
type            = selective_ide
connection      = direct
bus             = 0x1a
segment         = 0x1             ; use segment 0x1
path1           = rootport_1,endpoint_1
stream_id       = 1
```