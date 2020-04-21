## Get Public Key

This instruction returns an extended public key for the given BIP32 path. The call does not
display, nor returns, the address associated with the public key.

### Command Coding

#### Input data

| *CLA* | *INS* | *P1* | *P2* |   *Lc*   |
|-------|-------|------|------|----------|
|  0xE0 |  0x10 | 0x00 | 0x00 | variable |

Data payload contains BIP32 derivations setup.

| Description | Number of BIP32 Derivations | First Der. Index | ... | Last Der. Index | 
|-------------|-----------------------------|------------------|-----|-----------------|
| Size (Byte) |    1                        |        4         |     |       4         |

#### Response Payload

|Description: | Key Length     | Public Key  | Chain Code |
|-------------|----------------|-------------|------------|
|Size:        |   1            |  variable   |  variable  |

#### Application responsibility

Validate content of fields P1, P2, and Lc. All parameters are expected
to be set to defined values. Any other value will be identified
as an error and responded with error message.

Validate BIP32 derivation path to be valid withing Fantom address space.
Minimal path length is 0x02 and maximal path length is 0x0a.

We use BIP44 specification for address derivation, the address namespace
is shared with Ether.

#####Derivation indexes, their meanings and expected values:
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
 
Calculate the public key requested.

Respond with the public key.
