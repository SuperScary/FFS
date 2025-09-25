#pragma once

#include <string>
#include <vector>

enum class Op {
    INC_PTR,
    DEC_PTR,
    INC,
    DEC,
    OUT,
    IN,
    JZ,
    JNZ,
    ZERO_IF_EOF,
    DBG,
    CLEAR
};

struct Instr {
    Op          op;
    int         arg = 1;
    std::string label;
};

struct Program {
    std::vector<Instr> code;
};
