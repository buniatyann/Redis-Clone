#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include "config/Config.h"

namespace Snapshot {
    bool save();
    bool load();
    void periodicSave();
}

#endif // SNAPSHOT_H
