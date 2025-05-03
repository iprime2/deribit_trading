#pragma once

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

using json = nlohmann::json;
typedef websocketpp::client<websocketpp::config::asio_tls_client> ws_client;

class DeribitPrivateWSClient {
public:
    DeribitPrivateWSClient(const std::string& id, const std::string& secret);
    ~DeribitPrivateWSClient();

    void connect();

    bool buyOrder(const std::string& symbol, double price, double amount, const std::string& type);
    bool getOpenOrdersByInstrument(const std::string& instrument);
    bool cancelOrder(const std::string& order_id);
    bool editOrder(const std::string& order_id, double price, double amount);
    bool getPositions(const std::string& currency = "BTC");
    bool getOrderBook(const std::string& instrument_name, int depth = 1);

    std::mutex& getResponseMutex();
    std::condition_variable& getResponseCV();

    void clearWSResponse();
    std::string getLastWSResponse() const;
    bool hasWSResponse() const;

private:
    void on_open(websocketpp::connection_hdl h);
    void on_message(websocketpp::connection_hdl, ws_client::message_ptr msg);
    void on_fail(websocketpp::connection_hdl);
    void on_close(websocketpp::connection_hdl);

    bool isAuthenticated() const;
    bool sendJSON(const json& payload);
    void setWebSocketResponse(const json& response);

    ws_client client;
    websocketpp::connection_hdl hdl;
    std::thread client_thread;

    std::string client_id;
    std::string client_secret;
    std::string access_token;
    bool authenticated = false;
    bool connected = false;

    mutable std::mutex response_mutex;
    std::condition_variable response_cv;

    std::string last_ws_response;
    bool ws_response_received = false;
};
