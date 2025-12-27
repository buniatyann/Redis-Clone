#ifndef RESPONSE_H
#define RESPONSE_H

#include "core/Common.h"

namespace resp {

class Response {
public:
    virtual ~Response() {};
    virtual std::string prefix() = 0;
    virtual std::string serialize() = 0;
    static std::string CRLF;
};

class SimpleString : public Response {
public:
    SimpleString(const std::string& input) : str(input) {}

    std::string prefix() override { return "+"; }

    std::string serialize() override {
        return prefix() + str + CRLF;
    }

private:
    std::string str;
};

class Error : public Response {
public:
    Error(const std::string& input) : str(input) {}

    std::string prefix() override { return "-"; }

    std::string serialize() override {
        return prefix() + str + CRLF;
    }

private:
    std::string str;
};

class Integer : public Response {
public:
    Integer(int input) : val(input) {}

    std::string prefix() override { return ":"; }

    std::string serialize() override {
        return prefix() + std::to_string(val) + CRLF;
    }

private:
    int val;
};

class BulkString : public Response {
public:
    BulkString(const std::string& input) : str(input) {}

    std::string prefix() override { return "$"; }

    std::string serialize() override {
        return prefix() + std::to_string(str.size()) + CRLF + str + CRLF;
    }

private:
    std::string str;
};

class NullString : public Response {
public:
    NullString() : str("-1") {}

    std::string prefix() override { return "$"; }

    std::string serialize() override {
        return prefix() + str + CRLF;
    }

private:
    std::string str;
};

class Array : public Response {
public:
    Array() {}

    std::string prefix() override { return "*"; }

    std::string serialize() override {
        std::string res = prefix() + std::to_string(array.size()) + CRLF;
        for (const auto& it : array) {
            res += it->serialize();
        }
        return res;
    }

    void addElement(std::unique_ptr<Response> robj) {
        array.emplace_back(std::move(robj));
    }

    Array(const Array&) = delete;
    Array& operator=(const Array&) = delete;

private:
    std::vector<std::unique_ptr<Response>> array;
};

}

#endif // RESPONSE_H
