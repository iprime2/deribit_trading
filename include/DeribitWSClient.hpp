#pragma once

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <string>
#include <unordered_map>
#include <mutex>

class DeribitWSClient {
public:
    explicit DeribitWSClient(const std::string& symbol);
    void run();

private:
    void on_open(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg);

    websocketpp::client<websocketpp::config::asio_tls_client> client;
    websocketpp::connection_hdl hdl;
    std::string symbol;
    bool connected = false;
    long long start_time_us = -1;
    std::unordered_map<std::string, long long> channel_send_time_us;
    std::mutex latency_mutex;
};
