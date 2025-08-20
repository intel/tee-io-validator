# ide_test.ini spec

**TEE-IO Device Validation Utility** reads ide_test.ini to get the information, such as root port dev/func, endpoint dev/func, topology, etc. Its structure refers to [IdeKmTestCase](../doc/ide_test/IdeKmTestCase), [IdeKmTestConfiguration](../doc/ide_test/IdeKmTestConfiguration) and [IdeKmTestTopology](../doc/ide_test/IdeKmTestTopology)

There are 6 sections in the ide_test.ini.
1. Main
2. Ports
3. Switch_x (x is in [1,16])
4. Topology_x (x is in [1,16])
5. Configuration_x (x is in [1,32])
6. TestSuite_x (x is in [1,32])

## Format
```
[SectionName]
EntryName=EntryValue

    Where:
      1) SectionName is an ASCII string. The valid format is [A-Za-z0-9_]+
      2) EntryName is an ASCII string. The valid format is [A-Za-z0-9_]+
      3) EntryValue can be:
         3.1) an ASCII String. The valid format is [A-Za-z0-9_.,:]+
         3.2) a GUID. The valid format is xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx, where x is [A-Fa-f0-9]
         3.3) a decimal value. The valid format is [0-9]+
         3.4) a hexadecimal value. The valid format is 0x[A-Fa-f0-9]+
      4) '#' or ';' can be used as comment at anywhere.
      5) TAB(0x20) or SPACE(0x9) can be used as separator.
      6) LF(\n, 0xA) or CR(\r, 0xD) can be used as line break.
```

## Descriptions of sections
[Main]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
| pci_log|0/1 |0 | O | enable pci log if 1|
| libspdm_log|0/1 |0 | O | enable libspdm log if 1|
| debug_level | verbose/info/warn/error | warn | O | debug level|
| pcap_enable | 0/1 | 0 | O | enable pcap capture if 1|
| doe_log|0/1 |0 | O | enable doe log if 1|

[Ports]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|rootport_x|dev/func string| |M|For example 02.0. **x** in [1, 16]|
|endpoint_y|dev/func string||M|For example 00.0. **y** in [1, 16]|

[Switch_x]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|port_y|dev/func string||M|For example 00.0. **y** in [1, 16]|

**TEE-IO Device Validation Utility** supports multiple test categories.
- [PCIE-IDE Test](../doc/ide_test/)
- [TDISP Test](../doc/tdisp_test/)
- [CXL-IDE Test](../doc/cxl_ide_test/)
- [CXL-TSP Test](../doc/tsp_test/)
- [SPDM Test](../doc/spdm_test/)

Settings of **Topology** / **Configuration** / **TestSuite** are different.

- [PCIE-IDE Section settings](#pcie-ide-section-settings)
- [TDISP Section settings](#tdisp-section-settings)
- [CXL-IDE Section settings](#cxl-ide-section-settings)
- [CXL-TSP Section settings](#cxl-tsp-section-settings)
- [SPDM Section settings](#spdm-section-settings)

### PCIE-IDE Section settings
[Topology_x]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|type|string||M|available values are: **selective_ide, link_ide, selective_and_link_ide**|
|connection|string||M|available values are: **direct, switch**|
|segment|hex||M|The segment which rootport is connected to. For example 0x0000|
|bus|hex||M|The bus which rootport is connected to. For example 0x1a|
|path1|string||M|rootport_x to endpoint_y. Each ports are separated by ‘,’. For example: rootport_1,switch_1:port_1-port_2,endpoint_2|
|path2|string||O|rootport_x to endpoint_y. Each ports are separated by ‘,’. For example: rootport_1,switch_1:port_1-port_3,endpoint_3. <br/>**Note: path2 is only available in the connection of peer2peer**|
|stream_id|number|0|O|it shall be in [0, 255]|

[Configration_x]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|type|string||M|available values are **selective_ide, link_ide, selective_and_link_ide**|
|category|string|pcie-ide|O|must be **pcie-ide**|
|default|0/1|1|O||
|switch|0/1|0|O||
|partial_header_encryption|0/1|0|O||
|pcrc|0/1|0|O||
|aggregation|0/1|0|O||
|selective_ide_for_configuration|0/1|0|O||
|tee_limited_stream|0/1|0|O||

[TestSuite_x]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|type|string||M|available values are: **selective_ide, link_ide, selective_and_link_ide**|
|category|string|pcie-ide|O|must be **pcie-ide**|
|topology|number||M|Topology_x|
|configuration|number||M|Configuration_x|
|query|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [Query.md](../doc/ide_test/IdeKmTestCase/1.Query.md)|
|KeyProg|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [keyProg.md](../doc/ide_test/IdeKmTestCase/2.keyProg.md)|
|KSetGo|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [KSetGo.md](../doc/ide_test/IdeKmTestCase/3.KSetGo.md)|
|KSetStop|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [KSetStop.md](../doc/ide_test/IdeKmTestCase/4.KSetStop.md)|
|SpdmSession|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [SpdmSession.md](../doc/ide_test/IdeKmTestCase/5.SpdmSession.md)|

### CXL-IDE Section settings
[Topology_x]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|type|string||M|must be **link_ide**|
|connection|string||M|available values are: **direct, switch**|
|segment|hex||M|The segment which rootport is connected to. For example 0x0000|
|bus|hex||M|The bus which rootport is connected to. For example 0x1a|
|path1|string||M|rootport_x to endpoint_y. Each ports are separated by ‘,’. For example: rootport_1,switch_1:port_1-port_2,endpoint_2|
|path2|string||O|rootport_x to endpoint_y. Each ports are separated by ‘,’. For example: rootport_1,switch_1:port_1-port_3,endpoint_3. <br/>**Note: path2 is only available in the connection of peer2peer**|
|stream_id|number|0|O|it shall always be **0**|

[Configration_x]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|type|string||M|must be **link_ide**|
|category|string||M|must be **cxl-ide**|
|cxl_ide_mode|string|containment|O|available values are **containment** or **skid**|
|default|0/1|1|O||
|pcrc_disable|0/1|0|O||
|ide_stop|0/1|0|O||
|cxl_get_key|0/1|0|O|Generate keys by IDEKM GET_KEY when setting up CXL.memcache IDE Stream.|

[TestSuite_x]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|type|string||M|must be **link_ide**|
|category|string||M|must be **cxl-ide**|
|topology|number||M|Topology_x|
|configuration|number||M|Configuration_x|
|query|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [CxlQuery.md](../doc/cxl_ide_test/CxlIdeKmTestCase/1.CxlQuery.md)|
|KeyProg|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [CxlKeyProg.md](../doc/cxl_ide_test/CxlIdeKmTestCase/2.CxlKeyProg.md)|
|KSetGo|string||O|numbers separated by comma.<br> For example **1** means Cases1 in [CxlKSetGo.md](../doc/cxl_ide_test/CxlIdeKmTestCase/3.CxlKSetGo.md)|
|KSetStop|string||O|numbers separated by comma.<br> For example **1** means Cases1 in [CxlKSetStop.md](../doc/cxl_ide_test/CxlIdeKmTestCase/4.CxlKSetStop.md)|
|GetKey|string||O|numbers separated by comma.<br> For example **1** means Cases1 in [CxlGetKey.md](../doc/cxl_ide_test/CxlIdeKmTestCase/5.CxlGetKey.md)|

### CXL-TSP Section settings
[Topology_x]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|type|string||M|must be **link_ide**|
|connection|string||M|available values are: **direct, switch**|
|segment|hex||M|The segment which rootport is connected to. For example 0x0000|
|bus|hex||M|The bus which rootport is connected to. For example 0x1a|
|path1|string||M|rootport_x to endpoint_y. Each ports are separated by ‘,’. For example: rootport_1,switch_1:port_1-port_2,endpoint_2|
|path2|string||O|rootport_x to endpoint_y. Each ports are separated by ‘,’. For example: rootport_1,switch_1:port_1-port_3,endpoint_3. <br/>**Note: path2 is only available in the connection of peer2peer**|
|stream_id|number|0|O|it shall always be **0**|

[Configration_x]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|type|string||M|must be **link_ide**|
|category|string||M|must be **cxl-tsp**|
|default|0/1|1|O||

[TestSuite_x]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|type|string||M|must be **link_ide**|
|category|string||M|must be **cxl-tsp**|
|topology|number||M|Topology_x|
|configuration|number||M|Configuration_x|
|GetVersion|string||O|numbers separated by comma.<br> For example **1** means Case1 in [GetTargetTspVersionResponse.md](../doc/tsp_test/TspTestCase/1.GetTargetTspVersionResponse.md)|
|GetCaps|string||O|numbers separated by comma.<br> For example **1** means Case1 in [GetTargetTspCapabilitiesResponse.md](../doc/tsp_test/TspTestCase/2.GetTargetTspCapabilitiesResponse.md)|
|SetConfiguration|string||O|numbers separated by comma.<br> For example **1** means Case1 in [SetTargetTspConfigurationResponse.md](../doc/tsp_test/TspTestCase/3.SetTargetTspConfigurationResponse.md)|
|GetConfiguration|string||O|numbers separated by comma.<br> For example **1** means Case1 in [GetTargetTspConfigurationResponse.md](../doc/tsp_test/TspTestCase/4.GetTargetTspConfigurationResponse.md)|
|GetConfigurationReport|string||O|numbers separated by comma.<br> For example **1** means Case1 in [GetTargetTspConfigurationReportResponse.md](../doc/tsp_test/TspTestCase/5.GetTargetTspConfigurationReportResponse.md)|
|LockConfiguration|string||O|numbers separated by comma.<br> For example **1,2** means Case1 and Case2 in [LockTargetTspConfigurationResponse.md](../doc/tsp_test/TspTestCase/6.LockTargetTspConfigurationResponse.md)|

### SPDM Section settings
[Topology_x]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|type|string||M|must be **link_ide**|
|connection|string||M|available values are: **direct, switch**|
|segment|hex||M|The segment which rootport is connected to. For example 0x0000|
|bus|hex||M|The bus which rootport is connected to. For example 0x1a|
|path1|string||M|rootport_x to endpoint_y. Each ports are separated by ‘,’. For example: rootport_1,switch_1:port_1-port_2,endpoint_2|
|path2|string||O|rootport_x to endpoint_y. Each ports are separated by ‘,’. For example: rootport_1,switch_1:port_1-port_3,endpoint_3. <br/>**Note: path2 is only available in the connection of peer2peer**|

[Configration_x]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|category|string|spdm|M|must be **spdm**|

[TestSuite_x]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|category|string|spdm|M|must be **spdm**|
|topology|number||M|Topology_x|
|configuration|number||M|Configuration_x|
|Version|string||O|numbers separated by comma.<br> For example **1** means Cases1 in [Version.md](https://github.com/DMTF/SPDM-Responder-Validator/blob/main/doc/1.Version.md)|
|Capabilities|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [Capabilities.md](https://github.com/DMTF/SPDM-Responder-Validator/blob/main/doc/2.Capabilities.md)|
|Algorithms|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [Algorithms.md](https://github.com/DMTF/SPDM-Responder-Validator/blob/main/doc/3.Algorithms.md)|
|Certificate|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [Certificate.md](https://github.com/DMTF/SPDM-Responder-Validator/blob/main/doc/5.Certificate.md)|
|Measurements|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [Measurements.md](https://github.com/DMTF/SPDM-Responder-Validator/blob/main/doc/7.Measurements.md)|
|KeyExchangeRsp|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [KeyExchangeRsp.md](https://github.com/DMTF/SPDM-Responder-Validator/blob/main/doc/8.KeyExchangeRsp.md)|
|FinishRsp|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [FinishRsp.md](https://github.com/DMTF/SPDM-Responder-Validator/blob/main/doc/9.FinishRsp.md)|
|EndSessionAck|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [EndSessionAck.md](https://github.com/DMTF/SPDM-Responder-Validator/blob/main/doc/16.EndSessionAck.md)|

### TDISP Section settings
[Topology_x]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|type|string||M|available values are: **selective_ide, link_ide, selective_and_link_ide**|
|connection|string||M|available values are: **direct, switch**|
|segment|hex||M|The segment which rootport is connected to. For example 0x0000|
|bus|hex||M|The bus which rootport is connected to. For example 0x1a|
|tdisp_function_id|hex|0x0|O|FUNCTION_ID of the device hosting the TDI. Refer to PCIE/TDISP spec for more detailed information|
|path1|string||M|rootport_x to endpoint_y. Each ports are separated by ‘,’. For example: rootport_1,switch_1:port_1-port_2,endpoint_2|
|path2|string||O|rootport_x to endpoint_y. Each ports are separated by ‘,’. For example: rootport_1,switch_1:port_1-port_3,endpoint_3. <br/>**Note: path2 is only available in the connection of peer2peer**|
|stream_id|number|0|O|it shall be in [0, 255]|

[Configration_x]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|type|string||M|available values are **selective_ide, link_ide, selective_and_link_ide**|
|category|string|tdisp|M|must be **tdisp**|

[TestSuite_x]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|type|string||M|available values are: **selective_ide, link_ide, selective_and_link_ide**|
|category|string|tdisp|M|must be **tdisp**|
|topology|number||M|Topology_x|
|configuration|number||M|Configuration_x|
|Version|string||O|numbers separated by comma.<br> For example **1** means Cases1 in [TdispVersion.md](../doc/tdisp_test/TdispTestCase/1.TdispVersion.md)|
|Capabilities|string||O|numbers separated by comma.<br> For example **1** means Cases1 in [TdispCapabilities.md](../doc/tdisp_test/TdispTestCase/2.TdispCapabilities.md)|
|LockInterface|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [LockInterfaceResponse.md](../doc/tdisp_test/TdispTestCase/3.LockInterfaceResponse.md)|
|DeviceReport|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [DeviceInterfaceReport.md](../doc/tdisp_test/TdispTestCase/4.DeviceInterfaceReport.md)|
|DeviceState|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [DeviceInterfaceState.md](../doc/tdisp_test/TdispTestCase/5.DeviceInterfaceState.md)|
|StartInterface|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [StartInterfaceResponse.md](../doc/tdisp_test/TdispTestCase/6.StartInterfaceResponse.md)|
|StopInterface|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [StopInterfaceResponse.md](../doc/tdisp_test/TdispTestCase/7.StopInterfaceResponse.md)|
