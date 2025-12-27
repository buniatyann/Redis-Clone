#ifndef SERVER_H
#define SERVER_H

#include <vector>
#include <string>

void processRequest(const std::vector<std::string>& req, int clientFd);
void handleClient(int clientFd);
int setupServer();
void handleClients(int serverFd);

#endif // SERVER_H
