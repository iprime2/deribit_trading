#include "DeribitPrivateWSClient.hpp"
#include "utils.h"

#include <unordered_map>

constexpr const char* DERIBIT_WS_URL = "wss://test.deribit.com/ws/api/v2";


std::unordered_map<int, std::chrono::high_resolution_clock::time_point> sent_times;
std::mutex sent_times_mutex;

DeribitPrivateWSClient::DeribitPrivateWSClient(const std::string& id, const std::string& secret)
    : client_id(id), client_secret(secret) {
    
    client.init_asio();

    client.set_tls_init_handler([](websocketpp::connection_hdl) {
        auto ctx = websocketpp::lib::make_shared<websocketpp::lib::asio::ssl::context>(
            websocketpp::lib::asio::ssl::context::tlsv12_client);
        ctx->set_verify_mode(websocketpp::lib::asio::ssl::context::verify_none);
        return ctx;
    });

    client.set_open_handler(std::bind(&DeribitPrivateWSClient::on_open, this, std::placeholders::_1));
    client.set_message_handler(std::bind(&DeribitPrivateWSClient::on_message, this, std::placeholders::_1, std::placeholders::_2));
    client.set_fail_handler(std::bind(&DeribitPrivateWSClient::on_fail, this, std::placeholders::_1));
    client.set_close_handler(std::bind(&DeribitPrivateWSClient::on_close, this, std::placeholders::_1));
}

void DeribitPrivateWSClient::connect() {
    websocketpp::lib::error_code ec;
    auto con = client.get_connection(DERIBIT_WS_URL, ec);
    if (ec) {
        log_to_file("[ERROR] WebSocket connection error: " + ec.message());
        return;
    }

    client.connect(con);
    client_thread = std::thread([this]() { client.run(); });
}

void DeribitPrivateWSClient::on_open(websocketpp::connection_hdl h) {
    hdl = h;
    connected = true;

    log_to_file("[INFO] WebSocket connected. Sending authentication...");

    json auth = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "public/auth"},
        {"params", {
            {"grant_type", "client_credentials"},
            {"client_id", client_id},
            {"client_secret", client_secret}
        }}
    };

    sendJSON(auth);
}

void DeribitPrivateWSClient::on_message(websocketpp::connection_hdl, ws_client::message_ptr msg) {
    const std::string& payload = msg->get_payload();

    try {
        json response = json::parse(payload);

        int id = response.value("id", -1);

        if (id != -1) {
            auto end = std::chrono::high_resolution_clock::now();
        
            std::lock_guard<std::mutex> lock(sent_times_mutex);
            auto it = sent_times.find(id);
            if (it != sent_times.end()) {
                auto start = it->second;
                auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                auto latency_ms = latency_us / 1000.0;
        
                response["meta"]["latency_app_us"] = latency_us;
                response["meta"]["latency_app_ms"] = latency_ms;
        
                sent_times.erase(it);  // Clean up
            }
        }        

        if (response.contains("id") && response["id"] == 1 && response.contains("result")) {
            access_token = response["result"]["access_token"];
            authenticated = true;
            log_to_file("[INFO] Authenticated successfully");
            return;
        }

        // long latency_us = -1;
        // if (response.contains("usIn") && response.contains("usOut")) {
        //     latency_us = response["usOut"].get<long long>() - response["usIn"].get<long long>();
        //     response["meta"]["latency_us"] = latency_us;
        //     response["meta"]["latency_ms"] = latency_us / 1000.0;
        // }

        log_ws_event("RECEIVED", "", payload, response["meta"]["latency_app_us"] );
        setWebSocketResponse(response);

    } catch (const std::exception& e) {
        log_to_file(std::string("[ERROR] JSON parse error: ") + e.what());
    }
}

void DeribitPrivateWSClient::on_fail(websocketpp::connection_hdl) {
    log_to_file("[ERROR] WebSocket connection failed.");
}

void DeribitPrivateWSClient::on_close(websocketpp::connection_hdl) {
    log_to_file("[INFO] WebSocket connection closed.");
}

bool DeribitPrivateWSClient::isAuthenticated() const {
    return authenticated && !access_token.empty();
}

bool DeribitPrivateWSClient::buyOrder(const std::string& symbol, double price, double amount, const std::string& type) {
    if (!isAuthenticated()) return false;

    json payload = {
        {"jsonrpc", "2.0"},
        {"id", 2},
        {"method", "private/buy"},
        {"params", {
            {"instrument_name", symbol},
            {"amount", amount},
            {"type", type},
            {"access_token", access_token}
        }}
    };
    if (type == "limit") payload["params"]["price"] = price;
    return sendJSON(payload);
}

bool DeribitPrivateWSClient::getOpenOrdersByInstrument(const std::string& instrument) {
    if (!isAuthenticated()) return false;

    json payload = {
        {"jsonrpc", "2.0"},
        {"id", 11},
        {"method", "private/get_open_orders_by_instrument"},
        {"params", {
            {"instrument_name", instrument},
            {"access_token", access_token}
        }}
    };
    return sendJSON(payload);
}

bool DeribitPrivateWSClient::cancelOrder(const std::string& order_id) {
    if (!isAuthenticated()) return false;

    json payload = {
        {"jsonrpc", "2.0"},
        {"id", 3},
        {"method", "private/cancel"},
        {"params", {
            {"order_id", order_id},
            {"access_token", access_token}
        }}
    };
    return sendJSON(payload);
}

bool DeribitPrivateWSClient::editOrder(const std::string& order_id, double price, double amount) {
    if (!isAuthenticated()) return false;

    json payload = {
        {"jsonrpc", "2.0"},
        {"id", 12},
        {"method", "private/edit"},
        {"params", {
            {"order_id", order_id},
            {"price", price},
            {"amount", amount},
            {"access_token", access_token}
        }}
    };
    return sendJSON(payload);
}

bool DeribitPrivateWSClient::getPositions(const std::string& currency) {
    if (!isAuthenticated()) return false;

    json payload = {
        {"jsonrpc", "2.0"},
        {"id", 13},
        {"method", "private/get_positions"},
        {"params", {
            {"currency", currency},
            {"access_token", access_token}
        }}
    };
    return sendJSON(payload);
}

bool DeribitPrivateWSClient::getOrderBook(const std::string& instrument_name, int depth) {
    json payload = {
        {"jsonrpc", "2.0"},
        {"id", 14},
        {"method", "public/get_order_book"},
        {"params", {
            {"instrument_name", instrument_name},
            {"depth", depth}
        }}
    };
    return sendJSON(payload);
}

bool DeribitPrivateWSClient::sendJSON(const json& payload) {
    try {
        if (payload.contains("id")) {
            int id = payload["id"].get<int>();
            std::lock_guard<std::mutex> lock(sent_times_mutex);
            sent_times[id] = std::chrono::high_resolution_clock::now();
        }
        auto locked_hdl = hdl.lock();
        if (connected && locked_hdl) {
            client.send(locked_hdl, payload.dump(), websocketpp::frame::opcode::text);
            return true;
        } else {
            log_to_file("[ERROR] WebSocket is not connected or handle expired.");
        }
    } catch (const std::exception& e) {
        log_to_file(std::string("[ERROR] Failed to send message: ") + e.what());
    }
    return false;
}

void DeribitPrivateWSClient::setWebSocketResponse(const json& response) {
    std::lock_guard<std::mutex> lock(response_mutex);
    last_ws_response = response.dump();
    ws_response_received = true;
    response_cv.notify_all();
}

void DeribitPrivateWSClient::clearWSResponse() {
    std::lock_guard<std::mutex> lock(response_mutex);
    last_ws_response.clear();
    ws_response_received = false;
}

std::mutex& DeribitPrivateWSClient::getResponseMutex() { return response_mutex; }
std::condition_variable& DeribitPrivateWSClient::getResponseCV() { return response_cv; }
std::string DeribitPrivateWSClient::getLastWSResponse() const { return last_ws_response; }
bool DeribitPrivateWSClient::hasWSResponse() const { return ws_response_received; }

DeribitPrivateWSClient::~DeribitPrivateWSClient() {
    if (client_thread.joinable()) {
        client.stop();
        client_thread.join();
    }
}
