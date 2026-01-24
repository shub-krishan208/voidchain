#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    json j;
    j["message"] = "Random json check ..!";
    j["project"] = "Void Chain";
    j["status"] = "Works fine";
    
    std::cout<<j.dump(2)<<std::endl;
    return 0;
}