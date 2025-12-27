# Redis Clone

**Redis Clone** is a simple, in-memory key-value store that replicates the basic functionality of the popular database system Redis. It was created from scratch in C++ with the goal of understanding how Redis and server applications work at a deep level.

## Supported Commands

- `PING` / `ECHO`
- `SET` (with EX/PX/EXAT/PXAT options)
- `GET`
- `EXISTS`
- `DEL`
- `INCR` / `DECR`
- `LPUSH` / `RPUSH`
- `LRANGE`
- `SAVE`
- `CONFIG GET`

**Note**: Redis Clone is not intended to outperform official Redis, but rather to serve as a learning tool for understanding the internal workings of an in-memory database.

## Building

```bash
mkdir build && cd build
cmake ..
make
```

The executable `redis` is output to `build/app/redis`.

## Running

```bash
cd $REDIS_HOME
./build/app/redis
```

Ensure `config.json` exists in the current directory.

## Project Structure (Storage Engine Architecture)

```
├── app/                        # Application entry point
│   └── main.cpp
├── modules/                    # Core library (redis_core)
│   ├── network/                # Connection handling
│   │   └── Server.*            # TCP socket, client threads
│   ├── commands/               # Command execution
│   │   └── Handler.*           # Command dispatch and implementations
│   ├── data/                   # Data structures
│   │   └── Store.*             # Singleton key-value store
│   ├── protocol/               # Protocol handling
│   │   ├── RESPParser.*        # RESP protocol parser
│   │   └── Response.*          # RESP response serialization
│   ├── persistence/            # Disk I/O
│   │   └── Snapshot.*          # State save/restore to JSON
│   ├── config/                 # Configuration
│   │   └── Config.*            # JSON config loading
│   └── core/                   # Common utilities
│       └── Common.*            # I/O helpers, exceptions
├── third_party/nlohmann/       # JSON library
└── config/                     # Config file example
```

## Architecture

The project follows **Storage Engine Architecture**, commonly used by databases like Redis and LevelDB:

```
Client Request
      │
      ▼
┌─────────────┐
│   Network   │  TCP socket, client thread handling
└──────┬──────┘
       ▼
┌─────────────┐
│  Protocol   │  RESP parsing and response serialization
└──────┬──────┘
       ▼
┌─────────────┐
│  Commands   │  Command dispatch and execution
└──────┬──────┘
       ▼
┌─────────────┐
│    Data     │  In-memory storage (hash maps, deques)
└──────┬──────┘
       ▼
┌─────────────┐
│ Persistence │  Periodic snapshots to JSON
└─────────────┘
```

## Key Differences from Official Redis

| Aspect | Redis Clone | Official Redis |
|--------|-------------|----------------|
| Thread Model | One thread per client | Single-threaded event loop |
| Concurrency | `shared_mutex` locking | No locking needed |
| Persistence | JSON snapshots | Binary RDB/AOF |
| Protocol | RESP (subset) | Full RESP2/RESP3 |

## Configuration

Settings are controlled via `config.json`:

```json
{
    "port": 6379,
    "snapshot_period": 5
}
```

- `port`: The port the server listens on
- `snapshot_period`: Time period (in minutes) for periodic snapshots

## Module Details

### network/Server
Main server loop that accepts connections and spawns a thread per client. Also starts the periodic snapshot thread.

### protocol/RESPParser
Deserializes RESP protocol messages. Uses an 8KB read cache to minimize syscalls. Each client thread has its own parser instance.

### protocol/Response
Serializes responses back to RESP format. Defines base class `Response` with subclasses for each RESP type (SimpleString, Error, Integer, BulkString, Array).

### data/Store
Singleton class containing the in-memory data structures:
- `unordered_map<string, ValueEntry>` for key-value pairs with expiry
- `unordered_map<string, deque<string>>` for list operations

### persistence/Snapshot
Handles saving/loading state to `state.json`. Runs periodically in a background thread.

### commands/Handler
Implements all Redis commands as thin wrappers that validate input before calling Store methods.

### config/Config
Reads server configuration from `config.json`.
