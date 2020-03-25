## Get Public Key

This instruction returns an extended public key for the given BIP32 path. The call does not
display, nor returns, the address associated with the public key.

### Command Coding

#### Input data

| *CLA* | *INS* | *P1* | *P2* |   *Lc*   |   *Le*   |
|-------|-------|------|------|----------|----------|
|  0xE0 |  0x10 | 0x00 | 0x00 | variable | variable |

Data payload contains BIP32 derivations setup.

| Description | Number of BIP32 Derivations | First Der. Index | ... | Last Der. Index | 
|-------------|-----------------------------|------------------|-----|-----------------|
| Size (Byte) |    1                        |        4         |     |       4         |

#### Response Payload

|Description: | Key Length     | Public Key  |
|-------------|----------------|-------------|
|Size:        |   1            | variable    |

#### Application responsibility

Validate content of fields P1, P2, and Lc. All parameters are expected
to be set to defined values. Any other value will be identified
as an error and responded with error message.

Validate BIP32 derivation path to be valid withing Fantom address space.
Minimal path length is 0x02 and maximal path length is 0x0a.

Validate Lc. Lc >= 1; Lc = 1 + (BIP32 Derivations * 4).
 
Calculate the public key requested.

Respond with the public key.
