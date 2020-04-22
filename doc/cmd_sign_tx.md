## Sign Transaction

This instruction constructs and signs a transaction for given transaction inputs and provides ECDSA signature outputs.
Transaction signing process consists of several APDU messages.

The input data is RLP (Recursive Length Prefix) encoded transaction data without signature elements 
(values of *v*, *r*, and *s*). The data is streamed to Ledger via sequence of chunks. User is presented
with important transaction elements on screen before transaction is signed. Any rejection during the
process terminates transaction signature provisioning.

User validates:
- Source address
- Value to be transferred
- Gas price for the transaction
- Gas limit for the transaction
- Recipient address

### Command Coding

We use 3 types of APDU blocks to communicate during the signing process.
1) **Initialize Transaction** Signing block for source address construction.
2) **Transaction Details** block for RLP encoded transaction data streaming.
3) **Final Confirmation** block for processing the transaction data and building the signature.

#### Input data

**1) Initialize Transaction Signing block**
 
| *CLA* | *INS* | *P1* | *P2* |   *Lc*   |
|-------|-------|------|------|----------|
|  0xE0 |  0x20 | 0x00 | 0x00 | variable |

Data payload of the first transaction block contains BIP32 derivations setup. This will allow to construct source
address.

| Description | Number of BIP32 Derivations | First Der. Index | ... | Last Der. Index | 
|-------------|-----------------------------|------------------|-----|-----------------|
| Size (Byte) |    1                        |        4         |     |       4         |

###### Response Payload
The initialization block does not respond with any payload. Only confirmation, or rejection status message
is given back.  

**2) Transaction Details block**

| *CLA* | *INS* | *P1* | *P2* |   *Lc*   |
|-------|-------|------|------|----------|
|  0xE0 |  0x20 | 0x01 | 0x00 | variable |

Data payload of the subsequent transaction block container is RLP encoded transaction data.

| Description |  RLP Chunk  | 
|-------------|-------------|
| Size (Byte) |   variable  |

###### Response Payload:

|Description: |  *stage*  |
|-------------|-----------|
|Size:        |     1     |

The application responds with the current transaction processing stage. Following stages
are recognized and used by the application:

  - 0 (SIGN_STAGE_NONE): transaction is not being processed.
  - 1 (SIGN_STAGE_INIT): transaction processing is initialized.
  - 2 (SIGN_STAGE_COLLECT): transaction is being processed and tx data is expected to be received.  
  - 4 (SIGN_STAGE_FINALIZE): transaction processing is done and all the tx data has been collected.
  - 8 (SIGN_STAGE_DONE): transaction processing and signature building is finished.

Expect to receive either SIGN_STAGE_COLLECT, or SIGN_STAGE_FINALIZE on normal operations. The application 
is either waiting to receive more transaction data on the next APDU chunk, or it already has all 
the data it needs and is ready to go through the final transaction content verification, user confirmation
and signing process.

If you receive anything else than these two stages, the application is in an unpredicted state 
and you should stop sending new data and advice user to disconnect and re-connect the device.
The application is actively trying to prevent unpredicted state, please see the application 
responsibility section below.  

**3) Final Confirmation block**

| *CLA* | *INS* | *P1* | *P2* | *Lc* |
|-------|-------|------|------|------|
|  0xE0 |  0x20 | 0x80 | 0x00 | 0x00 |

###### Response Payload:

|Description: |  *v*  |  *r*  |  *s*  |
|-------------|-------|-------|-------|
|Size:        |   1   |   32  |   32  |

#### Application responsibility

Validate content of fields P1, P2, and Lc. All parameters are expected
to be set to defined values. Any other value will be identified
as an error and responded with error message.

Validate BIP32 derivation path on the Initialize Transaction Signing block 
to be valid withing Fantom address space. Minimal path length is 0x02 
and maximal path length is 0x0a. See [Get Address Instruction](cmd_get_address.md) for
any details.

Validate Lc. Lc >= 1; Lc = 1 + (BIP32 Derivations * 4).
 
Calculate the source address and let user validate it before continuing.

On Subsequent Transaction Details block the app validates instruction and previous 
block state validity, the previous block must be either P1 = 0x00, or P1 = 0x01. Any other
previous block invalidates the process and terminates the signing procedure with 
an error message.

Once the transaction details are provided, last block initializes final confirmation process. 
Application verifies the previous block was the signing instruction with P1 = 0x01 and parses
RLP data of the transaction. On failed RLP parse, the signing process terminates with an error message.

User is provided with several other transaction details to be confirmed. Please check the list of confirmed elements
above. Any rejection terminates the signing process.

On successful final confirmation and approval from user, 
the app builds the transaction and calculates ECDSA values of *v*, *r*, and *s*.

The application responds with *v*, *r*, and *s* values so the transaction can be sent to processing.
