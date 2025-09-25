#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include "compiler.h"
#include "util.h"
#include "vm.h"

int main (int argc, char **argv) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string file;
    int         cells   = 30000;
    int         dbg     = 8;
    bool        elastic = false;
    bool        strict  = false;
    bool        trace   = false;

    for (int i = 1; i < argc; ++i) {
        std::string a       = argv[i];
        auto        needVal = [&](const std::string &name) {
            if (i + 1 >= argc) {
                die("missing value for " + name);
            }
            return std::string(argv[++i]);
        };
        if (a == "-f" || a == "--file") {
            file = needVal(a);
        } else if (a == "--cells") {
            cells = std::stoi(needVal(a));
        } else if (a == "--dbg") {
            dbg = std::stoi(needVal(a));
        } else if (a == "--elastic") {
            elastic = true;
        } else if (a == "--strict") {
            strict = true;
        } else if (a == "--trace") {
            trace = true;
        } else {
            die("unknown flag: " + a);
        }
    }

    std::string src;
    if (file.empty()) {
        src = read_all(std::cin);
    } else {
        std::ifstream fin(file, std::ios::binary);
        if (!fin) {
            die("could not open file: " + file);
        }
        src = read_all(fin);
    }

    Program prog = compile_src(src, dbg);
    return run(prog, cells, elastic, strict, dbg, trace, stdin, stdout, stderr);
}
