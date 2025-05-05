
// json curl_get_request(const std::string& url, const std::string& auth_header = "") {
//     CURL* curl = curl_easy_init();
//     std::string response_string;
//     long http_code = 0;
//     using Clock = std::chrono::high_resolution_clock;
//     auto start = Clock::now();

//     if (curl) {
//         struct curl_slist* headers = nullptr;
//         if (!auth_header.empty()) {
//             headers = curl_slist_append(headers, auth_header.c_str());
//         }

//         curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
//         if (headers) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
//         curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
//         curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
//         curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

//         CURLcode res = curl_easy_perform(curl);
//         if (res != CURLE_OK) {
//             curl_easy_cleanup(curl);
//             return {
//                 {"error", "CURL request failed"},
//                 {"message", curl_easy_strerror(res)}
//             };
//         }

//         curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
//         curl_slist_free_all(headers);
//         curl_easy_cleanup(curl);
//     }

//     auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start);
//     return handle_response(response_string, http_code, latency);
// }



// size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* output) {
//     size_t totalSize = size * nmemb;
//     output->append((char*)contents, totalSize);
//     return totalSize;
// }