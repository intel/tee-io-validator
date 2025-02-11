# TEE-IO Device Validation Utility

Intel TDX Connect adds “device” to TEE scope. “Device” need to follow standard protocols (SPDM, IDE, TDISP) to communicate with Intel Root Port and Intel TDX TSM. We need validate the interoperability between the device and Intel component.

**TEE-IO Device Validation Utility** is proposed to validate the interoperability of standard protocols (SPDM/IDE/TDISP) between the device and Intel component.

## Documentation

[TEE-IO Device Validation Utility Build](./doc/teeio_validator_build.md)

[TEE-IO Device Validation Utility Usage](./doc/teeio_validator_usage.md)

[ide_test.ini spec](./doc/ide_test_ini.md)

[FAQ](./doc/faq.md)

### IDE_KM Test

#### Test Case

1. [Test for QUERY](./doc/ide_test/IdeKmTestCase/1.Query.md)

2. [Test for KEY_PROG](./doc/ide_test/IdeKmTestCase/2.KeyProg.md)

3. [Test for K_SET_GO](./doc/ide_test/IdeKmTestCase/3.KSetGo.md)

4. [Test for K_SET_STOP](./doc/ide_test/IdeKmTestCase/4.KSetStop.md)

5. [Test for SPDM Session](./doc/ide_test/IdeKmTestCase/5.SpdmSession.md)

#### Test Configuration

1. [Test for Selective IDE](./doc/ide_test/IdeKmTestConfiguration/1.SelectiveIDE.md)

2. [Test for Link IDE](./doc/ide_test/IdeKmTestConfiguration/2.LinkIDE.md)

3. [Test for both Selective IDE and Link IDE enabling](./doc/ide_test/IdeKmTestConfiguration/3.SelectiveAndLinkIDE.md)

#### Test Topology

1. [Test for Selective IDE](./doc/ide_test/IdeKmTestTopology/1.SelectiveIDE.md)

2. [Test for Link IDE](./doc/ide_test/IdeKmTestTopology/2.LinkIDE.md)

3. [Test for both Selective IDE and Link IDE enabling](./doc/ide_test/IdeKmTestTopology/3.SelectiveAndLinkIDE.md)

### Standard

#### DMTF

DMTF [DSP0274](https://www.dmtf.org/dsp/DSP0274) Security Protocol and Data Model (SPDM) Specification (version [1.2.2](https://www.dmtf.org/sites/default/files/standards/documents/DSP0274_1.2.2.pdf))

DMTF [DSP0277](https://www.dmtf.org/dsp/DSP0277) Secured Messages using SPDM Specification (version [1.1.1](https://www.dmtf.org/sites/default/files/standards/documents/DSP0277_1.1.1.pdf))

#### PCI-SIG

PCIe Base Specification Version [6.0.1](https://members.pcisig.com/wg/PCI-SIG/document/18363), [6.1](https://members.pcisig.com/wg/PCI-SIG/document/19849), [6.2](https://members.pcisig.com/wg/PCI-SIG/document/20590), [6.3](https://members.pcisig.com/wg/PCI-SIG/document/21847).

PCIe [DOE 1.0 ECN](https://members.pcisig.com/wg/PCI-SIG/document/14143) for PCIe 4.0, 5.0 (integrated in 6.0), [DOE 1.1 ECN](https://members.pcisig.com/wg/PCI-SIG/document/18483) for PCIe 5.0, 6.0 (integrated in 6.1).

PCIe [CMA ECN](https://members.pcisig.com/wg/PCI-SIG/document/14236) for PCIe 4.0, 5.0 (integrated in 6.0), [CMA Revised ECN](https://members.pcisig.com/wg/PCI-SIG/document/20110) for PCIe 6.1 (integrated in 6.2).

PCIe [IDE ECN](https://members.pcisig.com/wg/PCI-SIG/document/16599) for PCIe 5.0 (integrated in 6.0).

PCIe [TDISP ECN](https://members.pcisig.com/wg/PCI-SIG/document/18268) for PCIe 5.0, 6.0 (integrated in 6.1).

#### CXL

Compute Express Link Specification Revision [3.0](https://computeexpresslink.org/past-cxl-specifications/), [3.1](https://computeexpresslink.org/past-cxl-specifications/), [3.2](https://computeexpresslink.org/cxl-specification)

#### Intel

Intel [Root Complex IDE Key Configuration Unit - Software Programming Guide](https://cdrdv2.intel.com/v1/dl/getContent/732838)
