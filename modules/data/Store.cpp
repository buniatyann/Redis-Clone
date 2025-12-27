#include "Store.h"

Store* Store::instance = nullptr;
std::mutex Store::instanceMutex;
Store::DataType Store::data;
Store::ListType Store::listData;

Store& Store::getInstance() {
    if (instance == nullptr) {
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (instance == nullptr) {
            instance = new Store();
        }
    }

    return *instance;
}

void Store::deleteInstance() {
    if (instance != nullptr) {
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (instance != nullptr) {
            instance->clear();
            delete instance;
            instance = nullptr;
        }
    }
}

void Store::clear() {
    {
        std::unique_lock<std::shared_mutex> lock(dataMutex);
        data.clear();
    }
    {
        std::unique_lock<std::shared_mutex> lock(listMutex);
        listData.clear();
    }
}

void Store::set(const std::string& key, const std::string& value, const std::time_t expiryEpoch) {
    std::unique_lock<std::shared_mutex> lock(dataMutex);
    data[key] = {value, expiryEpoch};
}

static std::time_t nowEpoch() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::system_clock::to_time_t(now);
}

bool Store::get(const std::string& key, std::string& value) const {
    bool removeKey = false;
    {
        std::shared_lock<std::shared_mutex> lock(dataMutex);
        auto it = data.find(key);
        if (it != data.end()) {
            if (it->second.expiryEpoch > nowEpoch()) {
                value = it->second.val;
                return true;
            } 
            else {
                removeKey = true;
            }
        }
    }

    if (removeKey) {
        std::unique_lock<std::shared_mutex> lock(dataMutex);
        data.erase(key);
    }
    
    return false;
}

bool Store::exists(const std::string& key) const {
    static std::string temp;
    bool inData = get(key, temp);
    bool inListData = false;
    std::shared_lock<std::shared_mutex> lock(listMutex);
    auto it = listData.find(key);
    if (it != listData.end()) {
        inListData = true;
    }
    
    return inData || inListData;
}

int Store::erase(const std::string& key) {
    std::unique_lock<std::shared_mutex> lockData(dataMutex);
    int inData = data.erase(key);
    if (inData) {
        return inData;
    }
    
    std::unique_lock<std::shared_mutex> lockList(listMutex);
    int inListData = listData.erase(key);
    if (inListData) {
        return inListData;
    }
    
    return 0;
}

int Store::incr(const std::string key, bool reverse) {
    std::unique_lock<std::shared_mutex> lock(dataMutex);
    std::string strVal;
    auto it = data.find(key);
    if (it != data.end() && it->second.expiryEpoch > nowEpoch()) {
        int64_t intVal;
        try {
            intVal = std::stoll(it->second.val);
        } catch (const std::exception& e) {
            throw e;
        }
    
        int delta = reverse ? -1 : 1;
        strVal = std::to_string(intVal + delta);
        it->second.val = strVal;
        return intVal + delta;
    } 
    else {
        data[key] = {reverse ? "-1" : "1", LONG_MAX};
        return reverse ? -1 : 1;
    }
    
    return -1;
}

int Store::lpush(const std::string& key, const std::vector<std::string>& vals, bool reverse) {
    std::unique_lock<std::shared_mutex> lock(listMutex);
    auto it = listData.find(key);
    if (it == listData.end()) {
        listData[key] = {};
    }

    for (auto val : vals) {
        if (reverse) {
            listData[key].push_back(val);
        } 
        else {
            listData[key].push_front(val);
        }
    }

    return listData[key].size();
}

std::vector<std::string> Store::lrange(const std::string& key, int start, int end) {
    std::shared_lock<std::shared_mutex> lock(listMutex);
    auto it = listData.find(key);
    if (it == listData.end()) {
        return {};
    }

    if (start < 0) {
        start = it->second.size() + start;
        if (start < 0) {
            return {};
        }
    }

    if (end < 0) {
        end = it->second.size() + end;
        if (end < 0) {
            return {};
        }
    }

    if (end >= it->second.size()) {
        end = it->second.size() - 1;
    }

    if (start > end) {
        return {};
    }

    std::vector<std::string> res;
    for (int i = start; i <= end && i < it->second.size(); i++) {
        res.push_back(it->second[i]);
    }

    return res;
}
