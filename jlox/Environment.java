package jlox;

import java.util.HashMap;
import java.util.Map;

class Environment {
    private final Map<String, Object> values = new HashMap<>();
    final Environment enclosing;
    final static public Object UNINITIALIZED = new Object();

    Environment() {
        enclosing = null;
    }

    Environment(Environment enclosing) {
        this.enclosing = enclosing;
    }

    void define(String name, Object value) {
        values.put(name, value);
    }

    Object get(Token name) {
        if (values.containsKey(name.lexeme)) {
            Object value = values.get(name.lexeme);
            if (value == UNINITIALIZED) {
                throw new RuntimeError(name, "Trying to access uninitialized variable '" + name.lexeme + "'.");
            }
            return value;
        }
        if (enclosing != null)
            return enclosing.get(name); // recursive call through the scope stack
        throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }

    void assign(Token name, Object value) {
        if (values.containsKey(name.lexeme)) {
            values.put(name.lexeme, value);
            return;
        }

        // We COULD walk the env stack here
        // But it would mean that we can modify the value of global variables
        // Do we want that ?

        if (enclosing != null) {
            enclosing.assign(name, value);
            return;
        }

        throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }

    Object getAt(Integer distance, String name) {
        return ancestor(distance).values.get(name);
    }

    void assignAt(Integer distance, Token name, Object value) {
        ancestor(distance).values.put(name.lexeme, value);
    }

    Environment ancestor(Integer distance) {
        Environment environment = this;
        for (int i = 0; i < distance; i++) {
            environment = environment.enclosing;
        }
        
        return environment;
    }
}