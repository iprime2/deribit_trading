// main.cpp
#include "DeribitPrivateWSClient.hpp"
#include "auth.h"
#include "trading.h"
#include "utils.h"
#include <crow.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <thread>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <mutex>
// #include "WebSocketServer.hpp"
#include "DeribitWSClient.hpp"


using json = nlohmann::json;

std::unordered_map<std::string, std::string> token_map;
std::mutex token_mutex;

#define LOG_API(label, app_route, deribit_endpoint, code, latency, response, type)                   \
    do {                                                                                             \
        std::ostringstream oss;                                                                      \
        oss << "[App Route]     → " << app_route << "\n"                                             \
            << "[" << label << "] Deribit URL → " << deribit_endpoint << "\n"                        \
            << "[" << label << "] Connection Type → " << type << "\n"                                \
            << "[" << label << "] HTTP Code    → " << code << "\n"                                   \
            << "[" << label << "] Latency      → " << latency << " ms\n"                             \
            << "[" << label << "] Response     → " << "null" << "\n";                              \
        std::string log_entry = oss.str();                                                           \
        std::cout << log_entry;                                                                      \
        log_to_file(log_entry);                                                                      \
    } while(0)


int http_apis() {
    // std::string client_id = "GSdaPwOW";
    // std::string client_secret = "VxqDlLsYY6brBfh6Kpu5NiyUQI0pDMU-YKZz0gFhxZo";

    std::string client_id, client_secret;

    std::cout << "Enter your Deribit Client ID: ";
    std::cin >> client_id;
    // std::getline(std::cin, client_id);

    std::cout << "Enter your Deribit Client Secret: ";
    std::cin >> client_secret;
    // std::getline(std::cin, client_secret);

    if (client_id.empty() || client_secret.empty()) {
        std::cerr << "[ERROR] Client ID and Secret cannot be empty!" << std::endl;
        return 1;
    }

    // Start WebSocket server on port 9001
    // WebSocketServer wsServer(9101);
    // std::thread wsThread([&wsServer]() {
    //     wsServer.start();
    // });
    

    auto privateClient = std::make_shared<DeribitPrivateWSClient>(client_id, client_secret);
    privateClient->connect();

    crow::SimpleApp app;

    CROW_ROUTE(app, "/api/ws/buy").methods("POST"_method)
    ([privateClient](const crow::request& req) {
        std::string app_route = req.url;
        log_to_file("[API CALL] POST /api/ws/buy");
    
        auto body = crow::json::load(req.body);
        if (!body) {
            log_to_file("[ERROR] Invalid JSON in buy");
            return crow::response(400, "Invalid JSON payload");
        }
    
        std::string symbol = body["symbol"].s();
        double amount = body["amount"].d();
        std::string type = body["type"].s();
        double price = (type == "limit" && body["price"].t() != crow::json::type::Null) ? body["price"].d() : 0.0;
    
        if (symbol.empty() || amount <= 0 || type.empty() || (type == "limit" && price <= 0)) {
            log_to_file("[ERROR] Validation failed in buy");
            return crow::response(400, "Invalid input parameters");
        }
    
        privateClient->clearWSResponse();
    
        if (!privateClient->buyOrder(symbol, price, amount, type)) {
            log_to_file("[ERROR] Failed to send WebSocket buy request");
            return crow::response(500, "Failed to send WebSocket buy request");
        }
    
        std::unique_lock<std::mutex> lock(privateClient->getResponseMutex());
        if (!privateClient->getResponseCV().wait_for(lock, std::chrono::seconds(3), [&] {
            return privateClient->hasWSResponse();
        })) {
            log_to_file("[ERROR] Timeout waiting for WebSocket buy response");
            return crow::response(504, "Timeout waiting for response from Deribit");
        }
    
        std::string resp = privateClient->getLastWSResponse();
        json resp_json = json::parse(resp);
        double latency = resp_json.contains("meta") ? resp_json["meta"].value("latency_ms", -1.0) : -1.0;
    
        LOG_API("BUY", app_route, "/api/v2/private/buy", 200, latency, resp, "WS");
        return crow::response(200, resp);
    });    

    // Open Orders
    CROW_ROUTE(app, "/api/ws/open_orders").methods("GET"_method)
    ([privateClient](const crow::request& req) {
        std::string app_route = req.url;
        log_to_file("[API CALL] GET /api/ws/open_orders: " + app_route);

        std::string instrument = req.url_params.get("instrument") ? req.url_params.get("instrument") : "BTC-PERPETUAL";

        privateClient->clearWSResponse();

        if (!privateClient->getOpenOrdersByInstrument(instrument)) {
            log_to_file("[ERROR] Failed to send WebSocket open_orders request");
            return crow::response(500, "Failed to send WebSocket request to Deribit");
        }

        std::unique_lock<std::mutex> lock(privateClient->getResponseMutex());
        if (!privateClient->getResponseCV().wait_for(lock, std::chrono::seconds(5), [&] {
            return privateClient->hasWSResponse();
        })) {
            log_to_file("[ERROR] Timeout waiting for open orders WebSocket response");
            LOG_API("POSITIONS", app_route, "/api/v2/private/get_open_orders_by_instrument", 504, "NA", "Timeout", "WS");
            return crow::response(504, "Timeout waiting for open orders response from Deribit");
        }

        std::string resp = privateClient->getLastWSResponse();
        json resp_json = json::parse(resp);
        double latency = resp_json.contains("meta") ? resp_json["meta"].value("latency_ms", -1.0) : -1.0;

        log_to_file("[INFO] Open orders WebSocket response:\n" + resp);
        LOG_API("POSITIONS", app_route, "/api/v2/private/get_open_orders_by_instrument", 200, latency, resp, "WS");
        return crow::response(200, resp);
    });

    // Cancel Orders
    CROW_ROUTE(app, "/api/ws/cancel_order").methods("DELETE"_method)
    ([privateClient](const crow::request& req) {
        std::string app_route = req.url;
        log_to_file("[API CALL] DELETE /api/ws/cancel_order: " + app_route);
    
        const char* order_id_param = req.url_params.get("order_id");
        if (!order_id_param) {
            log_to_file("[ERROR] Missing 'order_id' query parameter in cancel_order");
            return crow::response(400, "Missing 'order_id' query parameter");
        }
    
        std::string order_id = order_id_param;
    
        privateClient->clearWSResponse();
    
        if (!privateClient->cancelOrder(order_id)) {
            log_to_file("[ERROR] Failed to send cancel order request for ID: " + order_id);
            return crow::response(500, "Failed to send cancel order WebSocket request");
        }
    
        std::unique_lock<std::mutex> lock(privateClient->getResponseMutex());
        if (!privateClient->getResponseCV().wait_for(lock, std::chrono::seconds(5), [&] {
            return privateClient->hasWSResponse();
        })) {
            log_to_file("[ERROR] Timeout waiting for cancel order WebSocket response");
            return crow::response(504, "Timeout waiting for cancel order response from Deribit");
        }
    
        const std::string& resp = privateClient->getLastWSResponse();
        LOG_API("CANCEL", app_route, "/api/v2/private/cancel", 200, json::parse(resp)["meta"]["latency_ms"], resp, "WS");
        return crow::response(200, resp);
    });
    
    // Modify Order
    CROW_ROUTE(app, "/api/ws/modify_order").methods("PATCH"_method)
    ([privateClient](const crow::request& req) {
        std::string app_route = req.url;
        log_to_file("[API CALL] PATCH /api/ws/modify_order: " + app_route);
    
        auto body = crow::json::load(req.body);
        if (!body || !body["order_id"] || !body["price"] || !body["amount"]) {
            log_to_file("[ERROR] Invalid JSON input in modify_order");
            return crow::response(400, "Invalid input JSON: order_id, price, and amount required");
        }
    
        std::string order_id = body["order_id"].s();
        double price = body["price"].d();
        double amount = body["amount"].d();
    
        privateClient->clearWSResponse();
    
        if (!privateClient->editOrder(order_id, price, amount)) {
            log_to_file("[ERROR] Failed to send modify order WebSocket request");
            return crow::response(500, "Failed to send modify request");
        }
    
        std::unique_lock<std::mutex> lock(privateClient->getResponseMutex());
        if (!privateClient->getResponseCV().wait_for(lock, std::chrono::seconds(5), [&] {
            return privateClient->hasWSResponse();
        })) {
            log_to_file("[ERROR] Timeout waiting for modify order WebSocket response");
            return crow::response(504, "Timeout waiting for response from Deribit");
        }
    
        const std::string& resp = privateClient->getLastWSResponse();
        LOG_API("MODIFY", app_route, "/api/v2/private/edit", 200, json::parse(resp)["meta"]["latency_ms"], resp, "WS");
        return crow::response(200, resp);
    });
        
    // GET Position
    CROW_ROUTE(app, "/api/ws/positions").methods("GET"_method)
    ([privateClient](const crow::request& req) {
        std::string app_route = req.url;
        log_to_file("[API CALL] GET /api/ws/positions: " + app_route);

        std::string currency = req.url_params.get("currency") ? req.url_params.get("currency") : "BTC";

        privateClient->clearWSResponse();

        if (!privateClient->getPositions(currency)) {
            log_to_file("[ERROR] Failed to send get_positions request");
            return crow::response(500, "Failed to send WebSocket request to Deribit");
        }

        std::unique_lock<std::mutex> lock(privateClient->getResponseMutex());
        if (!privateClient->getResponseCV().wait_for(lock, std::chrono::seconds(5), [&] {
            return privateClient->hasWSResponse();
        })) {
            log_to_file("[ERROR] Timeout waiting for get_positions WebSocket response");
            LOG_API("POSITIONS", app_route, "/api/v2/private/get_positions", 504, "NA", "Timeout", "WS");
            return crow::response(504, "Timeout waiting for response from Deribit");
        }

        const std::string& resp = privateClient->getLastWSResponse();
        LOG_API("POSITIONS", app_route, "/api/v2/private/get_positions", 200, json::parse(resp)["meta"]["latency_ms"], resp, "WS");
        return crow::response(200, resp);
    });

    //GET Orderbook
    CROW_ROUTE(app, "/api/ws/orderbook").methods("GET"_method)
    ([privateClient](const crow::request& req) {
        std::string app_route = req.url;
        log_to_file("[API CALL] GET /api/ws/orderbook: " + app_route);

        std::string instrument = req.url_params.get("instrument") ? req.url_params.get("instrument") : "BTC-PERPETUAL";
        // stoi to concert string to int
        int depth = req.url_params.get("depth") ? std::stoi(req.url_params.get("depth")) : 1;

        privateClient->clearWSResponse();

        if (!privateClient->getOrderBook(instrument, depth)) {
            log_to_file("[ERROR] Failed to send get_order_book request");
            return crow::response(500, "Failed to send WebSocket request to Deribit");
        }

        std::unique_lock<std::mutex> lock(privateClient->getResponseMutex());
        if (!privateClient->getResponseCV().wait_for(lock, std::chrono::seconds(5), [&] {
            return privateClient->hasWSResponse();
        })) {
            log_to_file("[ERROR] Timeout waiting for order book WebSocket response");
            LOG_API("ORDERBOOK", app_route, "/api/v2/public/get_order_book", 504, "NA", "Timeout", "WS");
            return crow::response(504, "Timeout waiting for order book response from Deribit");
        }

        const std::string& resp = privateClient->getLastWSResponse();
        LOG_API("ORDERBOOK", app_route, "/api/v2/public/get_order_book", 200, json::parse(resp)["meta"]["latency_ms"], resp, "WS");
        return crow::response(200, resp);
    });



    // HTTPS APIs

    CROW_ROUTE(app, "/api/health")([]() {
        crow::json::wvalue health;
        health["status"] = "ok";
        health["service"] = "Crow Deribit Trading API";
        health["uptime"] = "running";  // In production, replace with actual uptime metrics
        return crow::response{health};
    });

    CROW_ROUTE(app, "/api/auth").methods("POST"_method)
    ([](const crow::request& req) {
        nlohmann::json res;
        std::string app_route = req.url;  
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string client_id = body["client_id"];
            std::string client_secret = body["client_secret"];

            std::string token = get_access_token(client_id, client_secret);

            if (!token.empty()) {
                std::lock_guard<std::mutex> lock(token_mutex);
                token_map[token] = client_id;

                res["access_token"] = token;
                res["http_status"] = 200;
                res["latency_ms"] = 0;
                LOG_API("AUTH", app_route, "/api/v2/public/auth", 200, 0, res.dump(4), "HTTPS");
                return crow::response{ res.dump(4) };
            } else {
                LOG_API("AUTH", app_route, "/api/v2/public/auth", 401, 0, R"({"error": "Authentication failed"})", "HTTPS");
                return crow::response(401, R"({"error": "Authentication failed"})");
            }
        } catch (const std::exception& e) {
            std::string err = std::string("{\"error\": \"") + e.what() + "\"}";
            LOG_API("AUTH", app_route, "/api/v2/public/auth", 400, 0, err, "HTTPS");
            return crow::response(400, err);
        }
    });

    CROW_ROUTE(app, "/api/buy").methods("POST"_method)
    ([](const crow::request& req) {
        std::string app_route = req.url;  
        std::string token = extract_bearer_token(req);

        if (token.empty()) {
            LOG_API("BUY", app_route, "/api/v2/private/buy", 401, 0, R"({"error": "Missing or invalid token"})", "HTTPS");
            return crow::response(401, R"({"error": "Missing or invalid token"})");
        }

        try {
            auto body = nlohmann::json::parse(req.body);
            
            std::string instrument = body["instrument"];
            double amount = body["amount"];
            std::string type = body["type"];

            std::cout<<body<<std::endl;
            std::cout<<type<<std::endl;
            
            nlohmann::json res;
            if (type == "market") {
                res = place_market_buy(token, instrument, amount);
            } else if (type == "limit") {
                if (!body.contains("price")) {
                    return crow::response(400, R"({"error": "Missing 'price' for limit order."})");
                }
                double price = body["price"];
                res = place_limit_buy(token, instrument, amount, price);
            } else {
                return crow::response(400, R"({"error": "Invalid type. Use 'market' or 'limit'."})");
            }

            LOG_API("BUY", app_route, "/api/v2/private/buy", res["http_status"], res["latency_ms"], res.dump(4), "HTTPS");
            return crow::response{ res.dump(4) };
        } catch (const std::exception& e) {
            std::string err = std::string("{\"error\": \"") + e.what() + "\"}";
            LOG_API("BUY", app_route, "/api/v2/private/buy", 400, 0, err, "HTTPS");
            return crow::response(400, err);
        }
    });

    CROW_ROUTE(app, "/api/cancel").methods("DELETE"_method)
    ([](const crow::request& req) {
        std::string app_route = req.url;  
        std::string token = extract_bearer_token(req);
        if (token.empty()) {
            LOG_API("CANCEL", app_route, "/api/v2/private/cancel", 401, 0, R"({"error": "Missing or invalid token"})", "HTTPS");
            return crow::response(401, R"({"error": "Missing or invalid token"})");
        }

        auto query = crow::query_string(req.url_params);
        if (!query.get("order_id")) {
            return crow::response(400, R"({"error": "Missing 'order_id' in query"})");
        }

        std::string order_id = query.get("order_id");
        auto res = cancel_order(token, order_id);
        LOG_API("CANCEL", app_route, "/api/v2/private/cancel", res["http_status"], res["latency_ms"], res.dump(4), "HTTPS");
        return crow::response{ res.dump(4) };
    });

    CROW_ROUTE(app, "/api/orderbook").methods("GET"_method)
    ([](const crow::request& req) {
        std::string app_route = req.url;  
        auto query = crow::query_string(req.url_params);
        if (!query.get("instrument")) {
            return crow::response(400, R"({"error": "Missing 'instrument' parameter"})");
        }
        std::string instrument = query.get("instrument");
        int depth = query.get("depth") ? std::stoi(query.get("depth")) : 2;

        std::cout<<"before req"<<std::endl;
        auto res = get_orderbook(instrument, depth);
        
        LOG_API("ORDERBOOK", app_route, "/api/v2/public/get_order_book", res["http_status"], res["latency_ms"], res.dump(4), "HTTPS");
        return crow::response{ res.dump(4) };
    });

    CROW_ROUTE(app, "/api/orders").methods("GET"_method)
    ([](const crow::request& req) {
        std::string app_route = req.url;  
        std::string token = extract_bearer_token(req);

        if (token.empty()) {
            LOG_API("ORDERS", app_route, "/api/v2/private/get_open_orders_by_instrument", 401, 0, R"({"error": "Missing or invalid token"})", "HTTPS");
            return crow::response(401, R"({"error": "Missing or invalid token"})");
        }

        auto query = crow::query_string(req.url_params);
        if (!query.get("instrument")) {
            return crow::response(400, R"({"error": "Missing 'instrument' parameter"})");
        }

        std::string instrument = query.get("instrument");
        auto res = get_open_orders(token, instrument);
        LOG_API("ORDERS", app_route, "/api/v2/private/get_open_orders_by_instrument", res["http_status"], res["latency_ms"], res.dump(4), "HTTPS");
        return crow::response{ res.dump(4) };
    });

    // GET API to get current positions
    CROW_ROUTE(app, "/api/positions").methods("GET"_method)
    ([](const crow::request& req) {
        std::string app_route = req.url;
        std::string token = extract_bearer_token(req);
        if (token.empty()) {
            LOG_API("POSITIONS", app_route, "/api/v2/private/get_positions", 401, 0, R"({"error": "Missing or invalid token"})", "HTTPS");
            return crow::response(401, R"({"error": "Missing or invalid token"})");
        }

        auto query = crow::query_string(req.url_params);
        if (!query.get("currency")) {
            return crow::response(400, R"({"error": "Missing 'currency' parameter"})");
        }

        std::string currency = query.get("currency");
        auto res = get_positions(token, currency);
        LOG_API("POSITIONS", "/positions", "/api/v2/private/get_positions", res["http_status"], res["latency_ms"], res.dump(4), "HTTPS");
        return crow::response{ res.dump(4) };
    });

    CROW_ROUTE(app, "/api/modify").methods("PUT"_method)
    ([&](const crow::request& req) {
        std::string app_route = req.url;
        std::string token = extract_bearer_token(req);
        if (token.empty()) {
            LOG_API("MODIFY", app_route, "/api/v2/private/edit", 401, 0, R"({"error": "Missing or invalid token"})", "HTTPS");
            return crow::response(401, R"({"error": "Missing or invalid token"})");
        }

        try {
            auto body = nlohmann::json::parse(req.body);
            if (!body.contains("order_id") || !body.contains("amount") || !body.contains("price")) {
                return crow::response(400, R"({"error": "Missing 'order_id', 'amount', or 'price'"})");
            }

            std::string order_id = body["order_id"];
            double amount = body["amount"];
            double price = body["price"];

            auto res = modify_order(token, order_id, amount, price);
            LOG_API("MODIFY", app_route, "/api/v2/private/edit", res["http_status"], res["latency_ms"], res.dump(4), "HTTPS");
            return crow::response{ res.dump(4) };
        } catch (const std::exception& e) {
            std::string err = std::string("{\"error\": \"") + e.what() + "\"}";
            LOG_API("MODIFY", app_route, "/api/v2/private/edit", 400, 0, err, "HTTPS");
            return crow::response(400, err);
        }
    });

    log_to_file("[INFO] Crow API server started on port 8080.");
    app.port(8080).multithreaded().run();

    return 0;
}

int main () {
    int choice;

    while (true) {
        std::cout << "\nMenu:\n";
        std::cout << "1. Subscribe\n";
        std::cout << "2. APIS\n";
        std::cout << "0. Exit\n";
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                try {
                    std::string symbol;
                    std::cout << "Enter Deribit symbol (e.g., BTC-PERPETUAL): ";
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // fix here
                    std::getline(std::cin, symbol);
            
                    if (symbol.empty()) {
                        std::cerr << "[ERROR] Symbol cannot be empty." << std::endl;
                        return 1;
                    }
            
                    DeribitWSClient client(symbol);
                    client.run();
                } catch (const std::exception& e) {
                    std::cerr << "[ERROR] Exception in main: " << e.what() << std::endl;
                }
                break;
            case 2:
                http_apis();
                break;
            case 0:
                std::cout << "Exiting..." << std::endl;
                return 0;
            default:
                std::cout << "Invalid choice. Try again." << std::endl;
        }
    }


    return 0;
}


