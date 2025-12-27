#ifndef HANDLER_H
#define HANDLER_H

#include <unordered_map>
#include "core/Common.h"
#include "protocol/Response.h"

using CmdResult = std::unique_ptr<resp::Response>;
using CmdFunc = std::function<CmdResult(const std::vector<std::string>&)>;

#define CMD(NAME) CmdResult cmd##NAME(const std::vector<std::string>& req);

CMD(Ping)
CMD(Echo)
CMD(Set)
CMD(Get)
CMD(Exists)
CMD(Del)
CMD(Incr)
CMD(Decr)
CMD(Lpush)
CMD(Rpush)
CMD(Lrange)
CMD(Save)
CMD(Config)

CmdFunc getHandler(const std::string& cmdName);

#endif // HANDLER_H
