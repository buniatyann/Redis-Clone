#include <fstream> 
#include <filesystem>
#include "redisstore.h"

RedisStore* RedisStore::_Instance = nullptr;
std::mutex RedisStore::_Instancemutex;
RedisStore::data_type RedisStore::data;
RedisStore::list_type RedisStore::list_data;

RedisStore& RedisStore::getInstance() {

    if (_Instance == nullptr) {
        std::lock_guard<std::mutex> lock(_Instancemutex);
        if (_Instance == nullptr) {
            _Instance = new RedisStore();
        }
    }

    return *_Instance;
    
}

void RedisStore::deleteInstance() {
    
    if (_Instance != nullptr) {
        std::lock_guard<std::mutex> lock(_Instancemutex);
        if (_Instance != nullptr) {
            _Instance->clear();
            delete _Instance;
            _Instance = nullptr;
        }
    }
}

void RedisStore::clear() {

    {
        std::unique_lock<std::shared_mutex> lock(_datamutex);
        data.clear();
    }
    {
        std::unique_lock<std::shared_mutex> lock(_listmutex);
        list_data.clear();
    }
    
}

void RedisStore::set(const std::string& key, const std::string& value, const std::time_t expiry_epoch) {
    std::unique_lock<std::shared_mutex> lock(_datamutex);
    data[key] = {value, expiry_epoch}; // never expires
}

static std::time_t now_epoch() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::system_clock::to_time_t(now);
}

bool RedisStore::get(const std::string& key, std::string& value) const {

    bool remove_key = false;
    {
        std::shared_lock<std::shared_mutex> lock(_datamutex);
        auto it = data.find(key);
        if (it != data.end()) {

            if (it->second.expiry_epoch > now_epoch()) {
                value = it->second.val;
                return true;
            } else {
                // key expired, remove it
                remove_key = true;
                // can not upgrade to unique lock here as it
                // can lead to deadlock with another get call
            }
        }
    }
    if (remove_key) {
        // lazy removal
        std::unique_lock<std::shared_mutex> lock(_datamutex);
        data.erase(key);
    }

    return false;

}

bool RedisStore::exists(const std::string& key) const {

    static std::string temp;
    bool in_data = get(key, temp);
    bool in_list_data = false;
    std::shared_lock<std::shared_mutex> lock(_listmutex);
    auto it = list_data.find(key);
    if (it != list_data.end()) {
        in_list_data = true;
    }

    return in_data || in_list_data;

}

int RedisStore::erase(const std::string& key) {
    std::unique_lock<std::shared_mutex> lockdata(_datamutex);
    int in_data = data.erase(key);
    if (in_data) {
        return in_data;
    }
    std::unique_lock<std::shared_mutex> locklist(_listmutex);
    int in_list_data = list_data.erase(key);
    if (in_list_data) {
        return in_list_data;
    }

    return 0;

}


// int RedisStore::incr(const std::string& key, bool reverse) {
//     std::unique_lock<std::shared_mutex> lock(_datamutex);
//     std::string str_val;
//     auto it = data.find(key);
//     if (it != data.end() && it->second.expiry_epoch > now_epoch()) {
//         int64_t int_val;
//         try{
//             int_val = std::stoll(it->second.val);
//         } catch (const std::exception& e) {
//             throw e;
//         }
//         int delta = reverse ? -1 : 1;
//         str_val = std::to_string(int_val+delta);
//         it->second.val = str_val;
//         return int_val+delta;
//     } else {
//         data[key] = {reverse ? "-1" : "1", LONG_MAX};
//         return reverse ? -1 : 1;
//     }

//     // should not reach here
    
//     return -1;
// }

/**
 * reverse set to true means decr
*/
int RedisStore::incr(const std::string key, bool reverse)
{
    //return 0;
    std::unique_lock<std::shared_mutex> lock(_datamutex);
    std::string str_val;
    auto it = data.find(key);
    if (it != data.end() && it->second.expiry_epoch > now_epoch()) {
        int64_t int_val;
        try{
            int_val = std::stoll(it->second.val);
        } catch (const std::exception& e) {
            throw e;
        }
        int delta = reverse ? -1 : 1;
        str_val = std::to_string(int_val+delta);
        it->second.val = str_val;
        return int_val+delta;
    } else {
        data[key] = {reverse ? "-1" : "1", LONG_MAX};
        return reverse ? -1 : 1;
    }

    // should not reach here
    
    return -1;
}

int RedisStore::lpush(const std::string &key, const std::vector<std::string> &vals, bool reverse)
{

    std::unique_lock<std::shared_mutex> lock(_listmutex);

    auto it = list_data.find(key);
    if (it == list_data.end()) {
        // maybe this is not required
        list_data[key] = {};
    }

    for (auto val : vals) {
        if (reverse) {
            list_data[key].push_back(val);
        } else {
            list_data[key].push_front(val);
        }
    }

    return list_data[key].size();
}

std::vector<std::string> RedisStore::lrange(const std::string& key, int start, int end) {
    std::shared_lock<std::shared_mutex> lock(_listmutex);
    auto it = list_data.find(key);
    
    if (it == list_data.end()) {
        return {};
    }

    if (start < 0) {
        start = it->second.size() + start;
        if (start < 0) {
            // I think this should return empty list
            // unless we are wrapping around
            return {};
        }
    }

    if (end < 0) {
        end = it->second.size() + end;
        if (end < 0) {
            // I think this should return empty list
            // unless we are wrapping around
            return {};
        }
    }
    
    if (end >= it->second.size()) {
        end = it->second.size()-1;
    }
    
    if (start > end) {
        return {};
    }
    
    std::vector<std::string> res;
    
    for (int i=start; i<=end && i < it->second.size(); i++) {
        res.push_back(it->second[i]);
    }

    return res;

}

void to_json(nlohmann::json& j, const ValueEntry& v) {
    j = nlohmann::json{{"val", v.val}, {"expiry_epoch", v.expiry_epoch}};
}

void from_json(const nlohmann::json& j, ValueEntry& v) {
    j.at("val").get_to(v.val);
    j.at("expiry_epoch").get_to(v.expiry_epoch);
}

bool RedisStore::dump() {

    try {
        nlohmann::json json; // will contain data to be dumped
        {
            // Acquire read locks
            std::shared_lock<std::shared_mutex> lockdata(_datamutex);
            std::shared_lock<std::shared_mutex> locklist(_listmutex);

            // json library needs to_json()
            // defined for every type involved

            json["data"] = data;
            json["list_data"] = list_data;
        }
        std::filesystem::path current_path = std::filesystem::current_path();
        std::filesystem::path state = current_path / STATEFILE;
        // Dump into STATEFILE (state.json) in current directory
        std::ofstream outputFile(state);
        outputFile << json.dump(4) << std::endl;
    } catch (const std::exception& e) {
        // something went wrong
        // maybe log
        return false;
    }

    return true;

}

bool RedisStore::restore() {

    try {

        // Acquire write locks
        std::unique_lock<std::shared_mutex> lockdata(_datamutex);
        std::unique_lock<std::shared_mutex> locklist(_listmutex);

        std::filesystem::path current_path = std::filesystem::current_path();
        std::filesystem::path state = current_path / STATEFILE;

        // Expect STATEFILE (state.json) in current directory

        std::ifstream inputFile(state);
        nlohmann::json json = nlohmann::json::parse(inputFile);
        auto it_data = json.find("data");
        if (it_data != json.end()) {
            data = it_data.value();
        } else {
            throw RedisServerError("Could not find data map");
        }
        auto it_list_data = json.find("list_data");
        if (it_list_data != json.end()) {
            list_data = it_list_data.value();
        } else {
            throw RedisServerError("Could not find list_data map");
        }

    } catch (const std::exception& e) {
        // something went wrong
        // maybe log
        return false;
    }

    return true;

}