#include "error.h"

#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace ffs {
    void ErrorReporter::fatal (const ErrorInfo &error) {
        printError(error);
        std::exit(1);
    }

    void ErrorReporter::fatal (const std::string &message) {
        ErrorInfo error(ErrorCategory::INTERNAL, ErrorCode::INTERNAL_ERROR, message);
        fatal(error);
    }

    void ErrorReporter::syntaxError (ErrorCode             code, const std::string &message,
                                     const SourceLocation &loc, const std::string & suggestion) {
        ErrorInfo error(ErrorCategory::SYNTAX, code, message);
        error.location   = loc;
        error.suggestion = suggestion;
        fatal(error);
    }

    void ErrorReporter::runtimeError (ErrorCode          code, const std::string &   message,
                                      const std::string &context, const std::string &suggestion) {
        ErrorInfo error(ErrorCategory::RUNTIME, code, message);
        error.context    = context;
        error.suggestion = suggestion;
        fatal(error);
    }

    void ErrorReporter::argumentError (ErrorCode          code, const std::string &message,
                                       const std::string &suggestion) {
        ErrorInfo error(ErrorCategory::ARGUMENT, code, message);
        error.suggestion = suggestion;
        fatal(error);
    }

    void ErrorReporter::ioError (ErrorCode          code, const std::string &    message,
                                 const std::string &filename, const std::string &suggestion) {
        ErrorInfo error(ErrorCategory::IO, code, message);
        if (!filename.empty()) {
            error.context = "File: " + filename;
        }
        error.suggestion = suggestion;
        fatal(error);
    }

    void ErrorReporter::printError (const ErrorInfo &error) {
        bool useColor = supportsColor();

        // Error header with category and code
        if (useColor) {
            printWithColor("error", "31"); // Red
            std::cerr << "[" << getCategoryName(error.category) << ":" << getErrorCodeName(error.code) << "]: ";
        } else {
            std::cerr << "error[" << getCategoryName(error.category) << ":" << getErrorCodeName(error.code) << "]: ";
        }

        std::cerr << error.message << std::endl;

        // Source location if available
        if (error.location.has_value()) {
            const auto &loc = error.location.value();
            std::cerr << "  ";
            if (useColor) {
                printWithColor("-->", "36"); // Cyan
            } else {
                std::cerr << "-->";
            }

            if (!loc.filename.empty()) {
                std::cerr << " " << loc.filename;
            } else {
                std::cerr << " input";
            }

            if (loc.line > 0) {
                std::cerr << ":" << loc.line;
                if (loc.column > 0) {
                    std::cerr << ":" << loc.column;
                }
            } else if (loc.position > 0) {
                std::cerr << " at position " << loc.position;
            }
            std::cerr << std::endl;
        }

        // Additional context
        if (!error.context.empty()) {
            std::cerr << "  ";
            if (useColor) {
                printWithColor("note:", "34"); // Blue
            } else {
                std::cerr << "note:";
            }
            std::cerr << " " << error.context << std::endl;
        }

        // Helpful suggestion
        if (!error.suggestion.empty()) {
            std::cerr << "  ";
            if (useColor) {
                printWithColor("help:", "32"); // Green
            } else {
                std::cerr << "help:";
            }
            std::cerr << " " << error.suggestion << std::endl;
        }

        std::cerr << std::endl;
    }

    std::string ErrorReporter::getErrorCodeName (ErrorCode code) {
        switch (code) {
            case ErrorCode::UNMATCHED_BRACKET:
                return "unmatched-bracket";
            case ErrorCode::MISMATCHED_LABELS:
                return "mismatched-labels";
            case ErrorCode::INVALID_NUMBER_FORMAT:
                return "invalid-number";
            case ErrorCode::EMPTY_NUMBER:
                return "empty-number";
            case ErrorCode::POINTER_OVERFLOW:
                return "pointer-overflow";
            case ErrorCode::POINTER_UNDERFLOW:
                return "pointer-underflow";
            case ErrorCode::MEMORY_LIMIT_EXCEEDED:
                return "memory-limit";
            case ErrorCode::INVALID_JUMP_TARGET:
                return "invalid-jump";
            case ErrorCode::FILE_NOT_FOUND:
                return "file-not-found";
            case ErrorCode::FILE_READ_ERROR:
                return "file-read-error";
            case ErrorCode::INVALID_ARGUMENT_VALUE:
                return "invalid-value";
            case ErrorCode::MISSING_ARGUMENT_VALUE:
                return "missing-value";
            case ErrorCode::UNKNOWN_ARGUMENT:
                return "unknown-argument";
            case ErrorCode::OUT_OF_RANGE:
                return "out-of-range";
            case ErrorCode::INTERNAL_ERROR:
                return "internal";
            default:
                return "unknown";
        }
    }

    std::string ErrorReporter::getCategoryName (ErrorCategory category) {
        switch (category) {
            case ErrorCategory::SYNTAX:
                return "syntax";
            case ErrorCategory::RUNTIME:
                return "runtime";
            case ErrorCategory::IO:
                return "io";
            case ErrorCategory::ARGUMENT:
                return "argument";
            case ErrorCategory::INTERNAL:
                return "internal";
            default:
                return "unknown";
        }
    }

    bool ErrorReporter::supportsColor () {
#ifdef _WIN32
        // Windows 10 version 1511 and later support ANSI escape sequences
        HANDLE hOut = GetStdHandle(STD_ERROR_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE)
            return false;

        DWORD mode;
        if (!GetConsoleMode(hOut, &mode))
            return false;

        return SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
        // Check if stderr is a terminal
        return isatty(STDERR_FILENO);
#endif
    }

    void ErrorReporter::printWithColor (const std::string &text, const std::string &color) {
        std::cerr << "\033[" << color << "m" << text << "\033[0m";
    }
} // namespace ffs
