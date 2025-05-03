#include "utils.h"
#include <fstream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <crow.h> 

namespace fs = std::filesystem;

std::string current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void log_to_file(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm* local_time = std::localtime(&t);

    std::stringstream path_stream;
    path_stream << "logs/"
                << (local_time->tm_year + 1900) << "/"
                << std::setw(2) << std::setfill('0') << (local_time->tm_mon + 1);

    std::string dir_path = path_stream.str();

    // Create directories if they don't exist
    fs::create_directories(dir_path);

    std::stringstream file_name_stream;
    file_name_stream << dir_path << "/"
                     << std::setw(2) << std::setfill('0') << local_time->tm_mday << ".log";

    std::ofstream file(file_name_stream.str(), std::ios::app);
    if (file) {
        file << "[" << current_timestamp() << "] " << message << "\n";
    }
}

std::string extract_bearer_token(const crow::request& req) {
    auto auth_header = req.get_header_value("Authorization");
    const std::string prefix = "Bearer ";
    if (auth_header.rfind(prefix, 0) == 0) {
        return auth_header.substr(prefix.length());
    }
    return "";
}

void log_ws_event(const std::string& event_type, const std::string& response, const std::string& message, long latency_us) {
    std::ostringstream ss;
    ss << "[WS EVENT] " << event_type
       << " | Latency: " << (latency_us >= 0 ? std::to_string(latency_us) + " µs" : "N/A")
       << " | Latency: " << (latency_us >= 0 ? std::to_string(latency_us / 1000.0) + " ms" : "N/A")
       << " | Payload: " << message
       << " | Response: " << response;
    log_to_file(ss.str());
}