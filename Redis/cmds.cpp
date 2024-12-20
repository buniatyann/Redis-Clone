#include "cmds.h"
#include "redisstore.h"
#include <unordered_map>

std::unordered_map<std::string, redis_func_type> cmd_map = {
    {"ping", redis_ping},
    {"echo", redis_echo},
    {"set", redis_set},
    {"get", redis_get},
    {"exists", redis_exists},
    {"del", redis_del},
    {"incr", redis_incr},
    {"decr", redis_decr},
    {"lpush", redis_lpush},
    {"rpush", redis_rpush},
    {"lrange", redis_lrange},
    {"save", redis_save},
    {"config", redis_config}   
};

redis_func_type get_redis_func(const std::string& cmd_name){

    auto it  = cmd_map.find(cmd_name);
    if(it != nullptr){
        return it->second;
    }

    // Shouldn't reach here
    throw RedisServerError(cmd_name + " nod found!");

}

cmd_ret_type redis_ping(const std::vector<std::string>& req){

    if(req.size() == 0 || req[0] != "ping"){
        throw RedisServerError("Bad input");
    }

    if(req.size() > 2){
        return std::make_unique<redis::Error>("ERR wrong number of arguments for 'ping command");
    }

    if(req.size() == 1){
        // no arguments provided, return simple string
        return std::make_unique<redis::SimpleString>("PONG");
    }

    // return a bulk string of the argument if any
    return std::make_unique<redis::BulkString>(req[1]);
}

cmd_ret_type redis_echo(const std::vector<std::string>& req){

    if(req.size() == 0 || req[0] != "echo"){
        throw RedisServerError("Bad input");
    }

    if(req.size() != 2){
        return std::make_unique<redis::Error>("ERR wrong number of arguments for 'echo' command");
    }

    return std::make_unique<redis::SimpleString>(req[1]);
}

cmd_ret_type redis_set(const std::vector<std::string>& req){
    if (req.size() == 0 || req[0] != "set") {
        throw RedisServerError("Bad input");
    }

    std::time_t expiry_epoch = LONG_MAX;  // Default to no expiry
    int i = 3;

    if (req.size() < 3) {
        return std::make_unique<redis::Error>("ERR syntax error");
    }

    while (i < req.size()) {
        if (to_lower(req[i]) == "ex") {
            ++i;

            if (i >= req.size()) {
                return std::make_unique<redis::Error>("ERR syntax error");
            }

            std::time_t expiry_from_now;
            try {
                expiry_from_now = std::stol(req[i]);  // Seconds
            } 
            catch (const std::exception& e) {
                return std::make_unique<redis::Error>("ERR value is not an integer or out of range");
            }

            // Calculate expiry time point based on seconds
            auto now = std::chrono::system_clock::now();
            auto expiry = now + std::chrono::seconds(expiry_from_now);
            expiry_epoch = std::chrono::system_clock::to_time_t(expiry);
        }
        else if (to_lower(req[i]) == "px") {
            ++i;

            if (i >= req.size()) {
                return std::make_unique<redis::Error>("ERR syntax error");
            }

            std::time_t expiry_from_now;
            try {
                expiry_from_now = std::stol(req[i]);  // Milliseconds
            } 
            catch (const std::exception& e) {
                return std::make_unique<redis::Error>("ERR value is not an integer or out of range");
            }

            // Calculate expiry time point based on milliseconds
            auto now = std::chrono::system_clock::now();
            auto expiry = now + std::chrono::milliseconds(expiry_from_now);
            expiry_epoch = std::chrono::system_clock::to_time_t(expiry);
        }
        else if (to_lower(req[i]) == "exat") {
            ++i;

            if (i >= req.size()) {
                return std::make_unique<redis::Error>("ERR syntax error");
            }

            try {
                expiry_epoch = std::stol(req[i]);  // Directly set expiry as epoch time in seconds
            } 
            catch (const std::exception& e) {
                return std::make_unique<redis::Error>("ERR value is not an integer or out of range");
            }
        }
        else if (to_lower(req[i]) == "pxat") {
            ++i;

            if (i >= req.size()) {
                return std::make_unique<redis::Error>("ERR syntax error");
            }

            try {
                // Convert milliseconds to seconds by dividing by 1000
                expiry_epoch = std::stol(req[i]) / 1000;
            } 
            catch (const std::exception& e) {
                return std::make_unique<redis::Error>("ERR value is not an integer or out of range");
            }
        }
        else {
            return std::make_unique<redis::Error>("ERR syntax error");
        }

        ++i;
    }

    // Call the set method with the calculated expiry epoch
    RedisStore::getInstance().set(req[1], req[2], expiry_epoch);
    return std::make_unique<redis::SimpleString>("OK");

    // if(req.size() == 0 || req[0] != "set"){
    //     throw RedisServerError("Bad input");
    // }

    // std::time_t expiry_epoch = LONG_MAX;
    // int i = 3;
    // if(req.size() < 3){
    //     return std::make_unique<redis::Error>("ERR syntax error");
    // }

    // while(i < req.size()){
    //     if(to_lower(req[i]) == "ex"){
    //         ++i;
            
    //         if(i >= req.size()){
    //             return std::make_unique<redis::Error>("ERR syntax error");
    //         }

    //         std::time_t expiry_from_now;
            
    //         try{
    //             expiry_from_now = std::stol(req[i]);

    //         }
    //         catch(const std::exception& e){
    //             return std::make_unique<redis::Error>("ERR value is not an integer or out of range");
    //         }

    //         auto now = std::chrono::system_clock::now();
    //         auto expiry = now + std::chrono::system_clock::to_time_t(expiry_from_now);
    //         expiry_epoch = std::chrono::system_clock::to_time_t(expiry);
    //     }
    //     else if(to_lower(req[i]) == "px"){
    //         i++;
            
    //         if(i >= req.size()){
    //             return std::make_unique<redis::Error>("ERR syntax error");

    //         }

    //         std::time_t expiry_from_now;
    //         try{
    //             expiry_from_now = std::stol(req[i]) / 1000;
    //         }
    //         catch(const std::exception& e){
    //             return std::make_unique<redis::Error>("ERR value is not an integer or out of range");
    //         }

    //         auto now = std::chrono::system_clock::now();
    //         auto expiry = now + std::chrono::system_clock::to_time_t(expiry_from_now);
    //         expiry_epoch = std::chrono::system_clock::to_time_t(expiry);
    //     }
    //     else if (to_lower(req[i]) == "exat"){
    //         ++i;
            
    //         if(i >= req.size()){
    //             return std::make_unique<redis::Error>("ERR syntax error");
    //         }
    //         try{
    //             expiry_epoch = std::stol(req[i]);
    //         }
    //         catch(const std::exception e){
    //             return std::make_unique<redis::Error>("ERR value is not an integer or out of range");
    //         }
    //     }
    //     else if(to_lower(req[i]) == "pxat"){
    //         ++i;
    //         if(i >= req.size()){
    //             return std::make_unique<redis::Error>("ERR syntax error");
    //         }

    //         try{
    //             expiry_epoch = std::stol(req[i]) / 1000;
    //         }
    //         catch(const std::exception e){
    //             return std::make_unique<redis::Error>("ERR value is not an integer or out of range");
    //         }
    //     }
    //     else{
    //         return std::make_unique<redis::Error>("ERR syntax error");
    //     }

    //     ++i;

    // }

    // RedisStore::getInstance().set(req[1], req[2], expiry_epoch);
    // return std::make_unique<redis::SimpleString>("OK");

}

cmd_ret_type redis_get(const std::vector<std::string>& req){

    if(req.size() == 0 || req[0] != "get"){
        throw RedisServerError("Bad input");
    }

    if(req.size() != 2){
        return std::make_unique<redis::Error>("ERR wrong number of arguments for 'get' command");
    }

    std::string output;
    bool found = RedisStore::getInstance().get(req[1], output);

    if(!found){
        return std::make_unique<redis::NullString>();
    }

    return std::make_unique<redis::SimpleString>(output);

}

cmd_ret_type redis_exists(const std::vector<std::string>& req){

    if(req.size() == 0 || req[0] != "exists"){
        throw RedisServerError("Bad input");
    }

    int count = 0;
    int i = 1;

    if(req.size() < 2){
        return std::make_unique<redis::Error>("ERR syntax error");
    }

    while(i < req.size()){

        if(RedisStore::getInstance().exists(req[i])){
            count++;
        }
        i++;
    }

    return std::make_unique<redis::Integer>(count);

}

cmd_ret_type redis_del(const std::vector<std::string>& req){

    if(req.size() == 0 || req[0] != "del"){
        throw RedisServerError("Bad input");
    }

    int count = 0;
    int i = 1;

    if(req.size() < 2){
        return std::make_unique<redis::Error>("ERR syntax error");
    }

    while(i < req.size()){
        count == RedisStore::getInstance().erase(req[i]);
        i++;
    }

    return std::make_unique<redis::Integer>(count);

}

cmd_ret_type redis_incr(const std::vector<std::string>& req){
    if(req.size() == 0 || req[0] != "incr"){
        throw RedisServerError("Bad input");
    }

    if(req.size() != 2){
        return std::make_unique<redis::Error>("ERR syntax error");
    }

    try{
        int64_t res = RedisStore::getInstance().incr(req[1]);
        return std::make_unique<redis::Integer>(res);
    }
    catch(const std::exception& e){
        return std::make_unique<redis::Error>("ERR value is not an integer or out of range");
    }
 
}

cmd_ret_type redis_decr(const std::vector<std::string> req){
    
    if(req.size() == 0 || req[0] != "decr"){
        throw RedisServerError("Bad input");
    }

    if(req.size() != 2){
        return std::make_unique<redis::Error>("ERR syntax error");
    }

    try{
        int64_t res = RedisStore::getInstance().incr(req[1], true);
        return std::make_unique<redis::Integer>(res);
    }
    catch(const std::exception& e){
        return std::make_unique<redis::Error>("ERR value is not an integer or out of range");
    }

}

cmd_ret_type redis_lpush(const std::vector<std::string>& req){
    
    if(req.size() == 0 || req[0] != "lpush"){
        throw RedisServerError("Bad input");
    }

    if(req.size() < 3){
        return std::make_unique<redis::Error>("ERR wrong number of argments for 'lpush' command");
    }

    int i = 2;
    std::vector<std::string> vals;

    while(i < req.size()){
        vals.push_back(req[i]);
        i++;
    }

    int res = RedisStore::getInstance().lpush(req[1], vals);
    
    return std::make_unique<redis::Integer>(res);

}

cmd_ret_type redis_rpush(const std::vector<std::string>& req){

    if(req.size() == 0 || req[0] != "rpush"){
        throw RedisServerError("Bad input");
    }

    if(req.size() < 3){
        return std::make_unique<redis::Error>("ERR wrong number of argments for 'rpush' command");
    }
    
    int i = 2;
    std::vector<std::string> vals;

    while(i < req.size()){
        vals.push_back(req[i]);
        i++;
    }
    /////////////////
    int res = RedisStore::getInstance().lpush(req[1], vals, true);
    ///////////
    return std::make_unique<redis::Integer>(res);

}

cmd_ret_type redis_lrange(const std::vector<std::string> req){

    if(req.size() == 0 || req[0] != "lrange"){
        throw RedisServerError("Bad input");
    }

    if(req.size() != 4){
        return std::make_unique<redis::Error>("ERR wrong number of argments for 'lrange' command");
    }

    int64_t start;
    int64_t end;
    std::vector<std::string> res;
    std::unique_ptr<redis::Array> arr = std::make_unique<redis::Array>();

    try{
        start = std::stoll(req[2]);
        end = std::stoll(req[3]);
    }
    catch(const std::exception& e){
        return std::make_unique<redis::Error>("ERR value is not an integer or out of range");
    }

    res = RedisStore::getInstance().lrange(req[1], start, end);

    for(auto str : res){
        arr->add_element(std::make_unique<redis::BulkString>(str));
    }

    return std::move(arr);

}

cmd_ret_type redis_save(const std::vector<std::string>& req){
    
    if(req.size() == 0 || req[0] != "save"){
        throw RedisServerError("Bad input");
    }

    if(req.size() > 1){
        return std::make_unique<redis::Error>("ERR wrong number of argments for 'save' command");
    }

    if(RedisStore::getInstance().dump()){
        return std::make_unique<redis::SimpleString>("OK");
    }

    return std::make_unique<redis::Error>("Couldn't save! Make sure statefile path exists!");

}

cmd_ret_type redis_config_get(const std::vector<std::string>& req){

    std::unique_ptr<redis::Array> arr = std::make_unique<redis::Array>();
    arr->add_element(std::make_unique<redis::BulkString>("900"));
    arr->add_element(std::make_unique<redis::BulkString>("1"));

    return arr; // move not required because of RVO

}

cmd_ret_type redis_config(const std::vector<std::string>& req){

    if(req.size() == 0 || req[0] != "config"){
        throw RedisServerError("Bad input");
    }

    return redis_config_get(req);
    
}