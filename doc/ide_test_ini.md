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

[Topology_x]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|type|string||M|available values are: **selective_ide, link_ide, selective_and_link_ide**|
|connection|string||M|available values are: **direct, switch, peer2peer**|
|bus|hex||M|The bus which rootport is connected to. For example 0x1a|
|path1|string||M|rootport_x to endpoint_y. Each ports are separated by ‘,’. For example: rootport_1,switch_1:port_1-port_2,endpoint_2|
|path2|string||O|rootport_x to endpoint_y. Each ports are separated by ‘,’. For example: rootport_1,switch_1:port_1-port_3,endpoint_3. <br/>**Note: path2 is only available in the connection of peer2peer**|
|stream_id|number|0|O|it shall be in [0, 255]|

[Configration_x]
|Entry|Value|Default|Mandatory|Comment|
|------|------|------|------|------|
|type|string||M|available values are **selective_ide, link_ide, selective_and_link_ide**|
|default|0/1|0|O|if default is set, then below fields are ignored, otherwise below fields are needed.|
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
|category|string|pcie|O|Test category. Available values are: **pcie, cxl.mem**|
|topology|number||M|Topology_x|
|configuration|number||M|Configuration_x|
|query|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [Query.md](../doc/ide_test/IdeKmTestCase/1.Query.md)|
|KeyProg|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [keyProg.md](../doc/ide_test/IdeKmTestCase/2.keyProg.md)|
|KSetGo|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [KSetGo.md](../doc/ide_test/IdeKmTestCase/3.KSetGo.md)|
|KSetStop|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [KSetStop.md](../doc/ide_test/IdeKmTestCase/4.KSetStop.md)|
|SpdmSession|string||O|numbers separated by comma.<br> For example **1,2** means Cases1 and Cases2 in [SpdmSession.md](../doc/ide_test/IdeKmTestCase/5.SpdmSession.md)|
