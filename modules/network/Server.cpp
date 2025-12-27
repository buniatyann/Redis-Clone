#include <thread>
#include <vector>
#include "Server.h"
#include "commands/Handler.h"
#include "core/Common.h"
#include "protocol/RESPParser.h"
#include "protocol/Response.h"
#include "config/Config.h"

void processRequest(const std::vector<std::string>& req, int clientFd) {
    if (req.size() == 0) {
        return;
    }

    if (req[0] == "COMMAND") {
        const char* response = "*1\r\n$4\r\nPING\r\n";
        writeExactly(clientFd, response, strlen(response));
    }
    else {
        CmdFunc handler = getHandler(req[0]);
        std::unique_ptr<resp::Response> output = handler(req);
        if (output == nullptr) {
            throw RedisServerError("Command failed to return a valid Response!");
        }

        std::string response = output->serialize();
        writeExactly(clientFd, response.c_str(), response.size());
    }
}

void handleClient(int clientFd) {
    RESPParser parser(clientFd);
    while (true) {
        try {
            std::vector<std::string> req = parser.readNewRequest();
            if (req.size() == 0) {
                throw RedisServerError("Read empty request!");
            }

            req[0] = toLower(req[0]);
            processRequest(req, clientFd);
        }
        catch (const std::exception& e) {
            break;
        }

        if (close(clientFd)) {
            die("client");
        }
    }
}

int setupServer() {
    int serverFd;
    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        die("socket");
    }

    int reuse = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        die("setsockopt");
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(config::GlobalConfig.port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(serverFd, (const sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        die("bind");
    }

    if (listen(serverFd, SOMAXCONN) == -1) {
        die("listen");
    }

    std::cout << "Server listening on port: " << config::GlobalConfig.port << std::endl;
    return serverFd;
}

void handleClients(int serverFd) {
    std::vector<std::thread> clientThreads;
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientFd;
        if ((clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientAddrLen)) < 0) {
            die("accept");
        }

        clientThreads.emplace_back(handleClient, clientFd);
    }

    for (auto& thread : clientThreads) {
        thread.join();
    }
}
