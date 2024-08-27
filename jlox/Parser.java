package jlox;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static jlox.TokenType.*;

class Parser {
    private final List<Token> tokens;
    private int current = 0;

    private static class ParseError extends RuntimeException {
    }
    // For C operator precedence and associativity, see
    // https://en.cppreference.com/w/c/language/operator_precedence

    Parser(List<Token> tokens) {
        this.tokens = tokens;
    }

    List<Stmt> parse() {
        try {
            List<Stmt> statements = new ArrayList<>();
            while (!isAtEnd()) {
                statements.add(declaration());
            }
            return statements;
        } catch (ParseError error) {
            return null;
        }
    }

    private Stmt declaration() {
        try {
            if (match(VAR))
                return varDeclaration();
            return statement();
        } catch (ParseError error) {
            synchronize();
            return null;
        }
    }

    private Stmt varDeclaration() {
        Token name = consume(IDENTIFIER, "Expect variable name.");
        Expr initializer = null;
        if (match(EQUAL)) {
            initializer = expression();
        }
        consume(SEMICOLON, "Expect ';' after variable declaration.");
        return new Stmt.Var(name, initializer);
    }

    private Stmt statement() {
        if (match(PRINT))
            return printStatement();
        if (match(LEFT_BRACE))
            return new Stmt.Block(blockStatement());
        if (match(IF))
            return ifStatement();
        if (match(WHILE))
            return whileStatement();
        if (match(FOR))
            return forStatement();
        if (match(ASSERT))
            return assertStatement();
        if (match(FUN)) {
            if (check(IDENTIFIER)) return function("function");
            return lambdaStatement();
        }
        if (match(RETURN))
            return returnStatement();
        if (match(CLASS))
            return classDeclaration();

        return expressionStatement();
    }

    private Stmt printStatement() {
        Expr value = expression();
        consume(SEMICOLON, "Expected ';' after print statement");
        return new Stmt.Print(value);
    }

    private Stmt assertStatement() {
        Token operator = previous();
        Expr condition = expression();
        consume(SEMICOLON, "Expected ';' after assert statement");
        return new Stmt.Assert(operator, condition);
    }

    private List<Stmt> blockStatement() {
        List<Stmt> statements = new ArrayList<>();

        while (!check(RIGHT_BRACE) && !isAtEnd()) {
            statements.add(declaration());
        }

        consume(RIGHT_BRACE, "Expect '}' after block.");
        return statements;
    }

    private Stmt ifStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'if'.");
        Expr condition = expression();
        consume(RIGHT_PAREN, "Expect ')' after 'if' condition.");

        Stmt thenBranch = statement();
        Stmt elseBranch = null;
        if (match(ELSE)) {
            elseBranch = statement();
        }

        return new Stmt.If(condition, thenBranch, elseBranch);
    }

    private Stmt whileStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'while'.");
        Expr condition = expression();
        consume(RIGHT_PAREN, "Expect ')' after 'while' condition.");

        Stmt body = statement();

        return new Stmt.While(condition, body);
    }

    private Stmt forStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'for'.");

        Stmt initializer;
        if (match(SEMICOLON))
            initializer = null;
        else if (match(VAR))
            initializer = varDeclaration();
        else
            initializer = expressionStatement();

        Expr condition = null;
        if (!check(SEMICOLON))
            condition = expression();
        consume(SEMICOLON, "Expect ';' after 'for' condition.");

        Expr increment = null;
        if (!check(RIGHT_PAREN))
            increment = expression();

        consume(RIGHT_PAREN, "Expect ')' after 'for' clauses.");

        Stmt body = statement();

        if (increment != null) {
            body = new Stmt.Block(
                    Arrays.asList(body, new Stmt.Expression(increment)));
        }

        if (condition == null)
            condition = new Expr.Literal(true);

        body = new Stmt.While(condition, body);
        if (initializer != null) {
            body = new Stmt.Block(Arrays.asList(initializer, body));
        }

        return body;
    }

    private Stmt expressionStatement() {
        Expr expr = expression();
        consume(SEMICOLON, "Expect ';' after expression.");
        return new Stmt.Expression(expr);
    }

    private Stmt lambdaStatement() {
        // Ugly solution :
        // to know if its a function declaration or an anonymous function statement, we have to check for the identifier
        // but the lambda() function needs the 'fun' keyword to detect the lambda ;
        // therefore we need to rollback a bit
        if (current == 0) error(peek(), "Lambda function called but no 'fun' keyword found. This should never happen.");
        current--;
        Expr expr = lambda();
        consume(SEMICOLON, "Expect ';' after expression.");
        return new Stmt.Expression(expr);
    }

    private Stmt.Function function(String kind) {
        Token name = consume(IDENTIFIER, "Expect function identifier for kind '" + kind + "'.");
        List<Token> parameters = null;
        
        if (match(LEFT_PAREN)) {
            parameters = new ArrayList<>();
            if (!check(RIGHT_PAREN)) {
                do {
                    if (parameters.size() >= 255) {
                        error(peek(), "Can't have more than 255 parameters.");
                    }
                    parameters.add(
                            consume(IDENTIFIER, "Expect parameter name."));
                } while (match(COMMA));
            }
            consume(RIGHT_PAREN, "Expect ')' after parameters.");
        }

        consume(LEFT_BRACE, "Expect '{' before " + kind + " body.");

        List<Stmt> body = blockStatement();

        return new Stmt.Function(name, parameters, body);
    }

    private Stmt returnStatement() {
        Token keyword = previous();

        Expr value = null;

        if (!check(SEMICOLON)) {
            value = expression();
        }

        consume(SEMICOLON, "Expect ';' after return value.");
        return new Stmt.Return(keyword, value);
    }

    private Stmt classDeclaration() {
        Token name = consume(IDENTIFIER, "Expect class name.");

        consume(LEFT_BRACE, "Expect '{' after class name.");

        List<Stmt.Function> methods = new ArrayList<>();
        List<Stmt.Function> getters = new ArrayList<>();
        List<Stmt.Static> statics = new ArrayList<>();

        while (!check(RIGHT_BRACE) && !isAtEnd()) {
            if (match(STATIC))
                statics.add(staticMethod());
            else {
                Stmt.Function function = function("method");
                if (function.params != null) methods.add(function);
                else getters.add(function);
            }
        }

        consume(RIGHT_BRACE, "Expect '}' after class body.");

        return new Stmt.Class(name, methods, statics, getters);
    }

    private Stmt.Static staticMethod() {
        Stmt.Function method = function("method");
            
        return new Stmt.Static(method);
    }

    private Expr expression() {
        Expr expr = assignment();

        while (match(COMMA)) {
            Token operator = previous();
            Expr right = assignment();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr assignment() {
        Expr expr = ternary();

        if (match(EQUAL)) {
            Token equals = previous();
            Expr value = expression();

            if (expr instanceof Expr.Variable) {
                Token name = ((Expr.Variable) expr).name;
                return new Expr.Assign(name, value);
            } else if (expr instanceof Expr.Get) {
                Expr.Get get = (Expr.Get) expr;
                return new Expr.Set(get.object, get.name, value);
            }
            error(equals, "Invalid assignment target.");
        }

        return expr;
    }

    private Expr ternary() {
        Expr expr = or();

        if (match(QUESTION)) {
            Token operator = previous();
            Expr middle = expression(); // Parsed as if parenthesized
            consume(COLON, "Unfinished ternary operator. Syntax : condition ? value if true : value if false.");
            Expr right = ternary();
            expr = new Expr.Ternary(operator, expr, middle, right);
        }

        return expr;
    }

    private Expr or() {
        Expr expr = and();

        while (match(OR)) {
            Token operator = previous();
            Expr right = and();
            expr = new Expr.Logical(expr, operator, right);
        }

        return expr;
    }

    private Expr and() {
        Expr expr = equality();

        while (match(AND)) {
            Token operator = previous();
            Expr right = equality();
            expr = new Expr.Logical(expr, operator, right);
        }

        return expr;
    }

    private Expr equality() {
        Expr expr = comparison();

        while (match(BANG_EQUAL, EQUAL_EQUAL)) {
            Token operator = previous();
            Expr right = comparison();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr comparison() {
        Expr expr = term();

        while (match(GREATER, GREATER_EQUAL, LESS, LESS_EQUAL)) {
            Token operator = previous();
            Expr right = term();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr term() {
        Expr expr = factor();

        while (match(MINUS, PLUS)) {
            Token operator = previous();
            Expr right = factor();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr factor() {
        Expr expr = unary();

        while (match(SLASH, STAR)) {
            Token operator = previous();
            Expr right = unary();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr unary() {
        if (match(BANG, MINUS)) {
            Token operator = previous();
            Expr right = unary();
            return new Expr.Unary(operator, right);
        }

        return lambda();
    }

    private Expr lambda() {
        if (match(FUN)) {
            consume(LEFT_PAREN, "Expect '(' after lambda.");

            List<Token> parameters = new ArrayList<>();
            if (!check(RIGHT_PAREN)) {
                do {
                    if (parameters.size() >= 255) {
                        error(peek(), "Can't have more than 255 parameters.");
                    }
                    parameters.add(
                            consume(IDENTIFIER, "Expect parameter name."));
                } while (match(COMMA));
            }
            consume(RIGHT_PAREN, "Expect ')' after parameters.");

            consume(LEFT_BRACE, "Expect '{' before lambda body.");

            List<Stmt> body = blockStatement();

            return new Expr.Lambda(parameters, body);
        }

        return call();
    }

    private Expr call() {
        Expr expr = primary();

        while (true) {
            if (match(LEFT_PAREN)) {
                expr = finishCall(expr);
            } else if (match(DOT)) {
                Token name = consume(IDENTIFIER, "Expect attribute or method name after '.'.");
                expr = new Expr.Get(expr, name);
            } else {
                break;
            }
        }

        return expr;
    }

    private Expr primary() {
        if (match(FALSE))
            return new Expr.Literal(false);
        if (match(TRUE))
            return new Expr.Literal(true);
        if (match(NIL))
            return new Expr.Literal(null);

        if (match(NUMBER, STRING))
            return new Expr.Literal(previous().literal);

        if (match(LEFT_PAREN)) {
            Expr expr = expression();
            consume(RIGHT_PAREN, "Missing closing parenthesis ')'.");
            return new Expr.Grouping(expr);
        }

        if (match(THIS)) {
            return new Expr.This(previous());
        }

        if (match(IDENTIFIER)) {
            return new Expr.Variable(previous());
        }

        throw error(peek(), "Expected expression, got nothing or an unknown token.");
    }

    private void synchronize() {
        advance();

        while (!isAtEnd()) {
            if (previous().type == SEMICOLON)
                return;

            switch (peek().type) {
                // keywords at the start of statements
                case CLASS:
                case FUN:
                case VAR:
                case FOR:
                case IF:
                case WHILE:
                case PRINT:
                case ASSERT:
                case RETURN:
                    return;
                default:
                    break;
            }

            advance();
        }
    }

    private boolean match(TokenType... types) {
        for (TokenType type : types) {
            if (check(type)) {
                advance();
                return true;
            }
        }

        return false;
    }

    private boolean check(TokenType type) {
        if (isAtEnd())
            return false;
        return peek().type == type;
    }

    private Token advance() {
        if (!isAtEnd())
            current++;
        return previous();
    }

    private boolean isAtEnd() {
        return peek().type == EOF;
    }

    private Token peek() {
        return tokens.get(current);
    }

    private Token previous() {
        return tokens.get(current - 1);
    }

    private Token consume(TokenType type, String message) {
        if (check(type))
            return advance();
        throw error(peek(), message);
    }

    private ParseError error(Token token, String message) {
        Lox.error(token, message);
        return new ParseError();
    }

    private Expr finishCall(Expr callee) {
        List<Expr> arguments = new ArrayList<>();
        if (!check(RIGHT_PAREN)) {
            do {
                if (arguments.size() > 255) {
                    error(peek(), "Can't have more than 255 arguments to a function call.");
                }
                arguments.add(assignment());
            } while (match(COMMA));
        }

        Token token = consume(RIGHT_PAREN, "Expect ')' after arguments to the function call.");

        return new Expr.Call(callee, token, arguments);
    }
}