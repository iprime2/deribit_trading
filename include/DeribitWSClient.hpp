#pragma once

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <string>

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
};
