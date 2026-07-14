#pragma once
#include <nanodi.hpp>
#include <optional>
#include <string>

struct JSContext;

namespace notojs {

class Folder;
class Server;

class Errors : public Requires<Folder, Server>
{
public:
    Errors() = default;
    void error(std::string const &, std::string &&, std::optional<std::string> &&);
};

} // namespace notojs
