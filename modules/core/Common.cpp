#include "Common.h"

void die(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    return result;
}

int recvExactly(int fd, char* buf, size_t nBytes) {
    while (nBytes > 0) {
        ssize_t bytesRead = recv(fd, buf, nBytes, 0);
        if (bytesRead <= 0) {
            perror("recv");
            return -1;
        }
    
        assert((size_t)bytesRead <= nBytes);
        nBytes -= (size_t)bytesRead;
        buf += (size_t)bytesRead;
    }
    
    return 0;
}

int writeExactly(int fd, const char* buf, size_t nBytes) {
    while (nBytes > 0) {
        ssize_t bytesSent = send(fd, buf, nBytes, 0);
        if (bytesSent <= 0) {
            perror("send");
            return -1;
        }
    
        assert((size_t)bytesSent <= nBytes);
        nBytes -= (size_t)bytesSent;
        buf += (size_t)bytesSent;
    }
    
    return 0;
}
