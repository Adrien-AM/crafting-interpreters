#include "scanner.h"

#include <stdio.h>
#include <string.h>

#include "common.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

int isAtEnd() { return *scanner.current == '\0'; }

static char peekNext() {
    if (isAtEnd())
        return '\0';
    return scanner.current[1];
}

static char advance() { return *(scanner.current++); }

static bool match(char c) {
    if (isAtEnd())
        return false;
    if (*scanner.current == c) {
        advance();
        return true;
    }
    return false;
}

static bool isDigit(char c) { return c >= '0' && c <= '9'; }

static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static void skipWhiteSpace() {
    for (;;) {
        char c = *scanner.current;
        switch (c) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
            case '\n': scanner.line++;
#pragma GCC diagnostic pop
            case ' ':
            case '\r':
            case '\t': advance(); break;
            case '/':
                if (peekNext() == '/') {
                    while (*scanner.current != '\n' && !isAtEnd())
                        advance();
                } else {
                    return;
                }
                break;
            default: return;
        }
    }
}

Token makeToken(TokenType type) {
    Token token = {.start = scanner.start,
                   .line = scanner.line,
                   .length = (int)(scanner.current - scanner.start),
                   .type = type};

    return token;
}

Token errorToken(char* message) {
    Token token = {
        .start = message,
        .line = scanner.line,
        .length = (int)strlen(message),
        .type = TOKEN_ERROR,
    };

    return token;
}

static Token string() {
    while (*scanner.current != '"' && !isAtEnd()) {
        if (*scanner.current == '\n')
            scanner.line++;
        advance();
    }

    if (isAtEnd())
        return errorToken("Unterminated string.");

    advance();
    return makeToken(TOKEN_STRING);
}

static Token number() {
    while (isDigit(*scanner.current))
        advance();
    // Look for a fractional part.
    if (*scanner.current == '.' && isDigit(peekNext())) {
        // Consume the ".".
        advance();
        while (isDigit(*scanner.current))
            advance();
    }
    return makeToken(TOKEN_NUMBER);
}

static TokenType checkKeyword(int start, int length, const char* rest,
                              TokenType type) {
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
    switch (scanner.start[0]) {
        case 'a':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'n': return checkKeyword(2, 1, "d", TOKEN_AND);
                    case 's': return checkKeyword(2, 4, "sert", TOKEN_ASSERT);
                }
            }
            break;
        case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
        case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
        case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
        case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
        case 't':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}

static Token identifier() {
    while (isAlpha(*scanner.current) || isDigit(*scanner.current))
        advance();
    return makeToken(identifierType());
}

Token scanToken() {
    skipWhiteSpace();
    scanner.start = scanner.current;

    if (isAtEnd())
        return makeToken(TOKEN_EOF);

    char c = advance();

    if (isAlpha(c))
        return identifier();
    if (isDigit(c))
        return number();

    switch (c) {
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '-': return makeToken(TOKEN_MINUS);
        case '+': return makeToken(TOKEN_PLUS);
        case '/': return makeToken(TOKEN_SLASH);
        case '*': return makeToken(TOKEN_STAR);
        case '?': return makeToken(TOKEN_QUESTION);
        case ':': return makeToken(TOKEN_COLON);
        case '!': return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<': return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '\n':
            scanner.line++;
            advance();
            break;
        case '"': return string();
    }

    return errorToken("Unexpected character.");
}