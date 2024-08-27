package jlox;


class LoxGetter extends LoxFunction {
    LoxGetter(Stmt.Function declaration, Environment closure) {
        super(declaration, closure, false);
    }

    public Object call(Interpreter interpreter) {
        Environment environment = new Environment(closure);
        try {
            interpreter.executeBlock(declaration.body, environment);
        } catch (Return returnValue) {
            return returnValue.value;
        }

        return null;
    }

    @Override
    public int arity() {
        return 0;
    }

    @Override
    LoxGetter bind(LoxInstance instance) {
        Environment environment = new Environment(closure);
        environment.define("this", instance);
        return new LoxGetter(declaration, environment);
    }
}