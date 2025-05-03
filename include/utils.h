#pragma once
#include <string>
#include <crow.h> 

void log_to_file(const std::string& message);
std::string current_timestamp();

std::string extract_bearer_token(const crow::request& req);

void log_ws_event(const std::string& event_type, const std::string& symbol, const std::string& message, long latency_ms = -1);