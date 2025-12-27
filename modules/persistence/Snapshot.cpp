#include <fstream>
#include <filesystem>
#include <thread>
#include <nlohmann/json.hpp>
#include "Snapshot.h"
#include "data/Store.h"

#define STATEFILE "state.json"

void to_json(nlohmann::json& j, const ValueEntry& v) {
    j = nlohmann::json{{"val", v.val}, {"expiry_epoch", v.expiryEpoch}};
}

void from_json(const nlohmann::json& j, ValueEntry& v) {
    j.at("val").get_to(v.val);
    j.at("expiry_epoch").get_to(v.expiryEpoch);
}

bool Snapshot::save() {
    try {
        nlohmann::json json;
        Store& store = Store::getInstance();
        {
            std::shared_lock<std::shared_mutex> lockData(store.getDataMutex());
            std::shared_lock<std::shared_mutex> lockList(store.getListMutex());
            json["data"] = store.getData();
            json["list_data"] = store.getListData();
        }
    
        std::filesystem::path currentPath = std::filesystem::current_path();
        std::filesystem::path state = currentPath / STATEFILE;
        std::ofstream outputFile(state);
        outputFile << json.dump(4) << std::endl;
    } catch (const std::exception& e) {
        return false;
    }

    return true;
}

bool Snapshot::load() {
    try {
        Store& store = Store::getInstance();
        std::unique_lock<std::shared_mutex> lockData(store.getDataMutex());
        std::unique_lock<std::shared_mutex> lockList(store.getListMutex());
        std::filesystem::path currentPath = std::filesystem::current_path();
        std::filesystem::path state = currentPath / STATEFILE;
        std::ifstream inputFile(state);
        nlohmann::json json = nlohmann::json::parse(inputFile);
        auto itData = json.find("data");
        if (itData != json.end()) {
            store.setData(itData.value());
        } 
        else {
            throw RedisServerError("Could not find data map");
        }

        auto itListData = json.find("list_data");
        if (itListData != json.end()) {
            store.setListData(itListData.value());
        } 
        else {
            throw RedisServerError("Could not find list_data map");
        }
    } catch (const std::exception& e) {
        return false;
    }
    
    return true;
}

void Snapshot::periodicSave() {
    while (true) {
        save();
        std::this_thread::sleep_for(std::chrono::minutes(config::GlobalConfig.snapshotPeriod));
    }
}
