#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include "compiler.h"
#include "util.h"
#include "vm.h"
#include "version.h"

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
            try {
                int val = std::stoi(needVal(a));
                if (val < 1 || val > 1000000) {
                    die("--cells must be between 1 and 1,000,000");
                }
                cells = val;
            } catch (const std::exception &e) {
                die("invalid value for --cells: " + std::string(e.what()));
            }
        } else if (a == "--dbg") {
            try {
                int val = std::stoi(needVal(a));
                if (val < 1 || val > 1000) {
                    die("--dbg must be between 1 and 1,000");
                }
                dbg = val;
            } catch (const std::exception &e) {
                die("invalid value for --dbg: " + std::string(e.what()));
            }
        } else if (a == "--elastic") {
            elastic = true;
        } else if (a == "--strict") {
            strict = true;
        } else if (a == "--trace") {
            trace = true;
        } else if (a == "--version" || a == "-v") {
            std::cout << "FFS version " << ffs_version::VERSION_STRING << std::endl;
            return 0;
        } else if (a == "--help" || a == "-h") {
            std::cout << "FFS - A Brainfuck-like language interpreter\n"
                    << "Version: " << ffs_version::VERSION_STRING << "\n\n"
                    << "Usage: " << argv[0] << " [OPTIONS]\n\n"
                    << "Options:\n"
                    << "  -f, --file <file>    Input file (default: stdin)\n"
                    << "      --cells <n>      Number of memory cells (default: 30000)\n"
                    << "      --dbg <n>        Debug level (default: 8)\n"
                    << "      --elastic        Enable elastic memory\n"
                    << "      --strict         Enable strict mode\n"
                    << "      --trace          Enable trace mode\n"
                    << "  -v, --version        Show version information\n"
                    << "  -h, --help           Show this help message\n";
            return 0;
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
        try {
            src = read_all(fin);
        } catch (const std::exception &e) {
            die("error reading file: " + std::string(e.what()));
        }
    }

    Program prog = compile_src(src, dbg);
    return run(prog, cells, elastic, strict, dbg, trace, stdin, stdout, stderr);
}
