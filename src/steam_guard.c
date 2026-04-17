#include "steam_guard.h"
#include <ctype.h>
#include <stdlib.h>

#define MAX_DECODED_SECRET_LEN 64
#define MAX_SHARED_SECRET_LEN 128

typedef struct
{
    BLOBHEADER header;
    DWORD key_size;
    BYTE key_data[MAX_DECODED_SECRET_LEN];
} hmac_key_blob_t;

int gen_auth_code(char *out, const char *shared_secret, const int server_time_diff)
{
    const char code_dict[27] = "23456789BCDFGHJKMNPQRTVWXY";
    const int code_dict_size = strlen(code_dict);
    unsigned char dec_shared_secret[MAX_DECODED_SECRET_LEN];
    char shared_secret_buf[MAX_SHARED_SECRET_LEN];
    size_t shared_secret_end = strlen(shared_secret);
    size_t shared_secret_offset = 0;

    while (shared_secret_offset < shared_secret_end && isspace((unsigned char)shared_secret[shared_secret_offset]))
        shared_secret_offset++;
    while (shared_secret_end > shared_secret_offset && isspace((unsigned char)shared_secret[shared_secret_end - 1]))
        shared_secret_end--;

    size_t shared_secret_len = shared_secret_end - shared_secret_offset;

    if (0 == shared_secret_len || shared_secret_len >= sizeof(shared_secret_buf))
        return 1;

    memcpy(shared_secret_buf, shared_secret + shared_secret_offset, shared_secret_len);
    shared_secret_buf[shared_secret_len] = '\0';

#ifdef _WIN32
    DWORD dec_shared_secret_len = MAX_DECODED_SECRET_LEN;
    if (!CryptStringToBinaryA(shared_secret_buf, shared_secret_len, CRYPT_STRING_BASE64, dec_shared_secret, &dec_shared_secret_len, NULL, NULL))
        return 1;
    if (0 == dec_shared_secret_len)
        return 1;
#else
    int dec_shared_secret_len = EVP_DecodeBlock(dec_shared_secret, (unsigned char *)shared_secret_buf, shared_secret_len);

    if (-1 == dec_shared_secret_len)
        return 1;

    for (size_t i = shared_secret_len; i > 0 && '=' == shared_secret_buf[i - 1]; i--)
    {
        dec_shared_secret_len--;
    }

    if (dec_shared_secret_len <= 0 || dec_shared_secret_len > (int)sizeof(dec_shared_secret))
        return 1;
#endif

    int time = get_steam_time(server_time_diff) / 30;
    uint8_t time_array[8];
    char code_array[6];

    for (int i = 8; i > 0; i--)
    {
        time_array[i - 1] = time & 255;
        time >>= 8;
    }

#ifdef _WIN32
    HCRYPTPROV crypt_provider = 0;
    HCRYPTKEY crypt_key = 0;
    HCRYPTHASH crypt_hash = 0;
    BYTE hdata[20];
    DWORD hdata_len = sizeof(hdata);

    hmac_key_blob_t key_blob;

    memset(&key_blob, 0, sizeof(key_blob));
    key_blob.header.bType = PLAINTEXTKEYBLOB;
    key_blob.header.bVersion = CUR_BLOB_VERSION;
    // CALG_RC2 is used as a dummy algorithm identifier for CRYPT_IPSEC_HMAC_KEY imports.
    key_blob.header.aiKeyAlg = CALG_RC2;
    key_blob.key_size = dec_shared_secret_len;
    memcpy(key_blob.key_data, dec_shared_secret, dec_shared_secret_len);

    if (!CryptAcquireContextA(&crypt_provider, NULL, MS_ENHANCED_PROV_A, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        crypt_provider = 0;
        if (!CryptAcquireContextA(&crypt_provider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
            return 1;
    }

    if (!CryptImportKey(crypt_provider, (BYTE *)&key_blob, sizeof(BLOBHEADER) + sizeof(DWORD) + dec_shared_secret_len, 0, CRYPT_IPSEC_HMAC_KEY, &crypt_key))
    {
        SecureZeroMemory(&key_blob, sizeof(key_blob));
        CryptReleaseContext(crypt_provider, 0);
        return 1;
    }

    if (!CryptCreateHash(crypt_provider, CALG_HMAC, crypt_key, 0, &crypt_hash))
    {
        SecureZeroMemory(&key_blob, sizeof(key_blob));
        CryptDestroyKey(crypt_key);
        CryptReleaseContext(crypt_provider, 0);
        return 1;
    }

    HMAC_INFO hmac_info;
    memset(&hmac_info, 0, sizeof(hmac_info));
    hmac_info.HashAlgid = CALG_SHA1;

    if (!CryptSetHashParam(crypt_hash, HP_HMAC_INFO, (BYTE *)&hmac_info, 0))
    {
        SecureZeroMemory(&key_blob, sizeof(key_blob));
        CryptDestroyHash(crypt_hash);
        CryptDestroyKey(crypt_key);
        CryptReleaseContext(crypt_provider, 0);
        return 1;
    }

    if (!CryptHashData(crypt_hash, time_array, sizeof(time_array), 0) ||
        !CryptGetHashParam(crypt_hash, HP_HASHVAL, hdata, &hdata_len, 0))
    {
        SecureZeroMemory(&key_blob, sizeof(key_blob));
        CryptDestroyHash(crypt_hash);
        CryptDestroyKey(crypt_key);
        CryptReleaseContext(crypt_provider, 0);
        return 1;
    }

    SecureZeroMemory(&key_blob, sizeof(key_blob));
    CryptDestroyHash(crypt_hash);
    CryptDestroyKey(crypt_key);
    CryptReleaseContext(crypt_provider, 0);
#else
    uint8_t *hdata = HMAC(EVP_sha1(), dec_shared_secret, dec_shared_secret_len, time_array, 8, NULL, NULL);
    if (NULL == hdata)
        return 1;
#endif

    uint8_t b = hdata[19] & 0xF;
    int codepoint = ((hdata[b] & 0x7F) << 24) |
                    ((hdata[b + 1] & 0xFF) << 16) |
                    ((hdata[b + 2] & 0xFF) << 8) |
                    (hdata[b + 3] & 0xFF);

    for (int i = 0; i < 5; i++)
    {
        code_array[i] = code_dict[codepoint % code_dict_size];
        codepoint /= code_dict_size;
    }

    code_array[5] = '\0';
    memcpy(out, code_array, 6);

    return 0;
} // gen_auth_code
