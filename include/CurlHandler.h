#pragma once
#include <string>
#include <curl/curl.h>

class CurlHandler {
public:
    CurlHandler();
    ~CurlHandler();

    std::string performGet(const std::string& url, const std::string& auth_header = "", long* http_code = nullptr);

private:
    CURL* curl;
    struct curl_slist* headers = nullptr;
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* output);
};
