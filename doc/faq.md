# TEE-IO Device Validation Utility FAQ

This FAQ is for anyone interested in TEE-IO Device Validation Utility. The goal is for answers to be short & correct, but not necessarily complete. The [doc](../doc/) contains more in-depth articles.

If you find anything missing or incorrect in the FAQ, feel free to create an issue or PR.

## Questions List
- [What is TEE-IO Device Validation Utility?](#what-is-tee-io-device-validation-utility)

- [What should I do if some issue is found?](#what-should-i-do-if-some-issue-is-found)

- [What should I do if I want some new feature?](#what-should-i-do-if-i-want-some-new-feature)

- [How to setup build environment?](#how-to-setup-build-environment)

- [How to build binaries?](#how-to-build-binaries)

- [What is ide_test.ini in TEE-IO Device Validation Utility?](#what-is-ide_testini-in-tee-io-device-validation-utility)

- [Can I setup multiple Link/Selective IDE Streams with one ide_test.ini?](#can-i-setup-multiple-linkselective-ide-streams-with-one-ide_testini)

- [What I should do if I find test fail?](#what-i-should-do-if-i-find-test-fail)

- [How I know if the test is pass or fail?](#how-i-know-if-the-test-is-pass-or-fail)

- [Where to check the ouput log?](#where-to-check-the-ouput-log)

- [How to set debug level?](#how-to-set-debug-level)

- [How to enable spdm log?](#how-to-enable-spdm-log)

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

### How to run TEE-IO Device Validation Utility?
Refer to [teeio_validator_usage](./teeio_validator_usage.md) for detailed steps.

### What is ide_test.ini in TEE-IO Device Validation Utility?
TEE-IO Device Validation Utility reads ide_test.ini to get the information, such as root port dev/func, endpoint dev/func, topology, etc. Refer to [ide_test.ini](./ide_test_ini.md) for detailed information.

### Can I setup multiple Link/Selective IDE Streams with one ide_test.ini?
Refer to [Issue#21](https://github.com/intel/tee-io-validator/issues/21)

### What I should do if I find test fail?
- Check if the tool is running with root privileges.
- Check if the command options are correctly set. Refer to [teeio_validator_usage](./teeio_validator_usage.md) for detailed information.
- Check if the ide_test.ini is correctly set. Refer to [ide_test.ini](./ide_test_ini.md) for detailed information.
- Check if the devices are correctly connected. Use **lspci -t** to list the PCIe device tree.
- Set "**debug_level=info**" in ide_test.ini and run **TEE-IO Device Validtion Utility** again. Then check the output log to see what happened.
- If all above cannot solve the problem, refer to [What should I do if some issue is found?](#what-should-i-do-if-some-issue-is-found). Remeber to provide the **ide_test.ini/command line/logs** to help us to investigate the issue.

### How I know if the test is pass or fail?
TEE-IO Device Validation Utility will print out the test result as below:
```
 Print test results.
 TestSuite_1 - pass: 14, fail: 0, skip: 0
     TestGroup (SelectiveIDE Default Query) - pass: 2, fail: 0, skip: 0
       Query.1: case - pass; ide_stream_secure - na
       Query.2: case - pass; ide_stream_secure - na
     TestGroup (SelectiveIDE Default KeyProg) - pass: 6, fail: 0, skip: 0
       KeyProg.1: case - pass; ide_stream_secure - na
       KeyProg.2: case - pass; ide_stream_secure - na
       KeyProg.3: case - pass; ide_stream_secure - na
       KeyProg.4: case - pass; ide_stream_secure - na
       KeyProg.5: case - pass; ide_stream_secure - na
       KeyProg.6: case - pass; ide_stream_secure - na
     TestGroup (SelectiveIDE Default KSetGo) - pass: 2, fail: 0, skip: 0
       KSetGo.1: case - pass; ide_stream_secure - pass
       KSetGo.2: case - pass; ide_stream_secure - pass
     TestGroup (SelectiveIDE Default KSetStop) - pass: 4, fail: 0, skip: 0
       KSetStop.1: case - pass; ide_stream_secure - na
       KSetStop.2: case - pass; ide_stream_secure - na
       KSetStop.3: case - pass; ide_stream_secure - na
       KSetStop.4: case - pass; ide_stream_secure - na
```

### Where to check the ouput log?
TEE-IO Device Validation Utility prints information and debug log in both console and **teeio_log_\<timestamp\>.txt** (e.g. teeio_log_2024-04-17_21-23-38.txt). Refer to [Issue#30](https://github.com/intel/tee-io-validator/issues/30#issuecomment-2046467274)

### How to set debug level?
TEE-IO Device Validation Utility set debug_level in 2 ways: ide_test.ini and command option. Refer to [Issue#30](https://github.com/intel/tee-io-validator/issues/30#issuecomment-2046469418)

### How to enable spdm log?
TEE-IO Device Validation Utility enables spdm log output by setting ide_test.ini. Refer to [Issue#57](https://github.com/intel/tee-io-validator/issues/57#issuecomment-2116503580)
