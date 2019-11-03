## [](#header-1) Communications Between Aux and Main MCU
This describes the FIDO2 messages sent between Aux MCU and Main MCU.

## [](#header-2) Message Structure and Serial Link Specs 
Each FIDO2 message is wrapped in the header defined as follows:

| byte 4 - 5         | byte 6 - 7        | byte 8 - 555  |
|:-------------------|:------------------|:--------------|
| FIDO2 Message Type | Reserved          | Payload       |

**FIDO2 Message Type:**
- 0x0001: [FIDO2 Authenticate Credential Request](FIDO2_messages_details.md#fido2-authenticate-credential-request)
- 0x0002: [FIDO2 Authenticate Credential Response](FIDO2_messages_details.md#fido2-authenticate-credential-response)
- 0x0003: [FIDO2 Make Credential Request](FIDO2_messages_details.md#fido2-make-credential-request)
- 0x0004: [FIDO2 Make Credential Response](FIDO2_messages_details.md#fido2-make-credential-response)
- 0x0005: [FIDO2 Get Assertion Request](FIDO2_messages_details.md#fido2-get-assertion-request)
- 0x0006: [FIDO2 Get Assertion Response](FIDO2_messages_details.md#fido2-get-assertion-response)

**Payload**
Payload is described by the individual messages linked above.

