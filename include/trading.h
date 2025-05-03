#pragma once
#include <string>
#include <nlohmann/json.hpp>

nlohmann::json place_market_buy(const std::string& token, const std::string& instrument, double amount);
nlohmann::json place_limit_buy(const std::string& token, const std::string& instrument, double amount, double price);
nlohmann::json cancel_order(const std::string& token, const std::string& order_id);
nlohmann::json get_orderbook(const std::string& instrument_name, int depth = 1);
nlohmann::json get_open_orders(const std::string& token, const std::string& instrument_name);
nlohmann::json get_positions(const std::string& token, const std::string& currency);
nlohmann::json modify_order(const std::string& token, const std::string& order_id, double amount, double price);

