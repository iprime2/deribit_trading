#include "CurlHandler.h"
#include <stdexcept>
#include <sstream>
#include <iostream>

CurlHandler::CurlHandler() {
    curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to init CURL");

    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);

#ifdef CURLPIPE_MULTIPLEX
    curl_easy_setopt(curl, CURLOPT_PIPEWAIT, 1L);
#endif
#ifdef CURL_HTTP_VERSION_3
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_3);
#endif
}

CurlHandler::~CurlHandler() {
    if (headers) curl_slist_free_all(headers);
    if (curl) curl_easy_cleanup(curl);
}

size_t CurlHandler::writeCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total = size * nmemb;
    output->append((char*)contents, total);
    return total;
}

std::string CurlHandler::performGet(const std::string& url, const std::string& auth_header, long* http_code) {
    std::string response;
    if (!curl) throw std::runtime_error("CURL not initialized");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    if (!auth_header.empty()) {
        headers = curl_slist_append(headers, auth_header.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("CURL error: ") + curl_easy_strerror(res));
    }

    if (http_code) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_code);
    }

    return response;
}
