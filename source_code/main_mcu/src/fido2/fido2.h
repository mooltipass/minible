enum fido2_ErrorCode
{
    FIDO2_SUCCESS = 0,
    FIDO2_OPERATION_DENIED = 1,
    FIDO2_USER_NOT_PRESENT = 2,
    FIDO2_STORAGE_EXHAUSTED = 3,
    FIDO2_CRED_NOT_FOUND = 4,
    FIDO2_NO_CREDENTIALS = 5,
};

#define FIDO2_UP_BIT (1 << 0) //User Present
#define FIDO2_UV_BIT (1 << 2) //User Verified
#define FIDO2_AT_BIT (1 << 6) //Attested Credential Data Included
#define FIDO2_ED_BIT (1 << 7) //Extension Data Included

#define FIDO2_CREDENTIAL_EXISTS 1
#define FIDO2_NEW_CREDENTIAL 1

#define FIDO2_MINIBLE_AAGUID ((uint8_t*)"\x6d\xb0\x42\xd0\x61\xaf\x40\x4c\xa8\x87\xe7\x2e\x09\xba\x7e\xb4")


void fido2_process_exclude_list_item(fido2_auth_cred_req_message_t const *request, fido2_auth_cred_rsp_message_t *response);
void fido2_process_make_auth_data(fido2_make_auth_data_req_message_t const *request, fido2_make_auth_data_rsp_message_t *response);

