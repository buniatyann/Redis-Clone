#include "Handler.h"
#include "data/Store.h"
#include "persistence/Snapshot.h"
#include <unordered_map>

std::unordered_map<std::string, CmdFunc> cmdMap = {
    {"ping", cmdPing},
    {"echo", cmdEcho},
    {"set", cmdSet},
    {"get", cmdGet},
    {"exists", cmdExists},
    {"del", cmdDel},
    {"incr", cmdIncr},
    {"decr", cmdDecr},
    {"lpush", cmdLpush},
    {"rpush", cmdRpush},
    {"lrange", cmdLrange},
    {"save", cmdSave},
    {"config", cmdConfig}
};

CmdFunc getHandler(const std::string& cmdName) {
    auto it = cmdMap.find(cmdName);
    if (it != cmdMap.end()) {
        return it->second;
    }

    throw RedisServerError(cmdName + " not found!");
}

CmdResult cmdPing(const std::vector<std::string>& req) {
    if (req.size() == 0 || req[0] != "ping") {
        throw RedisServerError("Bad input");
    }
    if (req.size() > 2) {
        return std::make_unique<resp::Error>("ERR wrong number of arguments for 'ping command");
    }
    if (req.size() == 1) {
        return std::make_unique<resp::SimpleString>("PONG");
    }

    return std::make_unique<resp::BulkString>(req[1]);
}

CmdResult cmdEcho(const std::vector<std::string>& req) {
    if (req.size() == 0 || req[0] != "echo") {
        throw RedisServerError("Bad input");
    }
    if (req.size() != 2) {
        return std::make_unique<resp::Error>("ERR wrong number of arguments for 'echo' command");
    }

    return std::make_unique<resp::SimpleString>(req[1]);
}

CmdResult cmdSet(const std::vector<std::string>& req) {
    if (req.size() == 0 || req[0] != "set") {
        throw RedisServerError("Bad input");
    }

    std::time_t expiryEpoch = LONG_MAX;
    int i = 3;
    if (req.size() < 3) {
        return std::make_unique<resp::Error>("ERR syntax error");
    }

    while (i < req.size()) {
        if (toLower(req[i]) == "ex") {
            ++i;
            if (i >= req.size()) {
                return std::make_unique<resp::Error>("ERR syntax error");
            }

            std::time_t expiryFromNow;
            try {
                expiryFromNow = std::stol(req[i]);
            } catch (const std::exception& e) {
                return std::make_unique<resp::Error>("ERR value is not an integer or out of range");
            }

            auto now = std::chrono::system_clock::now();
            auto expiry = now + std::chrono::seconds(expiryFromNow);
            expiryEpoch = std::chrono::system_clock::to_time_t(expiry);
        }
        else if (toLower(req[i]) == "px") {
            ++i;
            if (i >= req.size()) {
                return std::make_unique<resp::Error>("ERR syntax error");
            }

            std::time_t expiryFromNow;
            try {
                expiryFromNow = std::stol(req[i]);
            } catch (const std::exception& e) {
                return std::make_unique<resp::Error>("ERR value is not an integer or out of range");
            }

            auto now = std::chrono::system_clock::now();
            auto expiry = now + std::chrono::milliseconds(expiryFromNow);
            expiryEpoch = std::chrono::system_clock::to_time_t(expiry);
        }
        else if (toLower(req[i]) == "exat") {
            ++i;
            if (i >= req.size()) {
                return std::make_unique<resp::Error>("ERR syntax error");
            }

            try {
                expiryEpoch = std::stol(req[i]);
            } catch (const std::exception& e) {
                return std::make_unique<resp::Error>("ERR value is not an integer or out of range");
            }
        }
        else if (toLower(req[i]) == "pxat") {
            ++i;
            if (i >= req.size()) {
                return std::make_unique<resp::Error>("ERR syntax error");
            }

            try {
                expiryEpoch = std::stol(req[i]) / 1000;
            } catch (const std::exception& e) {
                return std::make_unique<resp::Error>("ERR value is not an integer or out of range");
            }
        }
        else {
            return std::make_unique<resp::Error>("ERR syntax error");
        }

        ++i;
    }

    Store::getInstance().set(req[1], req[2], expiryEpoch);
    return std::make_unique<resp::SimpleString>("OK");
}

CmdResult cmdGet(const std::vector<std::string>& req) {
    if (req.size() == 0 || req[0] != "get") {
        throw RedisServerError("Bad input");
    }

    if (req.size() != 2) {
        return std::make_unique<resp::Error>("ERR wrong number of arguments for 'get' command");
    }

    std::string output;
    bool found = Store::getInstance().get(req[1], output);
    if (!found) {
        return std::make_unique<resp::NullString>();
    }

    return std::make_unique<resp::SimpleString>(output);
}

CmdResult cmdExists(const std::vector<std::string>& req) {
    if (req.size() == 0 || req[0] != "exists") {
        throw RedisServerError("Bad input");
    }

    int count = 0;
    int i = 1;
    if (req.size() < 2) {
        return std::make_unique<resp::Error>("ERR syntax error");
    }

    while (i < req.size()) {
        if (Store::getInstance().exists(req[i])) {
            count++;
        }

        ++i;
    }

    return std::make_unique<resp::Integer>(count);
}

CmdResult cmdDel(const std::vector<std::string>& req) {
    if (req.size() == 0 || req[0] != "del") {
        throw RedisServerError("Bad input");
    }

    int count = 0;
    int i = 1;
    if (req.size() < 2) {
        return std::make_unique<resp::Error>("ERR syntax error");
    }

    while (i < req.size()) {
        count += Store::getInstance().erase(req[i]);
        ++i;
    }

    return std::make_unique<resp::Integer>(count);
}

CmdResult cmdIncr(const std::vector<std::string>& req) {
    if (req.size() == 0 || req[0] != "incr") {
        throw RedisServerError("Bad input");
    }
    if (req.size() != 2) {
        return std::make_unique<resp::Error>("ERR syntax error");
    }

    try {
        int64_t res = Store::getInstance().incr(req[1]);
        return std::make_unique<resp::Integer>(res);
    } catch (const std::exception& e) {
        return std::make_unique<resp::Error>("ERR value is not an integer or out of range");
    }
}

CmdResult cmdDecr(const std::vector<std::string>& req) {
    if (req.size() == 0 || req[0] != "decr") {
        throw RedisServerError("Bad input");
    }
    if (req.size() != 2) {
        return std::make_unique<resp::Error>("ERR syntax error");
    }

    try {
        int64_t res = Store::getInstance().incr(req[1], true);
        return std::make_unique<resp::Integer>(res);
    } catch (const std::exception& e) {
        return std::make_unique<resp::Error>("ERR value is not an integer or out of range");
    }
}

CmdResult cmdLpush(const std::vector<std::string>& req) {
    if (req.size() == 0 || req[0] != "lpush") {
        throw RedisServerError("Bad input");
    }
    if (req.size() < 3) {
        return std::make_unique<resp::Error>("ERR wrong number of arguments for 'lpush' command");
    }

    int i = 2;
    std::vector<std::string> vals;
    while (i < req.size()) {
        vals.push_back(req[i]);
        ++i;
    }

    int res = Store::getInstance().lpush(req[1], vals);
    return std::make_unique<resp::Integer>(res);
}

CmdResult cmdRpush(const std::vector<std::string>& req) {
    if (req.size() == 0 || req[0] != "rpush") {
        throw RedisServerError("Bad input");
    }

    if (req.size() < 3) {
        return std::make_unique<resp::Error>("ERR wrong number of arguments for 'rpush' command");
    }

    int i = 2;
    std::vector<std::string> vals;
    while (i < req.size()) {
        vals.push_back(req[i]);
        ++i;
    }

    int res = Store::getInstance().lpush(req[1], vals, true);
    return std::make_unique<resp::Integer>(res);
}

CmdResult cmdLrange(const std::vector<std::string>& req) {
    if (req.size() == 0 || req[0] != "lrange") {
        throw RedisServerError("Bad input");
    }
    if (req.size() != 4) {
        return std::make_unique<resp::Error>("ERR wrong number of arguments for 'lrange' command");
    }

    int64_t start;
    int64_t end;
    std::vector<std::string> res;
    std::unique_ptr<resp::Array> arr = std::make_unique<resp::Array>();

    try {
        start = std::stoll(req[2]);
        end = std::stoll(req[3]);
    } catch (const std::exception& e) {
        return std::make_unique<resp::Error>("ERR value is not an integer or out of range");
    }

    res = Store::getInstance().lrange(req[1], start, end);
    for (auto str : res) {
        arr->addElement(std::make_unique<resp::BulkString>(str));
    }

    return std::move(arr);
}

CmdResult cmdSave(const std::vector<std::string>& req) {
    if (req.size() == 0 || req[0] != "save") {
        throw RedisServerError("Bad input");
    }
    if (req.size() > 1) {
        return std::make_unique<resp::Error>("ERR wrong number of arguments for 'save' command");
    }
    if (Snapshot::save()) {
        return std::make_unique<resp::SimpleString>("OK");
    }

    return std::make_unique<resp::Error>("Couldn't save! Make sure statefile path exists!");
}

CmdResult cmdConfigGet(const std::vector<std::string>& req) {
    std::unique_ptr<resp::Array> arr = std::make_unique<resp::Array>();
    arr->addElement(std::make_unique<resp::BulkString>("900"));
    arr->addElement(std::make_unique<resp::BulkString>("1"));

    return arr;
}

CmdResult cmdConfig(const std::vector<std::string>& req) {
    if (req.size() == 0 || req[0] != "config") {
        throw RedisServerError("Bad input");
    }

    return cmdConfigGet(req);
}
