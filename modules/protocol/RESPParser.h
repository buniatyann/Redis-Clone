#ifndef RESPPARSER_H
#define RESPPARSER_H

#include "core/Common.h"

#define NULL_BULK_STRING "NULL"
#define READ_CACHE_MAX 8912
#define ITEM_LEN_MAX 536870912

class RESPParser {

private:
    int readFd = -1;
    std::string readCache = "";

protected:
    bool validateArraySize(const std::string& sizeItem);
    bool validateBstrSize(const std::string& sizeItem);
    bool validateCrlf(const std::string& bstr);
    bool cacheHasValidItem(std::string& item);
    void updateCache();

    std::string readFromFd(int nBytes);
    std::string readNextItem();

public:
    RESPParser(int fd) {
        readFd = fd;
        readCache = "";
    }

    std::vector<std::string> readNewRequest();
};

#endif // RESPPARSER_H
