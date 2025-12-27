#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <sstream>
#include <cassert>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <string>
#include <chrono>
#include <functional>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <mutex>
#include <climits>
#include <stdexcept>

void die(const char* msg);

int recvExactly(int fd, char* buf, size_t nBytes);

int writeExactly(int fd, const char* buf, size_t nBytes);

std::string toLower(const std::string& str);

class IncorrectProtocol : public std::runtime_error {
public:
    explicit IncorrectProtocol(const std::string& message) : std::runtime_error(message) {}
};

class SysCallFailure : public std::runtime_error {
public:
    explicit SysCallFailure(const std::string& message) : std::runtime_error(message) {}
};

class RedisServerError : public std::runtime_error {
public:
    RedisServerError(const std::string& message) : std::runtime_error(message) {}
};

#endif // COMMON_H
