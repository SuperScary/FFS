#include "compiler.h"
#include "error.h"

#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace
{
    // Helper to calculate line and column from position
    ffs::SourceLocation calculateLocation(const std::string &source, size_t position,
                                          const std::string &filename = "")
    {
        size_t line = 1;
        size_t column = 1;

        for (size_t i = 0; i < position && i < source.size(); ++i)
        {
            if (source[i] == '\n')
            {
                line++;
                column = 1;
            }
            else
            {
                column++;
            }
        }

        return ffs::SourceLocation(line, column, position, filename);
    }

    // Strip comments and normalize source prior to tokenization
    std::string strip_comments(const std::string &s)
    {
        std::string out;
        out.reserve(s.size());
        bool inBlock = false;

        for (size_t i = 0; i < s.size(); ++i)
        {
            if (!inBlock && s[i] == '#')
            {
                while (i < s.size() && s[i] != '\n')
                {
                    ++i;
                }
                if (i < s.size())
                {
                    out.push_back('\n');
                }
                continue;
            }
            if (!inBlock && i + 1 < s.size() && s[i] == '/' && s[i + 1] == '*')
            {
                inBlock = true;
                ++i;
                continue;
            }
            if (inBlock && i + 1 < s.size() && s[i] == '*' && s[i + 1] == '/')
            {
                inBlock = false;
                ++i;
                continue;
            }
            if (!inBlock)
            {
                out.push_back(s[i]);
            }
        }

        return out;
    }

    bool is_ident(char32_t c)
    {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-';
    }

    int parse_number(const std::string &s, const std::string &filename = "", size_t position = 0)
    {
        try
        {
            unsigned long val;
            if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0)
            {
                if (s.length() <= 2)
                {
                    ffs::SourceLocation loc(1, 1, position, filename);
                    ffs::ErrorReporter::syntaxError(ffs::ErrorCode::EMPTY_NUMBER,
                                                    "Hexadecimal number missing digits after '0x'",
                                                    loc,
                                                    "Add hex digits after '0x', e.g., '0xFF' or '0x42'");
                }
                val = std::stoul(s.substr(2), nullptr, 16);
            }
            else if (s.rfind('b', 0) == 0 || s.rfind('B', 0) == 0)
            {
                if (s.length() <= 1)
                {
                    ffs::SourceLocation loc(1, 1, position, filename);
                    ffs::ErrorReporter::syntaxError(ffs::ErrorCode::EMPTY_NUMBER,
                                                    "Binary number missing digits after 'b'",
                                                    loc,
                                                    "Add binary digits after 'b', e.g., 'b1010' or 'B101'");
                }
                val = std::stoul(s.substr(1), nullptr, 2);
            }
            else
            {
                val = std::stoul(s, nullptr, 10);
            }

            if (val > 255)
            {
                ffs::SourceLocation loc(1, 1, position, filename);
                ffs::ErrorReporter::syntaxError(ffs::ErrorCode::OUT_OF_RANGE,
                                                "Number " + std::to_string(val) + " exceeds byte range (0-255)",
                                                loc,
                                                "Use a number between 0 and 255, or consider using multiple cells");
            }
            return static_cast<int>(val);
        }
        catch (const std::exception &)
        {
            ffs::SourceLocation loc(1, 1, position, filename);
            ffs::ErrorReporter::syntaxError(ffs::ErrorCode::INVALID_NUMBER_FORMAT,
                                            "Invalid number format: " + s,
                                            loc,
                                            "Use decimal (123), hex (0xFF), or binary (b1010) format");
        }
    }

    std::vector<Instr> desugar(const std::string &src, int dbgWidth, const std::string &filename = "")
    {
        std::vector<Instr> code;
        auto skipws = [&](size_t &i)
        {
            while (i < src.size() && std::isspace(static_cast<unsigned char>(src[i])))
            {
                ++i;
            }
        };

        for (size_t i = 0; i < src.size();)
        {
            skipws(i);
            if (i >= src.size())
            {
                break;
            }
            char c = src[i];

            auto parseRepeat = [&](size_t &j) -> int
            {
                if (j < src.size() && src[j] == 'x')
                {
                    size_t k = j + 1;
                    while (k < src.size() && std::isdigit(static_cast<unsigned char>(src[k])))
                    {
                        ++k;
                    }
                    if (k > j + 1)
                    {
                        int n = std::stoi(src.substr(j + 1, k - (j + 1)));
                        j = k;
                        return n;
                    }
                }
                return 1;
            };

            if (c == '>' || c == '<' || c == '+' || c == '-' || c == '.' || c == ',' || c == '[' || c == ']' ||
                c == '?' || c == '!')
            {
                if ((c == '[' || c == ']') && i + 1 < src.size() && src[i + 1] == '@')
                {
                    size_t j = i + 2;
                    while (j < src.size() && is_ident(src[j]))
                    {
                        ++j;
                    }
                    std::string name = src.substr(i + 2, j - (i + 2));
                    if (name.empty())
                    {
                        ++i;
                        continue;
                    }
                    Instr ins;
                    ins.op = (c == '[') ? Op::JZ : Op::JNZ;
                    ins.label = name;
                    code.push_back(ins);
                    i = j;
                    continue;
                }

                Instr ins;
                switch (c)
                {
                case '>':
                    ins.op = Op::INC_PTR;
                    break;
                case '<':
                    ins.op = Op::DEC_PTR;
                    break;
                case '+':
                    ins.op = Op::INC;
                    break;
                case '-':
                    ins.op = Op::DEC;
                    break;
                case '.':
                    ins.op = Op::OUT;
                    break;
                case ',':
                    ins.op = Op::IN;
                    break;
                case '[':
                    ins.op = Op::JZ;
                    break;
                case ']':
                    ins.op = Op::JNZ;
                    break;
                case '?':
                    ins.op = Op::ZERO_IF_EOF;
                    break;
                case '!':
                    ins.op = Op::DBG;
                    ins.arg = dbgWidth;
                    break;
                default:
                    break;
                }
                size_t j = i + 1;
                int rep = parseRepeat(j);
                for (int k = 0; k < rep; ++k)
                {
                    code.push_back(ins);
                }
                i = j;
                continue;
            }

            if (c == '=')
            {
                size_t j = i + 1;
                auto isNum = [&](char ch)
                {
                    return std::isdigit(static_cast<unsigned char>(ch)) || ch == 'x' || ch == 'X' || ch == 'b' ||
                           ch == 'B' || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
                };
                while (j < src.size() && isNum(src[j]))
                {
                    ++j;
                }
                std::string num = src.substr(i + 1, j - (i + 1));
                if (!num.empty())
                {
                    code.push_back({Op::CLEAR, 0, ""});
                    code.push_back({Op::INC, parse_number(num, filename, i + 1), ""});
                }
                i = j;
                continue;
            }

            if (c == ':')
            {
                size_t j = i + 1;
                while (j < src.size() && is_ident(src[j]))
                {
                    ++j;
                }
                i = j;
                continue;
            }

            ++i;
        }

        return code;
    }

    void link_jumps(std::vector<Instr> &code)
    {
        struct Frame
        {
            int pc;
            std::string tag;
        };

        std::vector<Frame> st;
        for (int i = 0; i < static_cast<int>(code.size()); ++i)
        {
            if (code[i].op == Op::JZ)
            {
                st.push_back({i, code[i].label});
            }
            else if (code[i].op == Op::JNZ)
            {
                if (st.empty())
                {
                    ffs::ErrorReporter::syntaxError(ffs::ErrorCode::UNMATCHED_BRACKET,
                                                    "Found ']' without matching '['",
                                                    {},
                                                    "Add a '[' before this ']' or remove the extra ']'");
                }
                auto top = st.back();
                st.pop_back();
                if (top.tag != code[i].label)
                {
                    if (!(top.tag.empty() && code[i].label.empty()))
                    {
                        ffs::ErrorReporter::syntaxError(ffs::ErrorCode::MISMATCHED_LABELS,
                                                        "Mismatched labels between '[" + top.tag + "]' and '[" + code[i].label + "]'",
                                                        {},
                                                        "Make sure labeled brackets match: [name] ... ]name");
                    }
                }
                code[top.pc].arg = i;
                code[i].arg = top.pc;
            }
        }
        if (!st.empty())
        {
            ffs::ErrorReporter::syntaxError(ffs::ErrorCode::UNMATCHED_BRACKET,
                                            "Found '[' without matching ']'",
                                            {},
                                            "Add a ']' to close this '[' or remove the extra '['");
        }
    }
} // namespace

Program compile_src(const std::string &raw, int dbgWidth, const std::string &filename)
{
    std::string noCom = strip_comments(raw);
    auto code = desugar(noCom, dbgWidth, filename);
    link_jumps(code);
    return Program{std::move(code)};
}
