#pragma once

#include <string>
#include <unordered_map>

enum class ErrorCode {
    // Compiler Errors
    VariableNotDeclared,
    VariableAlreadyDeclared,
    VariableNotMutable,

    // Syntax Errors
    StringTooLong,
    UnidentifiedToken,

    ExpectedExpression,
    ExpectedOpenParenthesis,
    ExpectedCloseParenthesis,
    ExpectedOpenCurly,
    ExpectedCloseCurly,
    ExpectedScope,
    ExpectedIntegerLiteral,
    ExpectedEndOfLine,
    UnknownOperator,

    // Program Errors
    InvalidProgram,
    InvalidUsage,
    OpenFileError,
};

class ErrorManager {
   public:
    static void error_expected(const ErrorCode error_code,
                               const size_t line_number,
                               const size_t column_number) {
        std::cerr << construct_error_message(error_code, line_number,
                                             column_number);
        exit(EXIT_FAILURE);
    }

    static std::string construct_error_message(ErrorCode code,
                                               const size_t line = 0,
                                               const size_t column = 0) {
        std::ostringstream oss;
        oss << get_error_message(code);

        if (line != 0) oss << ", at line: " << line;

        if (column != 0) oss << ", column: " << column;
        oss << ".\n";

        return oss.str();
    }

    static std::string get_error_message(ErrorCode code) {
        static const std::unordered_map<ErrorCode, std::string> error_messages =
            {
                {ErrorCode::VariableNotDeclared, "Variable is not declared: "},
                {ErrorCode::VariableAlreadyDeclared,
                 "Variable already declared"},
                {ErrorCode::VariableNotMutable, "Variable is not mutable"},

                {ErrorCode::StringTooLong, "Syntax error: string too long"},
                {ErrorCode::UnidentifiedToken,
                 "Syntax error: unidentified token"},

                {ErrorCode::ExpectedExpression,
                 "Syntax error: expected expression"},
                {ErrorCode::ExpectedOpenParenthesis,
                 "Syntax error: expected ("},
                {ErrorCode::ExpectedCloseParenthesis,
                 "Syntax error: expected )"},
                {ErrorCode::ExpectedOpenCurly, "Syntax error: expected {"},
                {ErrorCode::ExpectedCloseCurly, "Syntax error: expected }"},
                {ErrorCode::ExpectedScope, "Syntax error: expected scope"},
                {ErrorCode::ExpectedIntegerLiteral,
                 "Syntax error: expected integer literal"},
                {ErrorCode::ExpectedEndOfLine, "Syntax error: expected ;"},
                {ErrorCode::UnknownOperator, "Syntax error: unknown operator"},

                {ErrorCode::InvalidProgram, "Invalid program"},
                {ErrorCode::InvalidUsage, "Invalid usage"},
                {ErrorCode::OpenFileError, "Error opening file"},
            };

        auto it = error_messages.find(code);
        if (it != error_messages.end()) {
            return it->second;
        } else {
            return "Unknown error";
        }
    }
};
