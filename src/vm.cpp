#include "vm.h"

#include "util.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

int run (const Program &p,
         int            initCells,
         bool           elastic,
         bool           strict,
         int            dbgWidth,
         bool           trace,
         FILE *         fin,
         FILE *         file_out,
         FILE *         file_err) {
    (void) dbgWidth;
    if (initCells <= 0) {
        initCells = 30000;
    }
    std::vector<std::uint8_t> tape(static_cast<std::size_t>(initCells), 0);
    std::size_t               ptr  = 0;
    auto                      grow = [&] {
        std::size_t newSize = std::max(tape.size() * 2, tape.size() + static_cast<std::size_t>(1));
        tape.resize(newSize, 0);
    };

    auto cell = [&] () -> std::uint8_t & {
        return tape[ptr];
    };

    for (int pc = 0; pc < static_cast<int>(p.code.size()); ++pc) {
        const auto &ins = p.code[pc];
        if (trace) {
            std::fprintf(file_err,
                         "pc=%d op=%d arg=%d ptr=%zu cell=%u\n",
                         pc,
                         static_cast<int>(ins.op),
                         ins.arg,
                         ptr,
                         static_cast<unsigned>(cell()));
        }
        switch (ins.op) {
            case Op::INC_PTR:
                for (int n = 0; n < ins.arg; ++n) {
                    if (ptr == tape.size() - 1) {
                        if (elastic) {
                            grow();
                            ++ptr;
                        } else if (strict) {
                            die("ptr overflow");
                        } else {
                            // clamp
                        }
                    } else {
                        ++ptr;
                    }
                }
                break;
            case Op::DEC_PTR:
                for (int n = 0; n < ins.arg; ++n) {
                    if (ptr == 0) {
                        if (strict) {
                            die("ptr underflow");
                        }
                        // clamp
                    } else {
                        --ptr;
                    }
                }
                break;
            case Op::INC:
                cell() = static_cast<std::uint8_t>((cell() + ins.arg) & 0xFF);
                break;
            case Op::DEC:
                cell() = static_cast<std::uint8_t>((static_cast<int>(cell()) - ins.arg) & 0xFF);
                break;
            case Op::OUT:
                for (int n = 0; n < ins.arg; ++n) {
                    std::fputc(cell(), file_out);
                }
                break;
            case Op::IN:
                for (int n = 0; n < ins.arg; ++n) {
                    int ch = std::fgetc(fin);
                    if (ch == EOF) {
                        cell() = 255;
                    } else {
                        cell() = static_cast<std::uint8_t>(ch & 0xFF);
                    }
                }
                break;
            case Op::JZ:
                if (cell() == 0) {
                    pc = ins.arg;
                }
                break;
            case Op::JNZ:
                if (cell() != 0) {
                    pc = ins.arg;
                }
                break;
            case Op::ZERO_IF_EOF:
                if (cell() == 255) {
                    cell() = 0;
                }
                break;
            case Op::DBG: {
                std::size_t left  = ptr;
                std::size_t right = std::min(tape.size(), ptr + static_cast<std::size_t>(ins.arg));
                std::fprintf(file_err, "! ptr=%zu cells=[", ptr);
                for (std::size_t i = left; i < right; ++i) {
                    if (i > left) {
                        std::fputc(' ', file_err);
                    }
                    std::fprintf(file_err, "%u", static_cast<unsigned>(tape[i]));
                }
                std::fprintf(file_err, "]\n");
                break;
            }
            case Op::CLEAR:
                cell() = 0;
                break;
        }
    }

    return 0;
}
