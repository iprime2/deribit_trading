#include "trading.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <iostream>
#include <sstream>
#include <curl/curl.h>

#include "CurlHandler.h"
#include "ThreadPool.h"

static ThreadPool pool(4); 

using json = nlohmann::json;

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
    return pool.enqueue([url, auth_header]() {
        static thread_local CurlHandler handler;

        long http_code = 0;
        auto start = std::chrono::high_resolution_clock::now();
        std::string res = handler.performGet(url, auth_header, &http_code);
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start
        );
        return handle_response(res, http_code, latency);
    }).get();
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
