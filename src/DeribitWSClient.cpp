#include "DeribitWSClient.hpp"
#include "utils.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <iostream>
#include <functional>

using json = nlohmann::json;
using ws_client = websocketpp::client<websocketpp::config::asio_tls_client>;

DeribitWSClient::DeribitWSClient(const std::string& symbol) : symbol(symbol) {
    client.init_asio();

    client.set_tls_init_handler([](websocketpp::connection_hdl) {
        auto ctx = websocketpp::lib::make_shared<websocketpp::lib::asio::ssl::context>(
            websocketpp::lib::asio::ssl::context::tlsv12_client
        );
        try {
            ctx->set_verify_mode(websocketpp::lib::asio::ssl::context::verify_none);
        } catch (const std::exception& e) {
            log_to_file(std::string("[ERROR] TLS context setup failed: ") + e.what());
        }
        return ctx;
    });

    client.set_open_handler(std::bind(&DeribitWSClient::on_open, this, std::placeholders::_1));
    client.set_message_handler(std::bind(&DeribitWSClient::on_message, this, std::placeholders::_1, std::placeholders::_2));

    client.set_fail_handler([](websocketpp::connection_hdl) {
        log_to_file("[ERROR] WebSocket connection failed.");
    });

    client.set_close_handler([](websocketpp::connection_hdl) {
        log_to_file("[INFO] WebSocket connection closed.");
    });
}

void DeribitWSClient::on_open(websocketpp::connection_hdl h) {
    hdl = h;
    connected = true;
    log_to_file("[INFO] Connected to Deribit WebSocket");

    std::string channel = "book." + symbol + ".100ms";

    json subscribe_msg = {
        {"jsonrpc", "2.0"},
        {"method", "public/subscribe"},
        {"params", {{"channels", {channel}}}},
        {"id", 1}
    };

    client.send(hdl, subscribe_msg.dump(), websocketpp::frame::opcode::text);
}

void DeribitWSClient::on_message(websocketpp::connection_hdl, ws_client::message_ptr msg) {
    const std::string& payload = msg->get_payload();
    try {
        json parsed = json::parse(payload);

        if (parsed.contains("params") && parsed["params"].contains("data")) {
            auto& data = parsed["params"]["data"];
            if (data.contains("timestamp")) {
                long long sent_ms = data["timestamp"].get<long long>();
                long long sent_us = sent_ms * 1000;

                auto received_time = std::chrono::system_clock::now();
                long long now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    received_time.time_since_epoch()).count();

                long long latency_us = now_us - sent_us;
                log_ws_event("RECEIVED", symbol, payload, latency_us);
            } else {
                log_ws_event("RECEIVED", symbol, payload, -1);
            }
        }

    } catch (const std::exception& e) {
        log_to_file(std::string("[ERROR] JSON parse error: ") + e.what());
    }
}

void DeribitWSClient::run() {
    websocketpp::lib::error_code ec;
    auto conn = client.get_connection("wss://test.deribit.com/ws/api/v2/", ec);
    if (ec) {
        log_to_file("[ERROR] WebSocket connection failed: " + ec.message());
        return;
    }

    client.connect(conn);
    client.run();
}
