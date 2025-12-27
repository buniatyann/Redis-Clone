#include "RESPParser.h"

std::string RESPParser::readFromFd(int nBytes) {
    char buf[nBytes];
    ssize_t bytesRead = recv(readFd, buf, nBytes, 0);
    if (bytesRead <= 0) {
        throw SysCallFailure("recv failed!");
    }
    return std::string(buf, bytesRead);
}

bool RESPParser::cacheHasValidItem(std::string& item) {
    for (int i = 0; i < readCache.length(); i++) {
        item += readCache[i];

        if (item.length() >= 2 && item[item.length() - 2] == '\r' && item[item.length() - 1] == '\n') {
            readCache = readCache.substr(i + 1);
            return true;
        }
    }
    return false;
}

void RESPParser::updateCache() {
    readCache = readFromFd(READ_CACHE_MAX);
}

std::string RESPParser::readNextItem() {
    std::string item = "";

    while (!cacheHasValidItem(item)) {
        if (item.length() > ITEM_LEN_MAX) {
            throw IncorrectProtocol("item length too big!");
        }
        updateCache();
    }

    return item;
}

bool RESPParser::validateArraySize(const std::string& sizeItem) {
    int len = sizeItem.length();

    if (len < 4) {
        return false;
    }

    if (sizeItem[0] != '*') {
        return false;
    }

    if (sizeItem[len - 1] != '\n' || sizeItem[len - 2] != '\r') {
        return false;
    }

    for (int i = 0; i < len - 2; ++i) {
        if (sizeItem[i] < '0' || sizeItem[i] > '9') {
            return false;
        }
    }

    return true;
}

bool RESPParser::validateBstrSize(const std::string& sizeItem) {
    int len = sizeItem.length();

    if (len < 4) {
        return false;
    }

    if (sizeItem[0] != '$') {
        return false;
    }

    if (sizeItem[len - 1] != '\n' || sizeItem[len - 2] != '\r') {
        return false;
    }

    for (int i = 0; i < len - 2; ++i) {
        if (sizeItem[i] < '0' || sizeItem[i] > '9') {
            return false;
        }
    }

    return true;
}

bool RESPParser::validateCrlf(const std::string& bstr) {
    int len = bstr.length();

    if (len < 2) {
        return false;
    }

    return bstr[len - 2] == '\r' && bstr[len - 1] == '\n';
}

std::vector<std::string> RESPParser::readNewRequest() {
    std::string arrSizeItem = readNextItem();

    if (!validateArraySize(arrSizeItem)) {
        throw IncorrectProtocol("Bad array size");
    }

    int size = std::stoi(arrSizeItem.substr(1, arrSizeItem.length() - 3));

    std::vector<std::string> req(size);

    for (int i = 0; i < size; ++i) {
        std::string bstrSizeItem = readNextItem();

        if (!validateArraySize(bstrSizeItem)) {
            throw IncorrectProtocol("Bad bulk string size");
        }

        int bstrSize = std::stoi(bstrSizeItem.substr(1, bstrSizeItem.length() - 3));

        if (bstrSize == -1) {
            req[i] = NULL_BULK_STRING;
            continue;
        }

        if (bstrSize < -1) {
            throw IncorrectProtocol("Bulk string size is less than -1");
        }

        std::string bstrItem = readNextItem();

        if (!validateCrlf(bstrItem)) {
            throw IncorrectProtocol("Bulk string not terminated by CRLF");
        }

        std::string bstr = bstrItem.substr(0, bstrItem.length() - 2);

        if (bstr.length() != bstrSize) {
            throw IncorrectProtocol("Bulk string size doesn't match");
        }

        req[i] = bstr;
    }

    return req;
}
