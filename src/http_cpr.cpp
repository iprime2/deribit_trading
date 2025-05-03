#include "trading.h"
#include <cpr/cpr.h>
#include <chrono>
#include <iostream>



nlohmann::json handle_response(const cpr::Response& response, const std::chrono::milliseconds& latency) {
    try {
        auto json = nlohmann::json::parse(response.text);
        // Extract usIn/usOut if available from parsed json
        if (json.contains("usIn") && json.contains("usOut")) {
            long long usIn = json["usIn"].get<long long>();
            long long usOut = json["usOut"].get<long long>();
            long long latency_us = usOut - usIn;

            json["meta"] = {
                {"latency_us", latency_us},
                {"latency_ms", latency_us / 1000.0},
                {"latency_ms_app", latency.count()}
            };
        }
        json["http_status"] = response.status_code;
        json["latency_ms"] = latency.count();
        return json;
    } catch (const std::exception& e) {
        return {
            {"error", "JSON parse error"},
            {"message", e.what()},
            {"raw_response", response.text},
            {"http_status", response.status_code},
            {"latency_ms", latency.count()}
        };
    }
}

nlohmann::json place_market_buy(const std::string& token, const std::string& instrument, double amount) {
    using Clock = std::chrono::high_resolution_clock;
    auto start = Clock::now();
    cpr::Response response = cpr::Get(
        cpr::Url{ "https://test.deribit.com/api/v2/private/buy" },
        cpr::Parameters{
            {"instrument_name", instrument},
            {"amount", std::to_string(static_cast<float>(amount))},
            {"type", "market"},
            {"label", "market_order_demo"}
        },
        cpr::Header{{"Authorization", "Bearer " + token}}
    );
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start);
    return handle_response(response, latency);
}

nlohmann::json place_limit_buy(const std::string& token, const std::string& instrument, double amount, double price) {
    using Clock = std::chrono::high_resolution_clock;
    auto start = Clock::now();
    cpr::Response response = cpr::Get(
        cpr::Url{ "https://test.deribit.com/api/v2/private/buy" },
        cpr::Parameters{
            {"instrument_name", instrument},
            {"amount", std::to_string(amount)},
            {"price", std::to_string(price)},
            {"type", "limit"},
            {"post_only", "true"},
            {"label", "limit_order_demo"}
        },
        cpr::Header{{"Authorization", "Bearer " + token}}
    );
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start);
    return handle_response(response, latency);
}

nlohmann::json cancel_order(const std::string& token, const std::string& order_id) {
    using Clock = std::chrono::high_resolution_clock;
    auto start = Clock::now();
    cpr::Response response = cpr::Get(
        cpr::Url{ "https://test.deribit.com/api/v2/private/cancel" },
        cpr::Parameters{{"order_id", order_id}},
        cpr::Header{{"Authorization", "Bearer " + token}}
    );
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start);
    return handle_response(response, latency);
}

nlohmann::json get_orderbook(const std::string& instrument_name, int depth) {
    using Clock = std::chrono::high_resolution_clock;
    auto start = Clock::now();
    cpr::Response response = cpr::Get(
        cpr::Url{ "https://test.deribit.com/api/v2/public/get_order_book" },
        cpr::Parameters{
            {"instrument_name", instrument_name},
            {"depth", std::to_string(depth)}
        }
    );
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start);
    return handle_response(response, latency);
}

nlohmann::json get_open_orders(const std::string& token, const std::string& instrument_name) {
    using Clock = std::chrono::high_resolution_clock;
    auto start = Clock::now();
    cpr::Response response = cpr::Get(
        cpr::Url{ "https://test.deribit.com/api/v2/private/get_open_orders_by_instrument" },
        cpr::Parameters{{"instrument_name", instrument_name}},
        cpr::Header{{"Authorization", "Bearer " + token}}
    );
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start);
    return handle_response(response, latency);
}

nlohmann::json get_positions(const std::string& token, const std::string& currency) {
    using Clock = std::chrono::high_resolution_clock;
    auto start = Clock::now();

    cpr::Response response = cpr::Get(
        cpr::Url{ "https://test.deribit.com/api/v2/private/get_positions" },
        cpr::Parameters{
            {"currency", currency},
            {"kind", "any"}
        },
        cpr::Header{{"Authorization", "Bearer " + token}}
    );

    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start);
    return handle_response(response, latency);
}

nlohmann::json modify_order(const std::string& token, const std::string& order_id, double amount, double price) {
    using Clock = std::chrono::high_resolution_clock;
    auto start = Clock::now();

    cpr::Response response = cpr::Get(
        cpr::Url{ "https://test.deribit.com/api/v2/private/edit" },
        cpr::Parameters{
            {"order_id", order_id},
            {"amount", std::to_string(amount)},
            {"price", std::to_string(price)}
        },
        cpr::Header{{"Authorization", "Bearer " + token}}
    );

    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start);

    try {
        auto json = nlohmann::json::parse(response.text);
        json["http_status"] = response.status_code;
        json["latency_ms"] = latency.count();
        return json;
    } catch (const std::exception& e) {
        return {
            {"error", "JSON parse error"},
            {"message", e.what()},
            {"raw_response", response.text},
            {"http_status", response.status_code},
            {"latency_ms", latency.count()}
        };
    }
}
