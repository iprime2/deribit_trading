#pragma once

#include <uwebsockets/App.h>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <functional>
#include <mutex>
#include <thread>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

class WebSocketServer {
public:
    WebSocketServer(int port);
    void start();
    void broadcast(const std::string& symbol, const std::string& message);
    uWS::App app;

private:
    struct ClientData {
        std::unordered_set<std::string> subscriptions;
    };

    using ws_client_t = websocketpp::client<websocketpp::config::asio_tls_client>;

    int port;
    ws_client_t deribit_client; 
    websocketpp::connection_hdl deribit_hdl;
    std::thread deribit_thread;
    bool deribit_connected;
    std::mutex deribit_mutex;

    void subscribeToDeribitSymbol(const std::string& symbol);
};
