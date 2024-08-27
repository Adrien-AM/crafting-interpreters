package jlox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

class Interpreter implements Expr.Visitor<Object>,
        Stmt.Visitor<Void> {

    final Environment globals = new Environment(); // root env
    private Environment environment = globals; // current env

    public String lastValueRepr = null;

    private final Map<Expr, Integer> locals = new HashMap<>();

    // Define native functions
    Interpreter() {
        globals.define("clock", new LoxCallable() {
            @Override
            public int arity() {
                return 0;
            }

            @Override
            public Object call(Interpreter interpreter, List<Object> arguments) {
                return (double) System.currentTimeMillis() / 1000.0;
            }

            @Override
            public String toString() {
                return "<native fn clock>";
            }
        });
    }

    void interpret(List<Stmt> statements) {
        try {
            for (Stmt statement : statements) {
                lastValueRepr = null;
                execute(statement);
            }
        } catch (RuntimeError error) {
            Lox.runtimeError(error);
        }
    }

    @Override
    public Void visitExpressionStmt(Stmt.Expression statement) {
        Object value = evaluate(statement.expression);
        lastValueRepr = stringify(value);
        return null;
    }

    @Override
    public Void visitPrintStmt(Stmt.Print statement) {
        Object value = evaluate(statement.expression);
        System.out.println(stringify(value));
        return null;
    }

    @Override
    public Void visitAssertStmt(Stmt.Assert statement) {
        Object value = evaluate(statement.expression);
        if (!isTruthy(value)) {
            throw new RuntimeError(statement.operator, "Assertion error : " + stringify(value) + " is not truthy.");
        }

        return null;
    }

    @Override
    public Void visitBlockStmt(Stmt.Block statement) {
        executeBlock(statement.statements, new Environment(environment));
        return null;
    }

    @Override
    public Void visitVarStmt(Stmt.Var statement) {
        Object value = Environment.UNINITIALIZED;
        if (statement.initializer != null) {
            value = evaluate(statement.initializer);
        }

        environment.define(statement.name.lexeme, value);
        return null;
    }

    @Override
    public Void visitIfStmt(Stmt.If statement) {
        if (isTruthy(evaluate(statement.condition))) {
            execute(statement.thenBranch);
        } else if (statement.elseBranch != null) {
            execute(statement.elseBranch);
        }
        return null;
    }

    @Override
    public Void visitWhileStmt(Stmt.While statement) {
        while (isTruthy(evaluate(statement.condition))) {
            execute(statement.body);
        }

        return null;
    }

    @Override
    public Void visitFunctionStmt(Stmt.Function statement) {
        LoxFunction function = new LoxFunction(statement, environment, false);
        if (statement.name != null)
            environment.define(statement.name.lexeme, function);
        return null;
    }

    @Override
    public Void visitReturnStmt(Stmt.Return stmt) {
        Object value = null;

        if (stmt.value != null)
            value = evaluate(stmt.value);

        throw new Return(value);
    }

    @Override
    public Void visitClassStmt(Stmt.Class stmt) {
        environment.define(stmt.name.lexeme, null);

        Map<String, LoxFunction> methods = new HashMap<>();
        for (Stmt.Function method : stmt.methods) {
            LoxFunction function = new LoxFunction(method, environment, method.name.lexeme.equals("init"));
            methods.put(method.name.lexeme, function);
        }

        Map<String, LoxGetter> getters = new HashMap<>();
        for (Stmt.Function getter : stmt.getters) {
            LoxGetter function = new LoxGetter(getter, environment);
            getters.put(getter.name.lexeme, function);
        }

        Map<String, LoxFunction> statics = new HashMap<>();
        for (Stmt.Static method : stmt.statics) {
            LoxFunction function = new LoxFunction(method.function, globals, false);
            statics.put(method.function.name.lexeme, function);
        }

        LoxClass klass = new LoxClass(stmt.name.lexeme, methods, statics, getters);
        environment.assign(stmt.name, klass);

        return null;
    }

    @Override
    public Void visitStaticStmt(Stmt.Static stmt) {
        return null;
    }

    @Override
    public Object visitLiteralExpr(Expr.Literal expr) {
        return expr.value;
    }

    @Override
    public Object visitGroupingExpr(Expr.Grouping expr) {
        return evaluate(expr.expression);
    }

    @Override
    public Object visitThisExpr(Expr.This expr) {
        return lookUpVariable(expr.keyword, expr);
    }

    @Override
    public Object visitCallExpr(Expr.Call expr) {
        Object callee = evaluate(expr.callee);

        List<Object> arguments = new ArrayList<>();
        for (Expr arg : expr.arguments) {
            arguments.add(evaluate(arg));
        }

        if (!(callee instanceof LoxCallable)) {
            throw new RuntimeError(expr.paren, "Can only call functions or classes.");
        }
        LoxCallable function = (LoxCallable) callee;
        if (arguments.size() != function.arity()) {
            throw new RuntimeError(expr.paren,
                    "Expected " + function.arity() + " arguments, got " + arguments.size() + ".");
        }

        return function.call(this, arguments);
    }

    public Object visitGetExpr(Expr.Get expr) {
        Object object = evaluate(expr.object);

        if (object instanceof LoxInstance) {
            Object value = ((LoxInstance) object).get(expr.name);
            if (value instanceof LoxGetter)
                return ((LoxGetter) value).call(this);
            return value;
        }

        throw new RuntimeError(expr.name, "Getter cannot be called on non-object type.");
    }

    public Object visitSetExpr(Expr.Set expr) {
        Object object = evaluate(expr.object);

        if (object instanceof LoxInstance) {
            Object value = evaluate(expr.value);
            ((LoxInstance) object).set(expr.name, value);
            return value;
        }

        throw new RuntimeError(expr.name, "Cannot assign field of non-object type.");
    }

    public Object visitUnaryExpr(Expr.Unary expr) {
        Object right = evaluate(expr.right);

        switch (expr.operator.type) {
            case MINUS:
                checkNumberOperand(expr.operator, right);
                return -(double) right;
            case BANG:
                return !isTruthy(right);

            default:
                return null;
        }
    }

    @Override
    public Object visitBinaryExpr(Expr.Binary expr) {
        Object left = evaluate(expr.left);
        Object right = evaluate(expr.right);
        switch (expr.operator.type) {
            case MINUS:
                checkNumberOperands(expr.operator, left, right);
                return (double) left - (double) right;
            case SLASH:
                checkNumberOperands(expr.operator, left, right);
                checkNonZero(expr.operator, right);
                return (double) left / (double) right;
            case STAR:
                checkNumberOperands(expr.operator, left, right);
                return (double) left * (double) right;
            case PLUS:
                if (left instanceof Double && right instanceof Double)
                    return (double) left + (double) right;
                if (left instanceof String && right instanceof String)
                    return (String) left + (String) right;
                if (left instanceof String && right instanceof Double)
                    return (String) left + right.toString();
                if (left instanceof Double && right instanceof String)
                    return left.toString() + (String) right;
                throw new RuntimeError(expr.operator, "Operands must be two numbers, or one of them must be a String.");

            case GREATER:
                checkNumberOperands(expr.operator, left, right);
                return (double) left > (double) right;
            case GREATER_EQUAL:
                checkNumberOperands(expr.operator, left, right);
                return (double) left >= (double) right;
            case LESS:
                checkNumberOperands(expr.operator, left, right);
                return (double) left < (double) right;
            case LESS_EQUAL:
                checkNumberOperands(expr.operator, left, right);
                return (double) left <= (double) right;

            case BANG_EQUAL:
                return !isEqual(left, right);
            case EQUAL_EQUAL:
                return isEqual(left, right);

            case COMMA:
                return right;

            default:
                return null;
        }
    }

    @Override
    public Object visitTernaryExpr(Expr.Ternary expr) {
        Object left = evaluate(expr.left);
        Object middle = evaluate(expr.middle);
        Object right = evaluate(expr.right);

        switch (expr.operator.type) {
            case QUESTION:
                if ((Boolean) left) {
                    return middle;
                } else {
                    return right;
                }
            default:
                return null;
        }
    }

    public Object visitLogicalExpr(Expr.Logical expr) {
        Object left = evaluate(expr.left);

        // Lazy evaluation / Short-circuit here
        if (expr.operator.type == TokenType.OR) {
            if (isTruthy(left))
                return left;
        } else {
            if (!isTruthy(left))
                return left;
        }

        return evaluate(expr.right);
    }

    public Object visitVariableExpr(Expr.Variable expr) {
        return lookUpVariable(expr.name, expr);
    }

    public Object visitAssignExpr(Expr.Assign expr) {
        Object value = evaluate(expr.value);

        Integer distance = locals.get(expr);
        if (distance != null) {
            environment.assignAt(distance, expr.name, value);
        } else {
            environment.assign(expr.name, value);
        }

        return value;
    }

    @Override
    public Object visitLambdaExpr(Expr.Lambda expr) {
        return new LoxLambda(expr, environment);
    }

    private Void execute(Stmt stmt) {
        return stmt.accept(this);
    }

    void executeBlock(List<Stmt> statements, Environment environment) {
        Environment previous = this.environment;
        try {
            this.environment = environment;
            for (Stmt stmt : statements) {
                execute(stmt);
            }
        } finally {
            this.environment = previous;
        }
    }

    private Object evaluate(Expr expr) {
        return expr.accept(this);
    }

    private boolean isTruthy(Object object) {
        if (object == null)
            return false;
        if (object instanceof Boolean)
            return (boolean) object;
        return true;
    }

    private boolean isEqual(Object a, Object b) {
        if (a == null && b == null)
            return true;
        if (a == null)
            return false;
        return a.equals(b);
    }

    private void checkNumberOperand(Token operator, Object operand) {
        if (operand instanceof Double)
            return;
        throw new RuntimeError(operator, "Operand must be a number.");
    }

    private void checkNumberOperands(Token operator,
            Object left, Object right) {
        if (left instanceof Double && right instanceof Double)
            return;
        throw new RuntimeError(operator, "Operands must be numbers.");
    }

    private void checkNonZero(Token operator, Object operand) {
        if (!(operand instanceof Double))
            return;
        if ((Double) operand == 0) {
            throw new RuntimeError(operator, "Error: division by zero.");
        }
    }

    private String stringify(Object object) {
        if (object == null)
            return "nil";
        if (object instanceof Double) {
            String text = object.toString();
            if (text.endsWith(".0")) {
                text = text.substring(0, text.length() - 2);
            }
            return text;
        }
        return object.toString();
    }

    // Semantic analysis

    void resolve(Expr expr, int depth) {
        locals.put(expr, depth);
    }

    private Object lookUpVariable(Token name, Expr expr) {
        Integer distance = locals.get(expr);
        if (distance != null) {
            return environment.getAt(distance, name.lexeme);
        } else {
            return globals.get(name);
        }
    }
}