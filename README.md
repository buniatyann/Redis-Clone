# Redis Clone

**Redis Clone** is a simple, in-memory key-value store that replicates the basic functionality of the popular database system Redis. It was created from scratch in C++ with the goal of understanding how Redis and server applications work at a deep level. 
This project is a functional implementation of Redis-like commands, but not optimized for performance in the same way as the official Redis server.

**Redis Clone** supports the following Redis commands:
- `SET`
- `GET`
- `EXISTS`
- `DEL`
- `INCR`
- `DECR`
- `LPUSH`
- `RPUSH`
- `LRANGE`
- `SAVE`

**Note**: Redis Clone is not intended to outperform official Redis, but rather to serve as a learning tool for understanding the internal workings of an in-memory database.

## Building Redis Clone

Follow these steps to build **Redis Clone** from source:

1. Clone the repository and set the path to the cloned directory as `$REDIS_HOME`.
2. Navigate to the `bin` directory:
    ```bash
    $ cd $REDIS_HOME
    $ mkdir ${REDIS_HOME}/bin
    $ cd bin
    ```
3. Build the project using `CMake`:
    ```bash
    $ cmake $REDIS_HOME
    $ make
    ```

This will generate the executable named `redis` in the `bin` directory.

## Running Redis Clone

To run the **Redis Clone** server:

```bash
$ cd $REDIS_HOME
$ bin/redis
```

##Key Differences from Official Redis

`Thread Model`: Redis Clone uses one thread per client connection, unlike the official Redis, which uses a single-threaded event loop. This choice was made for simplicity and ease of implementation, avoiding the complexity of async I/O in C++.

`Multi-threading and Locking`: Since Redis Clone is multi-threaded, it implements necessary locking to protect against data races.

`Persistence`: Redis Clone supports persistence by dumping snapshots of its in-memory state into a state.json file located in the $REDIS_HOME directory. This is in contrast to Redis, which uses a binary RDB file. The state.json file is human-readable, making it easy to inspect changes to the data. If a previous snapshot exists, myRedis will load it at startup.

`Configuration`: The server’s settings are controlled via a config.json file, which includes:

`port`: The port the server listens on.

`snapshot_period`: The time period (in minutes) for periodic snapshots of the in-memory state.

## server.cpp
This is the main file, which starts up the server and launches a new thread for every client connection. This also creates the snapshot thread which periodically wakes up after a fixed time interval to dump Redis Clone's state.

## RESPParser.cpp (.h)
Redis uses RESP protocol to exchange messages between server and client.

Each client request is an array of bulk strings. This file implements the deserialization logic of the client request including the logic to read from client `fd`.

Since RESP uses `\r\n` (CRLF) to separate two meaningful items, a RESP parser would have to process the serialized message item by item. Instead of reading item by item from socket, Redis Clone has a read cache of 8192 bytes which prevents parser from making large number of expensive read syscalls.

Each client thread have a RESPParser object which exposes `read_new_request()` method.

## redisstore.cpp (.h)
This file implements the data structures which house all the data stored in myRedis. RedisStore is a singleton class which exposes relevant methods required by redis cmds.

## cmds.cpp (.h)
This file houses the implementation of Redis cmds. These are essentially a thin wrapper to do validation of redis cmds before calling actual methods of RedisStore.

## type.cpp (.h)
To send a reply to redis-client, redis-server also needs to serialize output as per RESP. There are multiple data types supported in RESP each of which have there own serialization logic.

This file defines a base class `RObject` from which all fancy types (string, error, integers, bulk string, array) inherit. Each sub-class needs to define it's own `serialize()` method as per RESP.

Arguably some part of RESPParser should have utilized object definitions here, and it might have been nicer to have serialization and deserialization logic in one place. However RESPParser had much more nuances because of validations required before successful deserialization. This is why myRedis has opted to keep them separate.

## config.cpp (.h)
Enables reading up config from the `config.json` file.
