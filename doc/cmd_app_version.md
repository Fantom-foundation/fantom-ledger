## Get Application Version

This instruction returns the version of the application running on Ledger. 
The function is safe to call at any time.

### Command Coding

#### Input data

| *CLA* | *INS* | *P1* | *p2* | *Lc* |
|-------|-------|------|------|------|
|  0xE0 |  0x01 | 0x00 | 0x00 | 0x00 |
 
#### Response Payload

|Type: | *MINOR* | *MAJOR* | *PATCH* | *FLAGS* |
|------|---------|---------|---------|---------|
|Size: |    1    |    1    |    1    |    1    |

Where tuple [MINOR, MAJOR, PATCH] represents 
the application version. The FLAGS field contains
specified configuration options.

| Mask | Value | Meaning                        |
|------|-------|--------------------------------|
| 0x01 | 0x01  | Identifies development version |

#### Application responsibility

Validate content of fields P1, P2, and Lc. All are expected
to be set to defined values. Any other value will be identified
as an error and responded with error message.

Respond with application version and internal flags.
