#include "network/Server.h"
#include "config/Config.h"
#include "data/Store.h"
#include "persistence/Snapshot.h"
#include "core/Common.h"
#include <thread>
#include <iostream>

int main() {
    if (!config::load()) {
        std::cout << "Unable to read config\n"
                  << "Please ensure config.json exists and is correctly setup"
                  << std::endl;
        return 0;
    }

    if (!Snapshot::load()) {
        std::cout << "State restoral failed! Continuing with empty state..." << std::endl;
    } 
    else {
        std::cout << "Previous state restored!" << std::endl;
    }

    std::thread snapshotThread(Snapshot::periodicSave);
    int serverFd = setupServer();
    handleClients(serverFd);
    if (close(serverFd)) {
        die("close");
    }

    snapshotThread.join();
    Store::deleteInstance();

    return 0;
}
