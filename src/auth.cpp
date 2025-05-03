#include "auth.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <iostream>

std::string get_access_token(const std::string& client_id, const std::string& client_secret) {
    auto response = cpr::Get(
        cpr::Url{ "https://test.deribit.com/api/v2/public/auth" },
        cpr::Parameters{
            {"grant_type", "client_credentials"},
            {"client_id", client_id},
            {"client_secret", client_secret}
        }
    );

    std::cout << "[AUTH API] Deribit URL → /api/v2/public/auth\n";
    std::cout << "[AUTH API] HTTP Code → " << response.status_code << "\n";
    std::cout << "[AUTH API] Response   → " << response.text << "\n";

    if (response.status_code == 200) {
        auto json = nlohmann::json::parse(response.text);
        return json["result"]["access_token"];
    } else {
        std::cerr << "[AUTH ERROR] Failed: " << response.text << std::endl;
        return "";
    }
}