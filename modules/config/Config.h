#ifndef CONFIG_H
#define CONFIG_H

#include "core/Common.h"

#define CONFIG_FILE "config.json"

namespace config {
    struct Settings {
        int port;
        std::string statefile;
        int snapshotPeriod;
    };

    extern Settings GlobalConfig;
    bool load();
}

#endif // CONFIG_H
