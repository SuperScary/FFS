#include "vm.h"

#include "util.h"
#include "error.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

int run(const Program &p,
        int initCells,
        bool elastic,
        bool strict,
        int dbgWidth,
        bool trace,
        FILE *fin,
        FILE *file_out,
        FILE *file_err)
{
    if (initCells <= 0)
    {
        initCells = 30000;
    }
    std::vector<std::uint8_t> tape(static_cast<std::size_t>(initCells), 0);
    std::size_t ptr = 0;
    constexpr std::size_t MAX_TAPE_SIZE = 1024 * 1024; // 1MB limit

    // Infinite loop detection
    std::uint64_t instructionCount = 0;
    constexpr std::uint64_t MAX_INSTRUCTIONS = 10000000; // 10 million instructions

    auto grow = [&]
    {
        if (tape.size() >= MAX_TAPE_SIZE)
        {
            ffs::ErrorReporter::runtimeError(ffs::ErrorCode::MEMORY_LIMIT_EXCEEDED,
                                             "Memory limit of " + std::to_string(MAX_TAPE_SIZE) + " cells exceeded",
                                             "Current memory usage: " + std::to_string(tape.size()) + " cells",
                                             "Consider using fewer cells or optimizing your program");
        }
        std::size_t newSize = std::min(MAX_TAPE_SIZE, std::max(tape.size() * 2, tape.size() + 1));
        tape.resize(newSize, 0);
    };

    auto cell = [&]() -> std::uint8_t &
    {
        return tape[ptr];
    };

    auto validateJump = [&](int jumpTarget) -> bool
    {
        return jumpTarget >= 0 && jumpTarget < static_cast<int>(p.code.size());
    };

    for (int pc = 0; pc < static_cast<int>(p.code.size()); ++pc)
    {
        // Check for infinite loop
        ++instructionCount;
        if (instructionCount > MAX_INSTRUCTIONS)
        {
            ffs::ErrorReporter::runtimeError(ffs::ErrorCode::INTERNAL_ERROR,
                                             "Infinite loop detected",
                                             "Executed " + std::to_string(instructionCount) + " instructions",
                                             "Check your loop conditions and ensure they can terminate");
            return 1;
        }

        constexpr std::uint8_t EOF_VALUE = 255;
        const auto &ins = p.code[pc];
        if (trace)
        {
            std::fprintf(file_err,
                         "pc=%d op=%d arg=%d ptr=%zu cell=%u (count=%llu)\n",
                         pc,
                         static_cast<int>(ins.op),
                         ins.arg,
                         ptr,
                         static_cast<unsigned>(cell()),
                         instructionCount);
        }
        switch (ins.op)
        {
        case Op::INC_PTR:
            for (int n = 0; n < ins.arg; ++n)
            {
                if (ptr >= tape.size() - 1)
                {
                    if (elastic)
                    {
                        grow();
                        if (ptr < tape.size() - 1)
                        {
                            ++ptr;
                        }
                        else if (strict)
                        {
                            ffs::ErrorReporter::runtimeError(ffs::ErrorCode::POINTER_OVERFLOW,
                                                             "Pointer overflow after memory growth",
                                                             "Attempted to access position " + std::to_string(
                                                                                                   ptr + tape.size()),
                                                             "Ensure your pointer movements don't exceed available memory");
                        }
                    }
                    else if (strict)
                    {
                        ffs::ErrorReporter::runtimeError(ffs::ErrorCode::POINTER_OVERFLOW,
                                                         "Pointer moved beyond available memory",
                                                         "Attempted to access position " + std::to_string(ptr),
                                                         "Use '<' to move the pointer back or ensure adequate memory");
                    }
                    else
                    {
                        // clamp - do nothing
                    }
                }
                else
                {
                    ++ptr;
                }
            }
            break;
        case Op::DEC_PTR:
            for (int n = 0; n < ins.arg; ++n)
            {
                if (ptr == 0)
                {
                    if (strict)
                    {
                        ffs::ErrorReporter::runtimeError(ffs::ErrorCode::POINTER_UNDERFLOW,
                                                         "Pointer moved below zero",
                                                         "Attempted to access negative position " + std::to_string(
                                                                                                        ptr),
                                                         "Use '>' to move the pointer forward or check your pointer movements");
                    }
                    // clamp
                }
                else
                {
                    --ptr;
                }
            }
            break;
        case Op::INC:
            cell() = static_cast<std::uint8_t>((cell() + ins.arg) & 0xFF);
            break;
        case Op::DEC:
            // Handle potential underflow properly by ensuring we stay in uint8_t range
            if (ins.arg <= static_cast<int>(cell()))
            {
                cell() = static_cast<std::uint8_t>(cell() - ins.arg);
            }
            else
            {
                // Wrap around (standard behavior)
                cell() = static_cast<std::uint8_t>(256 + static_cast<int>(cell()) - ins.arg);
            }
            break;
        case Op::OUT:
            for (int n = 0; n < ins.arg; ++n)
            {
                std::fputc(cell(), file_out);
            }
            break;
        case Op::IN:
            for (int n = 0; n < ins.arg; ++n)
            {
                int ch = std::fgetc(fin);
                if (ch == EOF)
                {
                    cell() = EOF_VALUE;
                }
                else
                {
                    cell() = static_cast<std::uint8_t>(ch & 0xFF);
                }
            }
            break;
        case Op::JZ:
            if (cell() == 0)
            {
                if (!validateJump(ins.arg))
                {
                    ffs::ErrorReporter::runtimeError(ffs::ErrorCode::INVALID_JUMP_TARGET,
                                                     "Invalid jump target in JZ instruction",
                                                     "Jump target: " + std::to_string(ins.arg) + ", program size: " + std::to_string(p.code.size()),
                                                     "This indicates a compiler bug - please report this issue");
                }
                pc = ins.arg;
            }
            break;
        case Op::JNZ:
            if (cell() != 0)
            {
                if (!validateJump(ins.arg))
                {
                    ffs::ErrorReporter::runtimeError(ffs::ErrorCode::INVALID_JUMP_TARGET,
                                                     "Invalid jump target in JNZ instruction",
                                                     "Jump target: " + std::to_string(ins.arg) + ", program size: " + std::to_string(p.code.size()),
                                                     "This indicates a compiler bug - please report this issue");
                }
                pc = ins.arg;
            }
            break;
        case Op::ZERO_IF_EOF:
            if (cell() == EOF_VALUE)
            {
                cell() = 0;
            }
            break;
        case Op::DBG:
        {
            std::size_t left = ptr;
            std::size_t right = std::min(tape.size(), ptr + static_cast<std::size_t>(dbgWidth));
            std::fprintf(file_err, "! ptr=%zu cells=[", ptr);
            for (std::size_t i = left; i < right; ++i)
            {
                if (i > left)
                {
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
