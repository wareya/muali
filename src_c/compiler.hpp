#ifndef BBEL_COMPILER
#define BBEL_COMPILER

#include <cassert>
#include <cstring>
#include <cstdint>

#include "types.hpp"
#include "grammar.hpp"

typedef uint32_t TypeId;

struct Function {
    Vec<TypeId> arg_types;
    Vec<uint8_t> bytecode;
    size_t num_vars;
    size_t num_regs;
    TypeId ret_type;
};

struct Variable {
    union {
        int64_t integer;
        double real;
        Shared<String> string;
        Shared<Vec<Variable>> array;
        Shared<ListMap<Variable, Variable>> dict;
        Shared<Function> func;
    } asdf;
    TypeId kind;
};

struct Global {
    Vec<Shared<Function>> funcs;
    Vec<Variable> vars;
};

Global compile_root();

#endif // BBEL_COMPILER
