package jlox;

import java.util.List;
import java.util.Map;

class LoxClass extends LoxInstance implements LoxCallable {
    final String name;
    final Map<String, LoxFunction> methods;
    final Map<String, LoxFunction> statics;
    final Map<String, LoxGetter> getters;

    LoxClass(String name, Map<String, LoxFunction> methods, Map<String, LoxFunction> statics,
            Map<String, LoxGetter> getters) {
        super(null); // Pass null as the class since this is the class itself
        this.name = name;
        this.methods = methods;
        this.statics = statics;
        this.getters = getters;
    }

    LoxFunction findMethod(String name) {
        return methods.get(name);
    }

    @Override
    public int arity() {
        LoxFunction initializer = findMethod("init");
        if (initializer != null)
            return initializer.arity();
        return 0;
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        LoxInstance instance = new LoxInstance(this);
        LoxFunction initializer = findMethod("init");

        if (initializer != null) {
            initializer.bind(instance).call(interpreter, arguments);
        }

        for (String name : getters.keySet()) {
            LoxGetter getter = getters.get(name).bind(instance);
            instance.addGetter(name, getter);
        }

        return instance;
    }

    @Override
    public String toString() {
        return "<Class " + name + ">";
    }

    @Override
    public Object get(Token name) {
        LoxFunction method = statics.get(name.lexeme);
        if (method != null)
            return method;

        throw new RuntimeError(name, "Undefined property '" + name.lexeme + "'.");
    }
}