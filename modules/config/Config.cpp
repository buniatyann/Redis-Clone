#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include "Config.h"

namespace fs = std::filesystem;
config::Settings config::GlobalConfig;

bool config::load() {
    fs::path currentPath = fs::current_path();
    for (const auto& entry : fs::directory_iterator(currentPath)) {
        if (entry.path().filename() == CONFIG_FILE) {
            std::ifstream configFile(entry.path());
            if (configFile.is_open()) {
                nlohmann::json json = nlohmann::json::parse(configFile);
                if (json.find("snapshot_period") == json.end()) {
                    std::cout << "Unable to read the snapshot config!" << std::endl;
                    return false;
                } 
                else {
                    config::GlobalConfig.snapshotPeriod = json["snapshot_period"];
                }

                if (json.find("port") == json.end()) {
                    std::cout << "Unable to read the port config!" << std::endl;
                    return false;
                } 
                else {
                    config::GlobalConfig.port = json["port"];
                }

                return true;
            } 
            else {
                break;
            }
        }
    }

    return false;
}
