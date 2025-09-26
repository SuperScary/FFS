#pragma once

#include <string>
#include <optional>

namespace ffs {
    // Error categories for better organization
    enum class ErrorCategory {
        SYNTAX,   // Source code parsing errors
        RUNTIME,  // VM execution errors
        IO,       // File/input errors
        ARGUMENT, // Command line argument errors
        INTERNAL  // Internal/unexpected errors
    };

    // Specific error codes
    enum class ErrorCode {
        // Syntax errors
        UNMATCHED_BRACKET,
        MISMATCHED_LABELS,
        INVALID_NUMBER_FORMAT,
        EMPTY_NUMBER,

        // Runtime errors
        POINTER_OVERFLOW,
        POINTER_UNDERFLOW,
        MEMORY_LIMIT_EXCEEDED,
        INVALID_JUMP_TARGET,

        // IO errors
        FILE_NOT_FOUND,
        FILE_READ_ERROR,

        // Argument errors
        INVALID_ARGUMENT_VALUE,
        MISSING_ARGUMENT_VALUE,
        UNKNOWN_ARGUMENT,
        OUT_OF_RANGE,

        // Internal errors
        INTERNAL_ERROR
    };

    // Source location for better error reporting
    struct SourceLocation {
        size_t      line     = 0;
        size_t      column   = 0;
        size_t      position = 0;
        std::string filename;

        SourceLocation () = default;

        SourceLocation (size_t pos) : position(pos) {
        }

        SourceLocation (size_t line, size_t col, size_t pos, const std::string &file = "")
            : line(line), column(col), position(pos), filename(file) {
        }
    };

    // Comprehensive error information
    struct ErrorInfo {
        ErrorCategory                 category;
        ErrorCode                     code;
        std::string                   message;
        std::optional<SourceLocation> location;
        std::string                   suggestion; // Helpful hint for fixing the error
        std::string                   context;    // Additional context (e.g., surrounding code)

        ErrorInfo (ErrorCategory cat, ErrorCode code, const std::string &msg)
            : category(cat), code(code), message(msg) {
        }
    };

    // User-friendly error reporting
    class ErrorReporter {
        public:
            // Report an error and exit
            [[noreturn]] static void fatal (const ErrorInfo &error);

            // Report an error with just a message (legacy compatibility)
            [[noreturn]] static void fatal (const std::string &message);

            // Create specific error types with helpful context
            [[noreturn]] static void syntaxError (ErrorCode             code, const std::string &message,
                                                  const SourceLocation &loc        = {},
                                                  const std::string &   suggestion = "");

            [[noreturn]] static void runtimeError (ErrorCode          code, const std::string &message,
                                                   const std::string &context    = "",
                                                   const std::string &suggestion = "");

            [[noreturn]] static void argumentError (ErrorCode          code, const std::string &message,
                                                    const std::string &suggestion = "");

            [[noreturn]] static void ioError (ErrorCode          code, const std::string &message,
                                              const std::string &filename   = "",
                                              const std::string &suggestion = "");

        private:
            static void printError (const ErrorInfo &error);

            static std::string getErrorCodeName (ErrorCode code);

            static std::string getCategoryName (ErrorCategory category);

            static bool supportsColor ();

            static void printWithColor (const std::string &text, const std::string &color);
    };
} // namespace ffs
