#include "trading.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <iostream>
#include <sstream>
#include <curl/curl.h>

using json = nlohmann::json;

size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

json handle_response(const std::string& response_text, long http_code, const std::chrono::milliseconds& latency) {
    try {
        auto json_resp = json::parse(response_text);
        if (json_resp.contains("usIn") && json_resp.contains("usOut")) {
            long long usIn = json_resp["usIn"].get<long long>();
            long long usOut = json_resp["usOut"].get<long long>();
            long long latency_us = usOut - usIn;

            json_resp["meta"] = {
                {"latency_us", latency_us},
                {"latency_ms", latency_us / 1000.0},
                {"latency_ms_app", latency.count()}
            };
        }
        json_resp["http_status"] = http_code;
        json_resp["latency_ms"] = latency.count();
        return json_resp;
    } catch (const std::exception& e) {
        return {
            {"error", "JSON parse error"},
            {"message", e.what()},
            {"raw_response", response_text},
            {"http_status", http_code},
            {"latency_ms", latency.count()}
        };
    }
}

json curl_get_request(const std::string& url, const std::string& auth_header = "") {
    CURL* curl = curl_easy_init();
    std::string response_string;
    long http_code = 0;
    using Clock = std::chrono::high_resolution_clock;
    auto start = Clock::now();

    if (curl) {
        struct curl_slist* headers = nullptr;
        if (!auth_header.empty()) {
            headers = curl_slist_append(headers, auth_header.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        if (headers) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            curl_easy_cleanup(curl);
            return {
                {"error", "CURL request failed"},
                {"message", curl_easy_strerror(res)}
            };
        }

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start);
    return handle_response(response_string, http_code, latency);
}

std::string construct_url(const std::string& base, const std::vector<std::pair<std::string, std::string>>& params) {
    std::ostringstream oss;
    oss << base << "?";
    for (size_t i = 0; i < params.size(); ++i) {
        oss << params[i].first << "=" << curl_easy_escape(nullptr, params[i].second.c_str(), 0);
        if (i + 1 < params.size()) oss << "&";
    }
    return oss.str();
}

json place_market_buy(const std::string& token, const std::string& instrument, double amount) {
    std::string url = construct_url("https://test.deribit.com/api/v2/private/buy", {
        {"instrument_name", instrument},
        {"amount", std::to_string(amount)},
        {"type", "market"},
        {"label", "market_order_demo"}
    });
    return curl_get_request(url, "Authorization: Bearer " + token);
}

json place_limit_buy(const std::string& token, const std::string& instrument, double amount, double price) {
    std::string url = construct_url("https://test.deribit.com/api/v2/private/buy", {
        {"instrument_name", instrument},
        {"amount", std::to_string(amount)},
        {"price", std::to_string(price)},
        {"type", "limit"},
        {"post_only", "true"},
        {"label", "limit_order_demo"}
    });
    return curl_get_request(url, "Authorization: Bearer " + token);
}

json cancel_order(const std::string& token, const std::string& order_id) {
    std::string url = construct_url("https://test.deribit.com/api/v2/private/cancel", {
        {"order_id", order_id}
    });
    return curl_get_request(url, "Authorization: Bearer " + token);
}

json get_orderbook(const std::string& instrument_name, int depth) {
    std::string url = construct_url("https://test.deribit.com/api/v2/public/get_order_book", {
        {"instrument_name", instrument_name},
        {"depth", std::to_string(depth)}
    });
    return curl_get_request(url);
}

json get_open_orders(const std::string& token, const std::string& instrument_name) {
    std::string url = construct_url("https://test.deribit.com/api/v2/private/get_open_orders_by_instrument", {
        {"instrument_name", instrument_name}
    });
    return curl_get_request(url, "Authorization: Bearer " + token);
}

json get_positions(const std::string& token, const std::string& currency) {
    std::string url = construct_url("https://test.deribit.com/api/v2/private/get_positions", {
        {"currency", currency},
        {"kind", "any"}
    });
    return curl_get_request(url, "Authorization: Bearer " + token);
}

json modify_order(const std::string& token, const std::string& order_id, double amount, double price) {
    std::string url = construct_url("https://test.deribit.com/api/v2/private/edit", {
        {"order_id", order_id},
        {"amount", std::to_string(amount)},
        {"price", std::to_string(price)}
    });
    return curl_get_request(url, "Authorization: Bearer " + token);
}
