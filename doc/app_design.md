# Fantom Ledger Application Design

## Communication Protocol
Fantom Ledger application communicates with a client using APDU protocol.
You can find details of the protocol on 
[ISO 7816-4, Section 5](http://cardwerk.com/smart-card-standard-iso7816-4-section-5-basic-organizations/#chap5_4) 
document page. 

Each command consists of series of APDU messages where each single message uses following format.

### Request


| *Field*       | *CLA* | *INS* | *P1* | *P2* | *Lc* | *Data*   | *Le* |
|---------------|-------|-------|------|------|------|----------|-------
| *Size* (Byte) |   1   |   1   |   1  |   1  |   1  | variable |   1  |


Where:
    - *CLA* is APDU class number.
    - *INS* is the instruction client requests.
    - *P2* and *P2* are the instruction parameters.
    - *Lc* is length of the data body as unsigned `uint8` type. Data lengths greater than 255 bytes are not supported.
    - *Data* is the message payload. Arbitrary data required to perform the instruction.
    - *Le* is the max length of the response.

Any unused fields *must* be set to zero by the sender. The application is responsible for verification 
and validation of the incoming message and if an unused field contains any other value than zero, the
application is responsible for raising en error even if the rest of the message could be processed. 
The reason for this is to avoid unexpected behavior in case of protocol upgrade. Please see below.

For the list of error messages, please refer to [src/errors.h](../src/errors.h).


### Response

Generally the application responds by a data message with much simpler format.
The structure is:

| *Field*       | *Data*   | *SW1* | *SW2* |
|---------------|----------|-------|--------
| *Size* (Byte) | variable |   1   |   1   |

Where:
    - *Data* is the response payload
    - *SW1* and *SW2* is the return code sent with the response. Code 0x9000 means the instruction was processed OK.

### Supported Instructions

Supported instructions are split into groups by their purpose.

#### INS 0x0i Group

This group contains informative and status related instructions. Messages in this group are not expected
to contain any payload.

  - 0x01 ... [Get Application Version](cmd_app_version.md)

#### INS 0x1i Group

This group contains instructions related to public key handling and address derivation.

  - 0x10 ... [Get Public Key](cmd_get_pubkey.md)
  - 0x11 ... [Get Address](cmd_get_address.md)


#### INS 0x2i Group

This group contains instructions related to transaction verification and signing.

  - 0x20 ... [Sign Transaction](cmd_sign_tx.md)


## Protocol Upgrade

We would like to ensure forward compatibility of the protocol with newer version of the application.
For that the sender is required to set all unused fields of a message to **ZERO**. When upgrading,
any previously unused field which will get a meaning will be required to use value **ONE** as the
correct value. IN taht case a previous version of the application will respond with error message
since there will be an unexpected value in the meesage, instead of falling back to some undefined
and unexpected behavior.
