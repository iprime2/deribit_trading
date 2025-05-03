// WebSocketServer.cpp
#include "WebSocketServer.hpp"
#include "utils.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <mutex>

WebSocketServer::WebSocketServer(int port) : port(port), deribit_connected(false) {}

void WebSocketServer::start() {
    app.ws<ClientData>("/ws", {
        .open = [](auto* ws) {
            log_to_file("[DEBUG] Client connected via WebSocket");
        },

        .message = [this](auto* ws, std::string_view message, uWS::OpCode opCode) {
            std::string msg(message);
            if (msg.rfind("subscribe:", 0) == 0) {
                std::string symbol = msg.substr(10);
                ws->subscribe(symbol);
                ws->getUserData()->subscriptions.insert(symbol);
                log_to_file("[DEBUG] Client subscribed to: " + symbol);
                this->subscribeToDeribitSymbol(symbol);
            } else if (msg.rfind("unsubscribe:", 0) == 0) {
                std::string symbol = msg.substr(12);
                ws->unsubscribe(symbol);
                ws->getUserData()->subscriptions.erase(symbol);
                log_to_file("[DEBUG] Client unsubscribed from: " + symbol);
            }
        },

        .close = [](auto* ws, int /*code*/, std::string_view /*message*/) {
            log_to_file("[DEBUG] Client disconnected");
        }
    }).listen(port, [this](auto* token) {
        if (token) {
            log_to_file("[INFO] WebSocket server listening on port " + std::to_string(port));
        } else {
            log_to_file("[ERROR] Failed to listen on port " + std::to_string(port));
        }
    }).run();
}

void WebSocketServer::broadcast(const std::string& symbol, const std::string& message) {
    app.publish(symbol, message, uWS::OpCode::TEXT);
}

void WebSocketServer::subscribeToDeribitSymbol(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(deribit_mutex);
    if (!deribit_connected) {
        deribit_client.init_asio();

        deribit_client.set_message_handler([this](websocketpp::connection_hdl, ws_client_t::message_ptr msg) {
            std::string payload = msg->get_payload();
            try {
                auto json_msg = nlohmann::json::parse(payload);
                if (json_msg.contains("params") && json_msg["params"].contains("channel")) {
                    std::string channel = json_msg["params"]["channel"];
                    std::string data = json_msg.dump();
                    broadcast(channel, data);
                }
            } catch (const std::exception& e) {
                log_to_file(std::string("[ERROR] Failed to parse Deribit WebSocket message: ") + e.what());
            }
        });

        deribit_client.set_open_handler([this](websocketpp::connection_hdl hdl) {
            std::lock_guard<std::mutex> lock(deribit_mutex);
            deribit_hdl = hdl;
            deribit_connected = true;
            log_to_file("[INFO] Connected to Deribit WebSocket");
        });

        websocketpp::lib::error_code ec;
        auto con = deribit_client.get_connection("wss://test.deribit.com/ws/api/v2", ec);
        if (ec) {
            log_to_file("[ERROR] Connection error: " + ec.message());
            return;
        }
        deribit_client.connect(con);
        deribit_thread = std::thread([this]() { deribit_client.run(); });
        deribit_thread.detach();
    }

    if (!deribit_hdl.lock()) {
        log_to_file("[ERROR] Deribit WebSocket handle not available yet.");
        return;
    }

    nlohmann::json sub_msg = {
        {"jsonrpc", "2.0"},
        {"id", 42},
        {"method", "public/subscribe"},
        {"params", {"channels", {"book." + symbol + ".none.1.100ms"}}}
    };

    try {
        deribit_client.send(deribit_hdl, sub_msg.dump(), websocketpp::frame::opcode::text);
    } catch (const std::exception& e) {
        log_to_file(std::string("[ERROR] Failed to send subscription to Deribit: ") + e.what());
    }
}
