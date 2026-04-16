#include "steam_guard.h"

int gen_auth_code(char *out, const char *shared_secret, const int server_time_diff)
{
    const char code_dict[27] = "23456789BCDFGHJKMNPQRTVWXY";
    const int code_dict_size = strlen(code_dict);

#ifdef _WIN32
    DWORD dec_shared_secret_len = 0;
    if (!CryptStringToBinaryA(shared_secret, 0, CRYPT_STRING_BASE64, NULL, &dec_shared_secret_len, NULL, NULL))
        return 1;

    unsigned char dec_shared_secret[64];
    if (dec_shared_secret_len > sizeof(dec_shared_secret))
        return 1;

    if (!CryptStringToBinaryA(shared_secret, 0, CRYPT_STRING_BASE64, dec_shared_secret, &dec_shared_secret_len, NULL, NULL))
        return 1;
#else
    unsigned char dec_shared_secret[32];
    int dec_shared_secret_len = EVP_DecodeBlock(dec_shared_secret, (unsigned char *)shared_secret, strlen(shared_secret));

    if (-1 == dec_shared_secret_len)
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
    BCRYPT_ALG_HANDLE alg_handle = NULL;
    BCRYPT_HASH_HANDLE hash_handle = NULL;
    BYTE hash_object[256];
    DWORD hash_object_len = 0;
    DWORD data_len = 0;
    BYTE hdata[20];
    DWORD hdata_len = sizeof(hdata);

    if (BCryptOpenAlgorithmProvider(&alg_handle, BCRYPT_SHA1_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG) < 0)
        return 1;

    if (BCryptGetProperty(alg_handle, BCRYPT_OBJECT_LENGTH, (PUCHAR)&hash_object_len, sizeof(hash_object_len), &data_len, 0) < 0 ||
        hash_object_len > sizeof(hash_object))
    {
        BCryptCloseAlgorithmProvider(alg_handle, 0);
        return 1;
    }

    if (BCryptCreateHash(alg_handle, &hash_handle, hash_object, hash_object_len, dec_shared_secret, dec_shared_secret_len, 0) < 0)
    {
        BCryptCloseAlgorithmProvider(alg_handle, 0);
        return 1;
    }

    if (BCryptHashData(hash_handle, (PUCHAR)time_array, sizeof(time_array), 0) < 0 ||
        BCryptFinishHash(hash_handle, hdata, hdata_len, 0) < 0)
    {
        BCryptDestroyHash(hash_handle);
        BCryptCloseAlgorithmProvider(alg_handle, 0);
        return 1;
    }

    BCryptDestroyHash(hash_handle);
    BCryptCloseAlgorithmProvider(alg_handle, 0);
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
