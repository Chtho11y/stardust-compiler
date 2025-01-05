#include "literal_process.h"

#include <string>
#include <stdexcept>
#include <sstream>
#include <cctype>
#include <iostream>

// Parse double literals, supporting decimal and scientific notation.
double parse_double(const std::string &literal) {
    return std::stod(literal);
}

// Parse integer literals, supporting decimal, hexadecimal, and binary.
size_t parse_int(const std::string &literal) {
    if (literal.empty()) {
        throw std::invalid_argument("Empty integer literal.");
    }

    size_t idx = 0;
    int base = 10;
    if (literal.size() > 2 && (literal[0] == '0' && (literal[1] == 'x' || literal[1] == 'X'))) {
        base = 16;
        idx = 2;
    } else if (literal.size() > 2 && (literal[0] == '0' && (literal[1] == 'b' || literal[1] == 'B'))) {
        base = 2;
        idx = 2;
    }

    try {
        return std::stoll(literal.substr(idx), nullptr, base);
    } catch (const std::exception &e) {
        throw std::invalid_argument("Invalid integer literal: " + literal);
    }
}

// Parse string literals, supporting escape sequences and surrounding quotes.
std::string parse_string(const std::string &literal) {
    if (literal.size() < 2 || literal.front() != '"' || literal.back() != '"') {
        throw std::invalid_argument("Invalid string literal: " + literal);
    }

    std::string result;
    for (size_t i = 1; i < literal.size() - 1; ++i) {
        if (literal[i] == '\\') {
            if (i + 1 >= literal.size() - 1) {
                throw std::invalid_argument("Incomplete escape sequence in string literal: " + literal);
            }

            char escape = literal[++i];
            switch (escape) {
                case 'n': result.push_back('\n'); break;
                case 't': result.push_back('\t'); break;
                case 'b': result.push_back('\b'); break;
                case 'f': result.push_back('\f'); break;
                case 'r': result.push_back('\r'); break;
                case '\\': result.push_back('\\'); break;
                case '0': result.push_back('\0'); break;
                case '"': result.push_back('"'); break;
                default:
                    result.push_back('\\');
                    i--;
                    // throw std::invalid_argument("Invalid escape character in string literal: " + literal);
            }
        } else {
            result.push_back(literal[i]);
        }
    }
    return result;
}

// Parse char literals, supporting escape sequences and surrounding single quotes.
char parse_char(const std::string &literal) {
    if (literal.size() < 3 || literal.front() != '\'' || literal.back() != '\'') {
        throw std::invalid_argument("Invalid char literal: " + literal);
    }

    if (literal[1] == '\\') {
        if (literal.size() != 4) {
            throw std::invalid_argument("Invalid escape sequence in char literal: " + literal);
        }

        switch (literal[2]) {
            case 'n': return '\n';
            case 't': return '\t';
            case 'b': return '\b';
            case 'f': return '\f';
            case 'r': return '\r';
            case '\\': return '\\';
            case '\'': return '\'';
            case '0': return '\0';
            default:
                throw std::invalid_argument("Invalid escape character in char literal: " + literal);
        }
    } else {
        if (literal.size() != 3) {
            throw std::invalid_argument("Char literal must contain exactly one character: " + literal);
        }
        return literal[1];
    }
}