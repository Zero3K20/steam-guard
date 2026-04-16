#include "server_time.h"

#ifdef _WIN32
static int get_server_time_diff_winhttp()
{
    HINTERNET session = WinHttpOpen(L"steam-guard/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (NULL == session)
        return 0;

    HINTERNET connection = WinHttpConnect(session, TWO_FACTOR_TIME_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (NULL == connection)
    {
        WinHttpCloseHandle(session);
        return 0;
    }

    HINTERNET request = WinHttpOpenRequest(connection, L"POST", TWO_FACTOR_TIME_PATH, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (NULL == request)
    {
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return 0;
    }

    BOOL sent = WinHttpSendRequest(request, L"Content-Type: application/x-www-form-urlencoded\r\n", -1L, "steamid=0", (DWORD)strlen("steamid=0"), (DWORD)strlen("steamid=0"), 0);
    BOOL received = sent && WinHttpReceiveResponse(request, NULL);

    if (!received)
    {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return 0;
    }

    char *res_buf = NULL;
    size_t res_buf_len = 0;
    DWORD available = 0;

    do
    {
        available = 0;
        if (!WinHttpQueryDataAvailable(request, &available))
            break;

        if (0 == available)
            break;

        char *next_buf = realloc(res_buf, res_buf_len + available + 1);
        if (NULL == next_buf)
            break;

        res_buf = next_buf;
        DWORD downloaded = 0;
        if (!WinHttpReadData(request, res_buf + res_buf_len, available, &downloaded))
            break;

        res_buf_len += downloaded;
        res_buf[res_buf_len] = '\0';
    } while (available > 0);

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    if (NULL == res_buf)
        return 0;

    time_t server_time = 0;
    cJSON *json = cJSON_ParseWithLength(res_buf, res_buf_len);
    free(res_buf);

    if (NULL == json)
        return 0;

    do
    {
        cJSON *j_response = cJSON_GetObjectItemCaseSensitive(json, "response");
        if (!cJSON_IsObject(j_response))
            break;

        cJSON *j_server_time = cJSON_GetObjectItemCaseSensitive(j_response, "server_time");
        if (!cJSON_IsString(j_server_time) || (NULL == j_server_time->valuestring))
            break;

        server_time = atoi(j_server_time->valuestring);

    } while (0);

    cJSON_Delete(json);

    if (0 == server_time)
        return 0;

    return server_time - time(NULL);
}
#else
static size_t write_cb(void *data, size_t size, size_t nmemb, void *clientp)
{
    size_t total_size = size * nmemb;
    char *buffer = (char *)clientp;

    strncat(buffer, (char *)data, total_size);

    return total_size;
} // write_cb

static int get_server_time_diff_curl()
{
    CURL *curl;
    if (!(curl = curl_easy_init()))
        return 0;

    char res_buf[1024] = "";
    CURLcode res_code;

    curl_easy_setopt(curl, CURLOPT_URL, TWO_FACTOR_TIME_QUERY);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "steamid=0");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, STEAM_USERAGENT);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, res_buf);

    res_code = curl_easy_perform(curl);
    
    if (CURLE_OK != res_code)
        fprintf(stderr, "could not get steam's server time: %s\n", curl_easy_strerror(res_code));

    curl_easy_cleanup(curl);

    time_t server_time = 0;

    cJSON *json;
    if ((json = cJSON_ParseWithLength(res_buf, strlen(res_buf))) == NULL)
        return 0;

    do
    {
        cJSON *j_response = cJSON_GetObjectItemCaseSensitive(json, "response");
        if (!cJSON_IsObject(j_response))
            break;

        cJSON *j_server_time = cJSON_GetObjectItemCaseSensitive(j_response, "server_time");
        if (!cJSON_IsString(j_server_time) || (NULL == j_server_time->valuestring))
            break;

        server_time = atoi(j_server_time->valuestring);

    } while (0);

    cJSON_Delete(json);

    if (0 == server_time)
        return 0;

    return server_time - time(NULL);
}
#endif

int get_server_time_diff()
{
#ifdef _WIN32
    return get_server_time_diff_winhttp();
#else
    return get_server_time_diff_curl();
#endif
}

int get_steam_time(int server_time_diff)
{
    return time(NULL) + server_time_diff;
} // get_steam_time
