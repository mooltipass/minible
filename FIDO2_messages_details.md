## FIDO2 Authenticate Credential Request

This message is sent as a request by the AUX MCU to authenticate a FIDO2 credential.
The MAIN MCU responds with [FIDO2 Authenticate Credential Response](FIDO2_messages_details.md#fido2-authenticate-credential-response).
The message is used to check whether the authenticator has the specified credential or not.

**From AUX MCU:**

| byte 8 - 259 | byte 260 - 275    |
|:-------------|:------------------|
| RPID         | Credential ID     |


**RPID**

[RPID](https://www.w3.org/TR/webauthn/#rp-id) for the request.

**Credental ID**

The [CredentialID](https://www.w3.org/TR/webauthn/#credential-id) that the authenticator is requested to check
whether it already exists or not.

## FIDO2 Authenticate Credential Response

This message is sent as a response to request by the AUX MCU to authenticate a FIDO2 credential.

**From MAIN MCU:**

| byte 8 - 23    | byte 24 - 87 | byte 88             | byte 89 |
|:---------------|:-------------|:-------------------:|---------|
| Credential ID  | User handle  | User handle length  | Result  |

**Credental ID**

The [CredentialID](https://www.w3.org/TR/webauthn/#credential-id) that was checked.

**User handle**

[User handle](https://www.w3.org/TR/webauthn/#dom-publickeycredentialuserentity-id) associated with the credential

**User handle length**

The length of the user handle.

**Result**

| Value  | Description                                |
|:-------|:-------------------------------------------|
| 0x00   | Authenticator does not have the credential |
| 0x01   | Authenticator does have the credential     |

## FIDO2 Make Credential Request

This message is sent as a request by the AUX MCU to create a credential for a FIDO2 credential.
The MAIN MCU responds with [FIDO2 Make Credential Response](FIDO2_messages_details.md#fido2-make-credential-response).
See FIDO2 specification for definition of the values that are not explicitly defined here.

**From AUX MCU:**

| byte 8 - 259 | byte 260 - 323 | byte 324           | byte 325 - 389 | byte 390 - 454 | byte 455 - 486   |
|:-------------|:---------------|-------------------:|----------------|:--------------:|------------------|
| RPID         | User Handle    | User handle length | User name      | Display name   | Client Data Hash |

**RPID**

[RPID](https://www.w3.org/TR/webauthn/#rp-id) for the request.

**User Handle**

[User handle](https://www.w3.org/TR/webauthn/#dom-publickeycredentialuserentity-id) associated with the credential

**User handle length**

The length of the user handle.

**Display name**

[Display name](https://www.w3.org/TR/webauthn/#dom-publickeycredentialuserentity-displayname) as per FIDO2 spec.

**Client data hash**

[Client data hash](https://www.w3.org/TR/webauthn/#collectedclientdata-hash-of-the-serialized-client-data)

## FIDO2 Make Credential Response

This message is sent as a response to request by the AUX MCU to make credential for a FIDO2 credential.
See FIDO2 specification for definition of the values that are not explicitly defined here.

**From MAIN MCU:**

| byte 8 - 23    | byte 24 - 55  | byte 56 - 59         | byte 60 - 63   | byte 64 - 95           |
|:---------------|:--------------|:---------------------|:---------------|:-----------------------|
| Credential ID  | RPID Hash     | Count (in BE format) | Flags          | Public key x component |

| byte 96 - 127          | byte 128 - 191   | byte 192 - 207 | byte 208 - 211       | byte 212   |
|:-----------------------|------------------|:---------------|:---------------------|:-----------|
| Public key y component | Attest signature | AAGUID         | Credential ID length | Error code |

**Credental ID**

The [CredentialID](https://www.w3.org/TR/webauthn/#credential-id) for the entry to check

**RPID Hash**

SHA256 hash of the RPID

**Count (in BE format)**

[Count](https://www.w3.org/TR/webauthn/#signcount) as per FIDO2 spec.

**Flags**

[Flags](https://www.w3.org/TR/webauthn/#flags) as per FIDO2 spec.

**Public key x/y components**

[Public key x/y components](https://www.w3.org/TR/webauthn/#credentialpublickey), [encoding](https://www.w3.org/TR/webauthn/#sctn-encoded-credPubKey-examples)

**Attest signature**

[Attest signature](https://www.w3.org/TR/webauthn/#attestation-signature)

**AAGUID**

[AAGUID](https://www.w3.org/TR/webauthn/#aaguid)

**Credential ID length**

[Credential ID length](https://www.w3.org/TR/webauthn/#credentialidlength)

**Error code:**

| Value | Description                                                           |
|:-------|:---------------------------------------------------------------------|
| 0x00   | Success. Credential created and stored                               |
| 0x01   | Operation Denied. User denied the operation                          |
| 0x02   | User not present. The user prompt to create the credential timed out |
| 0x03   | Storage exhausted. The database storage does not have room           |


## FIDO2 Get Assertion Request

This message is sent as a request by the AUX MCU to create assert a FIDO2 credential.
The MAIN MCU responds with [FIDO2 Get Assertion Response](FIDO2_messages_details.md#fido2-get-assertion-response).
See FIDO2 specification for definition of the values that are not explicitly defined here.

**From AUX MCU:**

| byte 8 - 259 | byte 260 - 291   | byte 292 - 455 | byte 456 |
|:-------------|:-----------------|:---------------|:---------|
| RPID         | Client Data Hash | Allow List     | Flags    |

**RPID**

[RPID](https://www.w3.org/TR/webauthn/#rp-id) for the request.

**Client data hash**

[Client data hash](https://www.w3.org/TR/webauthn/#collectedclientdata-hash-of-the-serialized-client-data)

**Allow List**

Allow list is the list of allowed credentials to be asserted. The list can contain 0 or more credentials.
If 1 or more credentials is specified we can only assert one of these (based on user selection).
See parameter name "allowList" at https://fidoalliance.org/specs/fido-v2.0-ps-20190130/fido-client-to-authenticator-protocol-v2.0-ps-20190130.html#authenticatorGetAssertion

| byte 292 - 295 | byte 296 - 311  | byte 312 - 327  | byte 328 - 439 | byte 440 - 455   |
|:---------------|:----------------|:----------------|:---------------|:-----------------|
| Length         | Credential ID 1 | Credential ID 2 |       ...      | Credential ID 10 |

**Length**

Number of "Credential IDs" following. Valid value is 0 - 10.

**Credential ID X**

The [CredentialID](https://www.w3.org/TR/webauthn/#credential-id) to present to the user to ask for assertion.

**Flags**

Flag for the operation

| Value | Description                                                           |
|:-------|:---------------------------------------------------------------------|
| 0x00   | Normal operation
| 0x01   | Silent Get Assertion. Do not prompt user



## FIDO2 Get Assertion Response

This message is sent as a response to request by the AUX MCU to a Get Assertion Request.
See FIDO2 specification for definition of the values that are not explicitly defined here.

**From MAIN MCU:**

| byte 8 - 23    | byte 24 - 87 | byte 88            | byte 89 - 120 | byte 121 - 124       | byte 125 - 128 |
|:---------------|:-------------|:-------------------|:--------------|:---------------------|:---------------|
| Credential ID  | User Handle  | User Handle Length | RPID Hash     | Count (in BE format) | Flags          |

| byte 129 - 192   | byte 193 - 208 | byte 209   |
|:-----------------|:---------------|:-----------|
| Attest signature | AAGUID         | Error code |

**Credental ID**

The [CredentialID](https://www.w3.org/TR/webauthn/#credential-id) for the entry to check

**User handle**

[User handle](https://www.w3.org/TR/webauthn/#dom-publickeycredentialuserentity-id) associated with the credential

**User handle length**

The length of the user handle.

**RPID Hash**

SHA256 hash of the RPID

**Count (in BE format)**

[Count](https://www.w3.org/TR/webauthn/#signcount) as per FIDO2 spec.

**Flags**

[Flags](https://www.w3.org/TR/webauthn/#flags) as per FIDO2 spec.

**Attest signature**

[Attest signature](https://www.w3.org/TR/webauthn/#attestation-signature)

**AAGUID**

[AAGUID](https://www.w3.org/TR/webauthn/#aaguid)

**Error code:**

| Value | Description                                                           |
|:-------|:---------------------------------------------------------------------|
| 0x00   | Success. Credential asserted                                         |
| 0x01   | Operation Denied. User denied the operation                          |
| 0x02   | User not present. The user prompt to assert the credential timed out |
| 0x03   | N/A                                                                  |
| 0x04   | No credentials available for requested RPID                          |

