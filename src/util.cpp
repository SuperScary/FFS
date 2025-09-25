#include "util.h"

#include <cstdio>
#include <sstream>

[[noreturn]] void die (const std::string &msg) {
    std::fprintf(stderr, "FFS: %s\n", msg.c_str());
    std::exit(1);
}

std::string read_all (std::istream &in) {
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}
