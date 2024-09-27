# IDE Key Byte Order

This document clarifies how to program AES-GCM Key and IV to IDE_KM message and Root Complex.

It describes the mapping of AES-GCM Key and IV for the following:
 * AES-GCM Key and IV (big endian) - Usually it is input to crypt API, such as openssl or mbedtls. It is also used in PCIE Specification Appendix L.
 * IDE_KM - It is defined in PCIE Specification and CXL Specification.
 * KEY/IV register - It is defined in Intel Root Complex IDE Key Configuration Unit - Software Programming Guide.

Reference:

 * PCIe Base Specification Version [6.2](https://members.pcisig.com/wg/PCI-SIG/document/20590).
 * Compute Express Link Specification Revision [3.1](https://computeexpresslink.org/cxl-specification).
 * Intel Root Complex IDE Key Configuration Unit – Software Programming Guide [1.02](https://www.intel.com/content/www/us/en/io/pci-express/pci-express-architecture-devnet-resources.html)

## AES GCM key/IV

Let's assume the AES GCM key/IV is following:

| Input Variables for AES GCM | Value byte[0], byte[1], ... byte[N] </br> Key, IV, MAC : Follows AES-GCM Specification with Most significant leftmost </br> (Align with PCIE specification appendix L)|
|---------|------------|
| Key (K) | df 25 41 52 05 6e 02 e0 ef 8b 7f eb 97 39 d4 d9 6a 4e b8 01 03 24 1d f7 cd 5e 24 b4 9c cd 27 20 |
| Initialization Vector (IV) - PCIE | (00 00 00 00) 00 00 00 00 00 00 00 01 |
| Initialization Vector (IV) - CXL  | (80 00 00 00) 00 00 00 00 00 00 00 01 |

## CXL IDE_KM KEY_PROG message

The CXL IDE_KM KEY_PROG message is following:

| Bits[31:24]               | Bits[23:16]               | Bits[15:8]                | Bits[7:0]                 |
|---------------------------|---------------------------|---------------------------|---------------------------|
|Key_DW7_Byte3: byte[0]:  df|Key_DW7_Byte2: byte[1]:  25|Key_DW7_Byte1: byte[2]:  41|Key_DW7_Byte0: byte[3]:  52|
|Key_DW6_Byte3: byte[4]:  05|Key_DW6_Byte2: byte[5]:  6e|Key_DW6_Byte1: byte[6]:  02|Key_DW6_Byte0: byte[7]:  e0|
|Key_DW5_Byte3: byte[8]:  ef|Key_DW5_Byte2: byte[9]:  8b|Key_DW5_Byte1: byte[10]: 7f|Key_DW5_Byte0: byte[11]: eb|
|Key_DW4_Byte3: byte[12]: 97|Key_DW4_Byte2: byte[13]: 39|Key_DW4_Byte1: byte[14]: d4|Key_DW4_Byte0: byte[15]: d9|
|Key_DW3_Byte3: byte[16]: 6a|Key_DW3_Byte2: byte[17]: 4e|Key_DW3_Byte1: byte[18]: b8|Key_DW3_Byte0: byte[19]: 01|
|Key_DW2_Byte3: byte[20]: 03|Key_DW2_Byte2: byte[21]: 24|Key_DW2_Byte1: byte[22]: 1d|Key_DW2_Byte0: byte[23]: f7|
|Key_DW1_Byte3: byte[24]: cd|Key_DW1_Byte2: byte[25]: 5e|Key_DW1_Byte1: byte[26]: 24|Key_DW1_Byte0: byte[27]: b4|
|Key_DW0_Byte3: byte[28]: 9c|Key_DW0_Byte2: byte[29]: cd|Key_DW0_Byte1: byte[30]: 27|Key_DW0_Byte0: byte[31]: 20|
|IV_DW2_Byte3:  byte[0]:  80|IV_DW2_Byte2:  byte[1]:  00|IV_DW2_Byte1:  byte[2]:  00|IV_DW2_Byte0:  byte[3]:  00|
|IV_DW1_Byte3:  byte[4]:  00|IV_DW1_Byte2:  byte[5]:  00|IV_DW1_Byte1:  byte[6]:  00|IV_DW1_Byte0:  byte[7]:  00|
|IV_DW0_Byte3:  byte[8]:  00|IV_DW0_Byte2:  byte[9]:  00|IV_DW0_Byte1:  byte[8]:  00|IV_DW0_Byte0:  byte[11]: 01|

## Intel Root Complex CXL Key/IV register

The Intel Root Complex CXL Key/IV register is following: (Match AES-GCM nature order)

| CXL Root Port Key/IV |IDE_KM Mapping|AES-GCM Mapping|
|----------------------|--------------|---------------|
|Link_Enc_Key_0[7:0]   |Key_DW7_Byte3 |byte[0]:  df   |
|Link_Enc_Key_0[15:8]  |Key_DW7_Byte2 |byte[1]:  25   |
|Link_Enc_Key_0[23:16] |Key_DW7_Byte1 |byte[2]:  41   |
|Link_Enc_Key_0[31:24] |Key_DW7_Byte0 |byte[3]:  52   |
|Link_Enc_Key_0[39:32] |Key_DW6_Byte3 |byte[4]:  05   |
|Link_Enc_Key_0[47:40] |Key_DW6_Byte2 |byte[5]:  6e   |
|Link_Enc_Key_0[55:48] |Key_DW6_Byte1 |byte[6]:  02   |
|Link_Enc_Key_0[63:56] |Key_DW6_Byte0 |byte[7]:  e0   |
|-                     |-             |-              |
|Link_Enc_Key_1[7:0]   |Key_DW5_Byte3 |byte[8]:  ef   |
|Link_Enc_Key_1[15:8]  |Key_DW5_Byte2 |byte[9]:  8b   |
|Link_Enc_Key_1[23:16] |Key_DW5_Byte1 |byte[10]: 7f   |
|Link_Enc_Key_1[31:24] |Key_DW5_Byte0 |byte[11]: eb   |
|Link_Enc_Key_1[39:32] |Key_DW4_Byte3 |byte[12]: 97   |
|Link_Enc_Key_1[47:40] |Key_DW4_Byte2 |byte[13]: 39   |
|Link_Enc_Key_1[55:48] |Key_DW4_Byte1 |byte[14]: d4   |
|Link_Enc_Key_1[63:56] |Key_DW4_Byte0 |byte[15]: d9   |
|-                     |-             |-              |
|Link_Enc_Key_2[7:0]   |Key_DW3_Byte3 |byte[16]: 6a   |
|Link_Enc_Key_2[15:8]  |Key_DW3_Byte2 |byte[17]: 4e   |
|Link_Enc_Key_2[23:16] |Key_DW3_Byte1 |byte[18]: b8   |
|Link_Enc_Key_2[31:24] |Key_DW3_Byte0 |byte[19]: 01   |
|Link_Enc_Key_2[39:32] |Key_DW2_Byte3 |byte[20]: 03   |
|Link_Enc_Key_2[47:40] |Key_DW2_Byte2 |byte[21]: 24   |
|Link_Enc_Key_2[55:48] |Key_DW2_Byte1 |byte[22]: 1d   |
|Link_Enc_Key_2[63:56] |Key_DW2_Byte0 |byte[23]: f7   |
|-                     |-             |-              |
|Link_Enc_Key_3[7:0]   |Key_DW1_Byte3 |byte[24]: cd   |
|Link_Enc_Key_3[15:8]  |Key_DW1_Byte2 |byte[25]: 5e   |
|Link_Enc_Key_3[23:16] |Key_DW1_Byte1 |byte[26]: 24   |
|Link_Enc_Key_3[31:24] |Key_DW1_Byte0 |byte[27]: b4   |
|Link_Enc_Key_3[39:32] |Key_DW0_Byte3 |byte[28]: 9c   |
|Link_Enc_Key_3[47:40] |Key_DW0_Byte2 |byte[29]: cd   |
|Link_Enc_Key_3[55:48] |Key_DW0_Byte1 |byte[30]: 27   |
|Link_Enc_Key_3[63:56] |Key_DW0_Byte0 |byte[31]: 20   |
|-                     |-             |-              |
|Link_Enc_IV[7:0]      |IV_DW0_Byte0  |byte[11]: 01   |
|Link_Enc_IV[15:8]     |IV_DW0_Byte1  |byte[10]: 00   |
|Link_Enc_IV[23:16]    |IV_DW0_Byte2  |byte[9]:  00   |
|Link_Enc_IV[31:24]    |IV_DW0_Byte3  |byte[8]:  00   |
|Link_Enc_IV[39:32]    |IV_DW1_Byte0  |byte[7]:  00   |
|Link_Enc_IV[47:40]    |IV_DW1_Byte1  |byte[6]:  00   |
|Link_Enc_IV[55:48]    |IV_DW1_Byte2  |byte[5]:  00   |
|Link_Enc_IV[63:56]    |IV_DW1_Byte3  |byte[4]:  00   |

## PCIE IDE_KM KEY_PROG message

The PCIE IDE_KM KEY_PROG message is following: (Same as CXL IDE_KM)

| Bits[31:24]               | Bits[23:16]               | Bits[15:8]                | Bits[7:0]                 |
|---------------------------|---------------------------|---------------------------|---------------------------|
|Key_DW7_Byte3: byte[0]:  df|Key_DW7_Byte2: byte[1]:  25|Key_DW7_Byte1: byte[2]:  41|Key_DW7_Byte0: byte[3]:  52|
|Key_DW6_Byte3: byte[4]:  05|Key_DW6_Byte2: byte[5]:  6e|Key_DW6_Byte1: byte[6]:  02|Key_DW6_Byte0: byte[7]:  e0|
|Key_DW5_Byte3: byte[8]:  ef|Key_DW5_Byte2: byte[9]:  8b|Key_DW5_Byte1: byte[10]: 7f|Key_DW5_Byte0: byte[11]: eb|
|Key_DW4_Byte3: byte[12]: 97|Key_DW4_Byte2: byte[13]: 39|Key_DW4_Byte1: byte[14]: d4|Key_DW4_Byte0: byte[15]: d9|
|Key_DW3_Byte3: byte[16]: 6a|Key_DW3_Byte2: byte[17]: 4e|Key_DW3_Byte1: byte[18]: b8|Key_DW3_Byte0: byte[19]: 01|
|Key_DW2_Byte3: byte[20]: 03|Key_DW2_Byte2: byte[21]: 24|Key_DW2_Byte1: byte[22]: 1d|Key_DW2_Byte0: byte[23]: f7|
|Key_DW1_Byte3: byte[24]: cd|Key_DW1_Byte2: byte[25]: 5e|Key_DW1_Byte1: byte[26]: 24|Key_DW1_Byte0: byte[27]: b4|
|Key_DW0_Byte3: byte[28]: 9c|Key_DW0_Byte2: byte[29]: cd|Key_DW0_Byte1: byte[30]: 27|Key_DW0_Byte0: byte[31]: 20|
|IFV_DW1_Byte3: byte[4]:  00|IFV_DW1_Byte2: byte[5]:  00|IFV_DW1_Byte1: byte[6]:  00|IFV_DW1_Byte0: byte[7]:  00|
|IFV_DW0_Byte3: byte[8]:  00|IFV_DW0_Byte2: byte[9]:  00|IFV_DW0_Byte1: byte[10]: 00|IFV_DW0_Byte0: byte[11]: 01|

## Intel Root Complex PCIE Key/IV register

The Intel Root Complex PCIE Key/IV register is following:  (Match IDE_KM)

| PCIE Root Port Key/IV |IDE_KM Mapping|AES-GCM Mapping|
|-----------------------|--------------|---------------|
|Key_Slot_DW0[7:0]      |Key_DW0_Byte0 |byte[31]: 20   |
|Key_Slot_DW0[15:8]     |Key_DW0_Byte1 |byte[30]: 27   |
|Key_Slot_DW0[23:16]    |Key_DW0_Byte2 |byte[29]: cd   |
|Key_Slot_DW0[31:24]    |Key_DW0_Byte3 |byte[28]: 9c   |
|-                      |-             |-              |
|Key_Slot_DW1[7:0]      |Key_DW1_Byte0 |byte[27]: b4   |
|Key_Slot_DW1[15:8]     |Key_DW1_Byte1 |byte[26]: 24   |
|Key_Slot_DW1[23:16]    |Key_DW1_Byte2 |byte[25]: 5e   |
|Key_Slot_DW1[31:24]    |Key_DW1_Byte3 |byte[24]: cd   |
|-                      |-             |-              |
|Key_Slot_DW2[7:0]      |Key_DW2_Byte0 |byte[23]: f7   |
|Key_Slot_DW2[15:8]     |Key_DW2_Byte1 |byte[22]: 1d   |
|Key_Slot_DW2[23:16]    |Key_DW2_Byte2 |byte[21]: 24   |
|Key_Slot_DW2[31:24]    |Key_DW2_Byte3 |byte[20]: 03   |
|-                      |-             |-              |
|Key_Slot_DW3[7:0]      |Key_DW3_Byte0 |byte[19]: 01   |
|Key_Slot_DW3[15:8]     |Key_DW3_Byte1 |byte[18]: b8   |
|Key_Slot_DW3[23:16]    |Key_DW3_Byte2 |byte[17]: 4e   |
|Key_Slot_DW3[31:24]    |Key_DW3_Byte3 |byte[16]: 6a   |
|-                      |-             |-              |
|Key_Slot_DW4[7:0]      |Key_DW4_Byte0 |byte[15]: d9   |
|Key_Slot_DW4[15:8]     |Key_DW4_Byte1 |byte[14]: d4   |
|Key_Slot_DW4[23:16]    |Key_DW4_Byte2 |byte[13]: 39   |
|Key_Slot_DW4[31:24]    |Key_DW4_Byte3 |byte[12]: 97   |
|-                      |-             |-              |
|Key_Slot_DW5[7:0]      |Key_DW5_Byte0 |byte[11]: eb   |
|Key_Slot_DW5[15:8]     |Key_DW5_Byte1 |byte[10]: 7f   |
|Key_Slot_DW5[23:16]    |Key_DW5_Byte2 |byte[9]:  8b   |
|Key_Slot_DW5[31:24]    |Key_DW5_Byte3 |byte[8]:  ef   |
|-                      |-             |-              |
|Key_Slot_DW6[7:0]      |Key_DW6_Byte0 |byte[7]:  e0   |
|Key_Slot_DW6[15:8]     |Key_DW6_Byte1 |byte[6]:  02   |
|Key_Slot_DW6[23:16]    |Key_DW6_Byte2 |byte[5]:  6e   |
|Key_Slot_DW6[31:24]    |Key_DW6_Byte3 |byte[4]:  05   |
|-                      |-             |-              |
|Key_Slot_DW7[7:0]      |Key_DW7_Byte0 |byte[3]:  52   |
|Key_Slot_DW7[15:8]     |Key_DW7_Byte1 |byte[2]:  41   |
|Key_Slot_DW7[23:16]    |Key_DW7_Byte2 |byte[1]:  25   |
|Key_Slot_DW7[31:24]    |Key_DW7_Byte3 |byte[0]:  df   |
|-                      |-             |-              |
|IFV_DW0[7:0]           |IFV_DW0_Byte0 |byte[11]: 01   |
|IFV_DW0[15:8]          |IFV_DW0_Byte1 |byte[10]: 00   |
|IFV_DW0[23:16]         |IFV_DW0_Byte2 |byte[9]:  00   |
|IFV_DW0[31:24]         |IFV_DW0_Byte3 |byte[8]:  00   |
|-                      |-             |-              |
|IFV_DW1[7:0]           |IFV_DW1_Byte0 |byte[7]:  00   |
|IFV_DW1[15:8]          |IFV_DW1_Byte1 |byte[6]:  00   |
|IFV_DW1[23:16]         |IFV_DW1_Byte2 |byte[5]:  00   |
|IFV_DW1[31:24]         |IFV_DW1_Byte3 |byte[4]:  00   |
