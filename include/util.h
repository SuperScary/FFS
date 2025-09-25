#pragma once

#include <iosfwd>
#include <string>

[[noreturn]] void die (const std::string &msg);

std::string read_all (std::istream &in);
