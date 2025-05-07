# TEE-IO Device Validation Utility Usage

## Usage
```
teeio_validator version 0.3.0

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
  -e <test_interval>  : test interval of 2 rounds.
  -n <test_rounds>    : test rounds of a case.
  -k                  : Use fixed IDE Key for debug purpose.
  -h                  : Display this usage
```

**Note**:
There 2 usage modes - **Pre-defined TestSuite mode** and **Tester designate mode**.
- **Pre-defined TestSuite mode**: teeio_validator automatically scan ide_test.ini and run all TestSuite_x sections.
- **Tester designate mode**: teeio_validator designate test topology/configuration/case from command line.

## Check how TEEIO Device is connected
Before running teeio-validator, use below command to check how the device is connected to host (For example device's BDF is da:00.0).
```
# lspci -PP -s da:00.0
d9:02.0/da:00.0 Non-Volatile memory controller: XXXX Vendor's Device
```
From above output, the host BDF is ```d9:02.0```, the device BDF is ```da:00.0```. These information will be used when preparing the test .ini file.

## Run PCIE-IDE Stream cases
**Step1** Prepare the pcie_ide.ini.

Refer to [ide_test_ini.md](../doc/ide_test_ini.md) for detailed description of .ini file. Here is a sample [pcie_ide.ini](./sample_ini/pcie_ide.ini).

**Note**: The ```bus``` in ```[Topology_1]``` shall be replaced by the bus of **host BDF**(```0xd9```). The ```rootport_1``` and ```endpoint_1``` in ```[Ports]``` shall be replaced by the dev/func of **host BDF** and **device  BDF** respectively. Refer to [Check how TEEIO Device is connected](#check-how-teeio-device-is-connected)

**Step2** Run test case

```
# Setup PCIE-IDE Stream
./teeio_validator -f pcie_ide.ini -c 1 -t 1 -s Test.IdeStream

# Run PCIE-IDE KeyRefresh
./teeio_validator -f pcie_ide.ini -c 1 -t 1 -s Test.KeyRefresh

# Run cases in [TestSuite_1] in pcie_ide.ini.
./teeio_validator -f pcie_ide.ini
```

## Run CXL-IDE Stream cases
**Step1** Prepare the cxl_ide.ini.

Refer to [ide_test_ini.md](../doc/ide_test_ini.md) for detailed description of .ini file. Here is a sample [cxl_ide.ini](./sample_ini/cxl_ide.ini).

**Note**: The ```bus``` in ```[Topology_1]``` shall be replaced by the bus of **host BDF**(```0xd9```). The ```rootport_1``` and ```endpoint_1``` in ```[Ports]``` shall be replaced by the dev/func of **host BDF** and **device  BDF** respectively. Refer to [Check how TEEIO Device is connected](#check-how-teeio-device-is-connected)

**Step2** Run test case

```
# Setup CXL-IDE Stream
./teeio_validator -f cxl_ide.ini -c 1 -t 1 -s Test.IdeStream

# Run CXL-IDE KeyRefresh
./teeio_validator -f cxl_ide.ini -c 1 -t 1 -s Test.KeyRefresh

# Run cases in [TestSuite_1] in cxl_ide.ini.
./teeio_validator -f cxl_ide.ini
```

## Run SPDM test cases
**Step1** Prepare the spdm_test.ini.

Refer to [ide_test_ini.md](../doc/ide_test_ini.md) for detailed description of .ini file. Here is a sample [spdm_test.ini](./sample_ini/spdm_test.ini).

**Note**: The ```bus``` in ```[Topology_1]``` shall be replaced by the bus of **host BDF**(```0xd9```). The ```rootport_1``` and ```endpoint_1``` in ```[Ports]``` shall be replaced by the dev/func of **host BDF** and **device  BDF** respectively. Refer to [Check how TEEIO Device is connected](#check-how-teeio-device-is-connected)

**Step2** Run test case

```
# Run cases in [TestSuite_1] in spdm_test.ini.
./teeio_validator -f spdm_test.ini
```