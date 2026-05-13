/*
 * @FilePath     : http_send.c
 * @Author       : huangjunda
 * @Date         : 2024-08-14 11:14:31
 * @LastEditors  : huangjunda
 * @LastEditTime : 2024-08-21 09:11:20
 * @Description  : 
 */

#include "http_send.h"


// 定义一个用于存储响应数据的结构
struct ResponseData 
{
    char *data;
    size_t size;
};

// 回调函数，用于处理响应数据
size_t WriteCallback(void *ptr, size_t size, size_t nmemb, struct ResponseData *response) 
{
    size_t newSize = response->size + size * nmemb;
    response->data = realloc(response->data, newSize + 1); // +1 for null terminator
    if (response->data == NULL) {
        fprintf(stderr, "Not enough memory (realloc returned NULL)\n");
        return 0;
    }
    memcpy(response->data + response->size, ptr, size * nmemb);
    response->data[newSize] = '\0'; // Null terminate the string
    response->size = newSize;

    return size * nmemb;
}

// 函数：将 JSON 数据转换为 URL 查询字符串
char* json_to_query_string(const char *json_str) 
{
    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL) {
        fprintf(stderr, "Failed to parse JSON\n");
        return NULL;
    }

    char *query_string = malloc(4096);
    if (!query_string) {
        fprintf(stderr, "Failed to allocate memory for query string\n");
        cJSON_Delete(json);
        return NULL;
    }
    
    query_string[0] = '\0'; // 初始化为空字符串

    cJSON *item = NULL;
    cJSON_ArrayForEach(item, json) {
        if (cJSON_IsString(item)) {
            // 处理字符串类型
            strcat(query_string, item->string);
            strcat(query_string, "=");
            strcat(query_string, item->valuestring);
            strcat(query_string, "&");
        } else if (cJSON_IsNumber(item)) {
            // 处理数字类型
            char value[20]; // 假设数字不会超过 20 位
            snprintf(value, sizeof(value), "%g", item->valuedouble); // 使用 %g 处理整数和浮点数
            strcat(query_string, item->string);
            strcat(query_string, "=");
            strcat(query_string, value);
            strcat(query_string, "&");
        }
    }

    // 去掉最后一个 '&'
    size_t len = strlen(query_string);
    if (len > 0) {
        query_string[len - 1] = '\0';
    }

    cJSON_Delete(json);
    return query_string;
}


//用于访问http接口
int http_send(const char *pUrl, const int nType,const char *pToken, const char *pInput, char **pOutput)
{
    CURL *curl;
    CURLcode res;
    struct ResponseData response = {NULL, 0}; // 初始化响应数据
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // 设置 URL
        curl_easy_setopt(curl, CURLOPT_URL, pUrl);

        // 根据请求类型设置相应的选项
        if (nType == 0) 
        {   
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, pInput);
        } 
        else if (nType == 1) 
        {
            char *query_string = json_to_query_string(pInput);
            char full_url[2048];
            snprintf(full_url, sizeof(full_url), "%s?%s", pUrl, query_string);
            curl_easy_setopt(curl, CURLOPT_URL, full_url);
        } else {
            curl_easy_cleanup(curl);
            return -1;
        }
        // 设置请求头
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        char achApiToken[512];
        if (NULL != pToken)
        {
            
            snprintf(achApiToken, sizeof(achApiToken), "ApiToken: %s", pToken);
        }
        headers = curl_slist_append(headers, achApiToken);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        // 设置回调函数以获取响应数据
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // 发起请求
        res = curl_easy_perform(curl);
        // 检查请求是否成功
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            free(response.data);
            return -1;
        }

        // 将响应数据存储到输出参数
        *pOutput = response.data;
        // 清理
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return 0;
}

