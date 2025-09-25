#include <bits/stdc++.h>
using namespace std;

enum class Op {
    INC_PTR, DEC_PTR, INC, DEC, OUT, IN, JZ, JNZ, ZERO_IF_EOF, DBG, CLEAR
};

struct Instr {
    Op     op;
    int    arg = 1;
    string label;
};

struct Program {
    vector<Instr> code;
};

static void die (const string &msg) {
    fprintf(stderr, "FFS: %s\n", msg.c_str());
    exit(1);
}

static string read_all (istream &in) {
    ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Strip comments and shit
static string strip_comments (const string &s) {
    string out;
    out.reserve(s.size());
    bool inBlock = false;

    for (size_t i = 0; i < s.size(); ++i) {
        if (!inBlock && s[i] == '#') {
            while (i < s.size() && s[i] != '\n') ++i;
            if (i < s.size()) out.push_back('\n');
            continue;
        }
        if (!inBlock && i + 1 < s.size() && s[i] == '/' && s[i + 1] == '*') {
            inBlock = true;
            ++i;
            continue;
        }
        if (inBlock && i + 1 < s.size() && s[i] == '*' && s[i + 1] == '/') {
            inBlock = false;
            ++i;
            continue;
        }
        if (!inBlock) out.push_back(s[i]);
    }

    return out;
}

static bool isIdent (char32_t c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-';
}

static int parse_number (const string &s) {
    // dec, 0x, b...
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
        return static_cast<int>(stoul(s.substr(2), nullptr, 16) & 0xFFu);
    }

    if (s.rfind("b", 0) == 0 || s.rfind("B", 0) == 0) {
        return static_cast<int>(stoul(s.substr(1), nullptr, 2) & 0xFFu);
    }

    return static_cast<int>(stoul(s, nullptr, 10) & 0xFFu);
}

// TOKENIZER + DESUGAR
static vector<Instr> desugar (const string &src, int dbgWidth) {
    vector<Instr> code;
    auto skipws = [&](size_t &i) { while (i < src.size() && isspace(static_cast<unsigned char>(src[i]))) ++i; };

    for (size_t i = 0; i < src.size();) {
        skipws(i);
        if (i >= src.size()) break;
        char c = src[i];

        auto parseRepeat = [&](size_t &j)-> int {
            if (j < src.size() && src[j] == 'x') {
                size_t k = j + 1;
                while (k < src.size() && isdigit(static_cast<unsigned char>(src[k]))) ++k;
                if (k > j + 1) {
                    int n = stoi(src.substr(j + 1, k - (j + 1)));
                    j     = k;
                    return n;
                }
            }
            return 1;
        };

        if (c == '>' || c == '<' || c == '+' || c == '-' || c == '.' || c == ',' || c == '[' || c == ']' || c == '?' ||
            c == '!') {
            // labeled bracket?
            if ((c == '[' || c == ']') && i + 1 < src.size() && src[i + 1] == '@') {
                size_t j = i + 2;
                while (j < src.size() && isIdent(src[j])) ++j;
                string name = src.substr(i + 2, j - (i + 2));
                if (name.empty()) {
                    ++i;
                    continue;
                }
                Instr ins;
                ins.op    = (c == '[') ? Op::JZ : Op::JNZ;
                ins.label = name;
                code.push_back(ins);
                i = j;
                continue;
            }
            // plain op
            Instr ins;
            switch (c) {
                case '>': ins.op = Op::INC_PTR;
                    break;
                case '<': ins.op = Op::DEC_PTR;
                    break;
                case '+': ins.op = Op::INC;
                    break;
                case '-': ins.op = Op::DEC;
                    break;
                case '.': ins.op = Op::OUT;
                    break;
                case ',': ins.op = Op::IN;
                    break;
                case '[': ins.op = Op::JZ;
                    break;
                case ']': ins.op = Op::JNZ;
                    break;
                case '?': ins.op = Op::ZERO_IF_EOF;
                    break;
                case '!': ins.op = Op::DBG;
                    ins.arg = dbgWidth;
                    break;
            }
            size_t j   = i + 1;
            int    rep = parseRepeat(j);
            for (int k = 0; k < rep; ++k) code.push_back(ins);
            i = j;
            continue;
        }

        if (c == '=') {
            // inline constant
            size_t j = i + 1;
            // accept digits, x/X, b/B, hex letters
            auto isNum = [&](char ch) {
                return isdigit(static_cast<unsigned char>(ch)) || ch == 'x' || ch == 'X' || ch == 'b' || ch == 'B' ||
                       (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
            };
            while (j < src.size() && isNum(src[j])) ++j;
            string num = src.substr(i + 1, j - (i + 1));
            if (!num.empty()) {
                int val = parse_number(num);
                code.push_back({Op::CLEAR, 0, ""}); // fast clear
                code.push_back({Op::INC, val, ""}); // add value (VM does mod 256 anyway)
            }
            i = j;
            continue;
        }

        if (c == ':') {
            // label definition (no-op for VM; helps humans)
            size_t j = i + 1;
            while (j < src.size() && isIdent(src[j])) ++j;
            // ignore for now
            i = j;
            continue;
        }

        // ignore anything else (keeps source forgiving)
        ++i;
    }
    return code;
}

static void link_jumps (vector<Instr> &code) {
    struct Frame {
        int    pc;
        string tag;
    };
    vector<Frame> st;
    for (int i = 0; i < (int) code.size(); ++i) {
        if (code[i].op == Op::JZ) {
            // CLEAR is also Op::JZ in some designs; we used Op::CLEAR to avoid ambiguity
            st.push_back({i, code[i].label});
        } else if (code[i].op == Op::JNZ) {
            if (st.empty()) die("unmatched ']'");
            auto top = st.back();
            st.pop_back();
            if (top.tag != code[i].label) {
                // allow unlabeled-to-unlabeled and labeled-to-labeled only
                if (!(top.tag.empty() && code[i].label.empty()))
                    die("mismatched labels between '[' and ']'");
            }
            code[top.pc].arg = i;      // forward target
            code[i].arg      = top.pc; // back target
        }
    }
    if (!st.empty()) die("unmatched '['");
}

// ---------- VM ----------
static int run (const Program &p, int     initCells, bool elastic, bool strict, int dbgWidth, bool trace,
                FILE *         fin, FILE *fout, FILE *    ferr) {
    if (initCells <= 0) initCells = 30000;
    vector<uint8_t> tape((size_t) initCells, 0);
    size_t          ptr  = 0;
    auto            grow = [&] {
        size_t newSize = max(tape.size() * 2, tape.size() + size_t(1));
        tape.resize(newSize, 0);
    };

    auto cell = [&] ()-> uint8_t & { return tape[ptr]; };

    for (int pc = 0; pc < (int) p.code.size(); ++pc) {
        const auto &ins = p.code[pc];
        if (trace) fprintf(ferr, "pc=%d op=%d arg=%d ptr=%zu cell=%u\n", pc, int(ins.op), ins.arg, ptr,
                           (unsigned) cell());
        switch (ins.op) {
            case Op::INC_PTR:
                for (int n = 0; n < ins.arg; ++n) {
                    if (ptr == tape.size() - 1) {
                        if (elastic) {
                            grow();
                            ++ptr;
                        } else if (strict) die("ptr overflow");
                        else {
                            /* clamp */
                        }
                    } else ++ptr;
                }
                break;
            case Op::DEC_PTR:
                for (int n = 0; n < ins.arg; ++n) {
                    if (ptr == 0) { if (strict) die("ptr underflow"); /* else clamp */ } else --ptr;
                }
                break;
            case Op::INC: cell() = uint8_t((cell() + ins.arg) & 0xFF);
                break;
            case Op::DEC: cell() = uint8_t((int(cell()) - ins.arg) & 0xFF);
                break;
            case Op::OUT:
                for (int n = 0; n < ins.arg; ++n) fputc(cell(), fout);
                break;
            case Op::IN:
                for (int n = 0; n < ins.arg; ++n) {
                    int ch = fgetc(fin);
                    if (ch == EOF) cell() = 255;
                    else cell()           = uint8_t(ch & 0xFF);
                }
                break;
            case Op::JZ:
                if (cell() == 0) { pc = ins.arg; }
                // next loop iteration will ++pc, so we want to land on the matching ]
                break;
            case Op::JNZ:
                if (cell() != 0) { pc = ins.arg; }
                break;
            case Op::ZERO_IF_EOF:
                if (cell() == 255) cell() = 0;
                break;
            case Op::DBG: {
                size_t left = ptr, right = min(tape.size(), ptr + size_t(ins.arg));
                fprintf(ferr, "! ptr=%zu cells=[", ptr);
                for (size_t i = left; i < right; ++i) {
                    if (i > left) fputc(' ', ferr);
                    fprintf(ferr, "%u", (unsigned) tape[i]);
                }
                fprintf(ferr, "]\n");
                break;
            }
            case Op::CLEAR:
                cell() = 0;
                break;
        }
    }
    return 0;
}

// ---------- Compile ----------
static Program compile_src (const string &raw, int dbgWidth) {
    string noCom = strip_comments(raw);
    auto   code  = desugar(noCom, dbgWidth);
    link_jumps(code);
    return Program{std::move(code)};
}

// ---------- CLI ----------
int main (int argc, char **argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string file    = "";
    int    cells   = 30000, dbg    = 8;
    bool   elastic = false, strict = false, trace = false;

    // dumb flag parser
    for (int i = 1; i < argc; ++i) {
        string a       = argv[i];
        auto   needVal = [&](const string &name) {
            if (i + 1 >= argc) die("missing value for " + name);
            return string(argv[++i]);
        };
        if (a == "-f" || a == "--file") file = needVal(a);
        else if (a == "--cells") cells = stoi(needVal(a));
        else if (a == "--dbg") dbg = stoi(needVal(a));
        else if (a == "--elastic") elastic = true;
        else if (a == "--strict") strict = true;
        else if (a == "--trace") trace = true;
        else die("unknown flag: " + a);
    }

    string src;
    if (file.empty()) src = read_all(cin);
    else {
        ifstream fin(file, ios::binary);
        if (!fin) die("could not open file: " + file);
        src = read_all(fin);
    }

    Program prog = compile_src(src, dbg);
    return run(prog, cells, elastic, strict, dbg, trace, stdin, stdout, stderr);
}
