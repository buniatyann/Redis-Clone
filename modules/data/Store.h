#ifndef STORE_H
#define STORE_H

#include "core/Common.h"
#include <shared_mutex>
#include <mutex>
#include <deque>
#include <climits>

#define STATEFILE "state.json"

struct ValueEntry {
    std::string val;
    std::time_t expiryEpoch;
};

class Store {
    typedef std::unordered_map<std::string, ValueEntry> DataType;
    typedef std::unordered_map<std::string, std::deque<std::string>> ListType;

public:
    static Store& getInstance();
    static void deleteInstance();

    void set(const std::string& key, const std::string& value, const std::time_t expiryEpoch = LONG_MAX);
    bool get(const std::string& key, std::string& value) const;
    bool exists(const std::string& key) const;
    int erase(const std::string& key);
    int incr(const std::string key, bool reverse = false);
    int lpush(const std::string& key, const std::vector<std::string>& vals, bool reverse = false);
    std::vector<std::string> lrange(const std::string& key, int start, int end);
    void clear();

    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;

    // Expose data for persistence layer
    const DataType& getData() const { return data; }
    const ListType& getListData() const { return listData; }
    void setData(const DataType& d) { data = d; }
    void setListData(const ListType& ld) { listData = ld; }

    std::shared_mutex& getDataMutex() { return dataMutex; }
    std::shared_mutex& getListMutex() { return listMutex; }

private:
    static DataType data;
    static ListType listData;

    Store() {}
    ~Store() {}

    static Store* instance;
    static std::mutex instanceMutex;

    mutable std::shared_mutex dataMutex;
    mutable std::shared_mutex listMutex;
};

#endif // STORE_H
