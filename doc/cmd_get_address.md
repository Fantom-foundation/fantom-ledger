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

We use BIP44 specification for address derivation, the address namespace
is shared with Ether. The minimal expected path contains account field, 
user is warned about unusual request if the path does not follow BIP44
standard.

#####Derivation indexes, their meanings and expected values:
The BIP44 standard defines following five levels of BIP32 path:

`m / purpose' / coin_type' / account' / change / address_index` 

1) **Purpose**: 
    fixed value *44'*.
    
    Following BIP43 recommendation. It indicates that 
    the subtree is used according to BIP44 specification and is hardened.
    
2) **Coin Type**: 
    fixed value *60'*.
    
    This level creates a separate subtree for every crypto coin, avoiding 
    reusing addresses across crypto coins and improving privacy issues.
    Value *60* is registered for *Ether*. Fantom uses the same address space
    at the moment of the app creation.       

3) **Account**
    assume account *0x0* if not specified for the first available account.
    
    This level splits the key space into independent user identities, 
    so the wallet never mixes the coins across different accounts.
    
    Users can use these accounts to organize the funds in the same 
    fashion as bank accounts; for donation purposes 
    (where all addresses are considered public), 
    for saving purposes, for common expenses etc.
    
4) **Change**
    assume change *0x0* for external chain if not specified.

     Constant *0x0* is used for external chain and constant *0x1* for internal 
     chain (also known as change addresses). External chain is used for addresses 
     that are meant to be visible outside of the wallet (e.g. for receiving payments). 
     Internal chain is used for addresses which are not meant to be visible outside 
     of the wallet and is used for return transaction change.
     
     Public derivation is used at this level.
     
5) **Index**
    assume index *0x0* if not specified for the first address of the account.

    Addresses are numbered from index 0 in sequentially increasing manner. 
    This number is used as child index in BIP32 derivation.
    
    Public derivation is used at this level.
                    
Validate Lc. Lc >= 1; Lc = 1 + (BIP32 Derivations * 4).
 
Calculate the address requested.

If address validation has been requested, the application displays the address
on the device and waits for user interaction before sending the address to host.

Until user confirms, or rejects the address displayed, the application does 
not process and respond to any other incoming APDU traffic.

If validated, respond with account address.
