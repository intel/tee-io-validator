# TEE-IO Device Validation Utility Usage

## Usage
```
teeio_validator version 0.1.0

Usage:
  teeio_validator -f ide_test.ini
  teeio_validator -f ide_test.ini -t 1 -c 1 -s Test.IdeStream

Options:
  -f <ide_test.ini>   : The file name of test configuration. For example ide_test.ini
  -t <top_id>         : topology id which is to be tested. For example 1
  -c <config_id>      : configuration id which is to be tested. For example 1
  -s <test_case>      : Test case to be tested. For example Test.IdeStream
  -l <debug_level>    : Set debug level. error/warn/info/verbose
  -b <scan_bus>       : Bus number in hex format. For example 0x1a
  -h                  : Display this usage
```

**Note**:
1. There 2 usage modes - **Pre-defined TestSuite mode** and **Tester designate mode**.
- Pre-defined TestSuite mode: teeio_validator automatically scan ide_test.ini and run all TestSuite_x sections.
- Tester designate mode: teeio_validator designate test topology/configuration/case from command line.
2. Refer to [ide_test_ini.md](../doc/ide_test_ini.md) for detailed description of ide_test.ini. Here is a sample [ide_test.ini](./ide_test.ini).
