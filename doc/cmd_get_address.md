## Get Address

This instruction returns an address for the given BIP32 path. The address
can be optionally displayed on the device for user verification before
being returned.

### Command Coding

#### Input data

| *CLA* | *INS* | *P1* | *P2* |   *Lc*   |   *Le*   |
|-------|-------|------|------|----------|----------|
|  0xE0 |  0x11 | 0x01 | 0x00 | variable | variable |
|  0xE0 |  0x11 | 0x02 | 0x00 | variable | variable |

Where meaning of the P1 parameter value is:
- 0x01 Return address directly to the host.
- 0x02 Display address on the device screen and let user confirm it before it's returned.

Data payload contains BIP32 derivations setup.

| Description | Number of BIP32 Derivations | First Der. Index | ... | Last Der. Index | 
|-------------|-----------------------------|------------------|-----|-----------------|
| Size (Byte) |    1                        |        4         |     |       4         |

#### Response Payload

|Description: | Address Length | Address  |
|-------------|----------------|----------|
|Size:        |    1           | variable |

#### Application responsibility

Validate content of fields P1, P2, and Lc. All parameters are expected
to be set to defined values. Any other value will be identified
as an error and responded with error message.

Validate BIP32 derivation path to be valid withing Fantom address space.
Minimal path length is 0x02 and maximal path length is 0x0a.

Validate Lc. Lc >= 1; Lc = 1 + (BIP32 Derivations * 4).
 
Calculate the address requested.

If address validation has been requested, the application displays the address
on the device and waits for user interaction before sending the address to host.

Until user confirms, or rejects the address displayed, the application does 
not process and respond to any other incoming APDU traffic.

If validated, respond with account address.
