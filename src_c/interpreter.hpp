#ifndef MUALI_INTERPRETER
#define MUALI_INTERPRETER

#include <cassert>
#include <cstring>
#include <cstdint>

#include "types.hpp"
#include "grammar.hpp"
#include "vm_common.hpp"

struct Variable {
    union Data {
        uint8_t boolean;
        int64_t integer;
        double real;
        Shared<String> string;
        Shared<Vec<Variable>> array;
        Shared<ListMap<Variable, Variable>> dict;
        Shared<Function> func;
        constexpr Data() {}
        ~Data() {}
    } data;
    TypeId kind = TYPEID_NULL;
    
    static Variable of_type(TypeId id) { auto ret = Variable(); ret.kind = id; return ret; }
    
    constexpr Variable(){}
    
    constexpr Variable(Variable && other)
    {
        kind = other.kind;
        switch (kind)
        {
        case TYPEID_INT:
        case TYPEID_FLOAT:
        case TYPEID_BOOL:
            data.integer = std::move(other.data.integer);
            break;
        case TYPEID_STRING:
            data.string = std::move(other.data.string);
            break;
        case TYPEID_ARRAY:
            data.array = std::move(other.data.array);
            break;
        case TYPEID_DICT:
            data.dict = std::move(other.data.dict);
            break;
        case TYPEID_FUNC:
            data.func = std::move(other.data.func);
            break;
        default:
            break;
        }
    }
    
    constexpr Variable(const Variable & other)
    {
        kind = other.kind;
        switch (kind)
        {
        case TYPEID_INT:
        case TYPEID_FLOAT:
        case TYPEID_BOOL:
            data.integer = std::move(other.data.integer);
            break;
        case TYPEID_STRING:
            data.string = other.data.string;
            break;
        case TYPEID_ARRAY:
            data.array = other.data.array;
            break;
        case TYPEID_DICT:
            data.dict = other.data.dict;
            break;
        case TYPEID_FUNC:
            data.func = other.data.func;
            break;
        default:
            break;
        }
    }
    
    constexpr Variable & operator=(Variable && other)
    {
        destruct();
        
        kind = other.kind;
        switch (kind)
        {
        case TYPEID_INT:
        case TYPEID_FLOAT:
        case TYPEID_BOOL:
            data.integer = std::move(other.data.integer);
            break;
        case TYPEID_STRING:
            data.string = std::move(other.data.string);
            break;
        case TYPEID_ARRAY:
            data.array = std::move(other.data.array);
            break;
        case TYPEID_DICT:
            data.dict = std::move(other.data.dict);
            break;
        case TYPEID_FUNC:
            data.func = std::move(other.data.func);
            break;
        default:
            break;
        }
        return *this;
    }
    
    constexpr Variable & operator=(const Variable & other)
    {
        destruct();
        
        kind = other.kind;
        switch (kind)
        {
        case TYPEID_INT:
        case TYPEID_FLOAT:
        case TYPEID_BOOL:
            data.integer = other.data.integer;
            break;
        case TYPEID_STRING:
            data.string = other.data.string;
            break;
        case TYPEID_ARRAY:
            data.array = other.data.array;
            break;
        case TYPEID_DICT:
            data.dict = other.data.dict;
            break;
        case TYPEID_FUNC:
            data.func = other.data.func;
            break;
        default:
            break;
        }
        return *this;
    }
    
    void destruct()
    {
        switch (kind)
        {
        case TYPEID_STRING:
            data.string = 0;
            break;
        case TYPEID_ARRAY:
            data.array = 0;
            break;
        case TYPEID_DICT:
            data.dict = 0;
            break;
        case TYPEID_FUNC:
            data.func = 0;
            break;
        default:
            break;
        }
    }
    
    ~Variable()
    {
        destruct();
    }
};

struct Interpreter;
extern "C" typedef void (*OpHandler)(const uint8_t * pc, Variable * vars, Interpreter * global);
struct OpTable {
    OpHandler t[256];
};


#define DEF_HANDLER(X) extern "C" void X(const uint8_t *, Variable *, Interpreter *)

DEF_HANDLER(op_set);
DEF_HANDLER(op_setimm);

DEF_HANDLER(op_add);
DEF_HANDLER(op_addimm);

DEF_HANDLER(op_sub);
DEF_HANDLER(op_subimm);

DEF_HANDLER(op_mul);
DEF_HANDLER(op_mulimm);

DEF_HANDLER(op_div);
DEF_HANDLER(op_divimm);

DEF_HANDLER(op_mod);
DEF_HANDLER(op_modimm);

DEF_HANDLER(op_inci);
DEF_HANDLER(op_deci);

DEF_HANDLER(op_incf);
DEF_HANDLER(op_decf);

DEF_HANDLER(op_bitand);
DEF_HANDLER(op_bitor);
DEF_HANDLER(op_bitxor);
DEF_HANDLER(op_shl);
DEF_HANDLER(op_shr);

DEF_HANDLER(op_negate);
DEF_HANDLER(op_not);

DEF_HANDLER(op_cmpi0);
DEF_HANDLER(op_cmpi1);
DEF_HANDLER(op_cmpf0);
DEF_HANDLER(op_cmpf1);

DEF_HANDLER(op_cmpe);
DEF_HANDLER(op_cmpne);
DEF_HANDLER(op_cmpgt);
DEF_HANDLER(op_cmplt);
DEF_HANDLER(op_cmpgte);
DEF_HANDLER(op_cmplte);

DEF_HANDLER(op_and);
DEF_HANDLER(op_or);

DEF_HANDLER(op_setnull);
DEF_HANDLER(op_setzeroi);
DEF_HANDLER(op_setzerof);
DEF_HANDLER(op_setonei);
DEF_HANDLER(op_setonef);
DEF_HANDLER(op_setnegonei);
DEF_HANDLER(op_setnegonef);
DEF_HANDLER(op_setemptystr);
DEF_HANDLER(op_setemptyarray);
DEF_HANDLER(op_setemptydict);
DEF_HANDLER(op_settrue);
DEF_HANDLER(op_setfalse);

DEF_HANDLER(op_tostring);
DEF_HANDLER(op_toint);
DEF_HANDLER(op_tofloat);
DEF_HANDLER(op_ftoibits);
DEF_HANDLER(op_itofbits);

DEF_HANDLER(op_get_index);
DEF_HANDLER(op_set_index);
DEF_HANDLER(op_set_indeximm);

DEF_HANDLER(op_get_member);
DEF_HANDLER(op_set_member);
DEF_HANDLER(op_set_memberimm);

DEF_HANDLER(op_get_global);
DEF_HANDLER(op_set_global);
DEF_HANDLER(op_set_globalimm);

DEF_HANDLER(op_sqrt);

DEF_HANDLER(op_returnval);
DEF_HANDLER(op_returnimm);
DEF_HANDLER(op_returnnull);

DEF_HANDLER(op_j);
DEF_HANDLER(op_jif);
DEF_HANDLER(op_jifnot);
DEF_HANDLER(op_jifelse);
DEF_HANDLER(op_jifnotelse);
DEF_HANDLER(op_jcmp);

DEF_HANDLER(op_call);
DEF_HANDLER(op_call_indirect);
DEF_HANDLER(op_calldiscard);
DEF_HANDLER(op_calld_indirect);
DEF_HANDLER(op_become);
DEF_HANDLER(op_become_indirect);

DEF_HANDLER(op_noop);
DEF_HANDLER(op_exit);
DEF_HANDLER(op_fault);

DEF_HANDLER(op_unk);

constexpr OpTable make_opcode_table()
{
    OpTable table;
    for (size_t i = 0; i < 256; i++)
        table.t[i] = op_unk;
    
    table.t[OP_SETIMM] = op_setimm;
    table.t[OP_ADD] = op_add;
    table.t[OP_RETURNVAL] = op_returnval;
    
    return table;
}

constexpr OpTable opcode_table = make_opcode_table();

struct Interpreter;

struct Interpreter {
    Vec<Shared<Function>> funcs;
    Vec<Variable> vars;
    ListMap<String, size_t> func_names;
    ListMap<String, size_t> var_names;
    
    Variable retval; // return value trampoline
    
    Interpreter(Global info);
    Variable call_func(Shared<Function> func, Vec<Variable> args);
    Variable call_func_by_name(String funcname, Vec<Variable> args);
};

extern "C" void op_unk(const uint8_t * pc, Variable * vars, Interpreter * interpreter)
{
    (void)vars;
    (void)interpreter;
    printf("unknown instruction %02X -- breaking!\n", *(pc-1));
    throw;
    return;
}

#define CALL_NEXT() __attribute__((musttail)) return opcode_table.t[*pc](++pc, vars, global);

Variable read_immediate(const uint8_t * & pc)
{
    auto c = *pc;
    pc++;
    auto ret = Variable::of_type(c);
    switch (c)
    {
    case TYPEID_NULL:
        return ret;
    case TYPEID_INT:
        memcpy(&ret.data.integer, pc, 8);
        pc += 8;
        return ret;
    default:
        assert(((void)"TODO other immediates interpreter", 0));
    }
}

size_t read_varlen_int(const uint8_t * & pc)
{
    size_t ret = 0;
    while (*pc == 255)
    {
        ret += 255;
        pc++;
    }
    ret += *pc;
    pc++;
    return ret;
}

extern "C" void op_setimm(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto index = read_varlen_int(pc);
        auto var = read_immediate(pc);
        vars[index] = var;
    }
    CALL_NEXT();
}

extern "C" void op_add(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto i_in2 = read_varlen_int(pc);
        auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        auto & var2 = vars[i_in2];
        switch (var1.kind)
        {
        case TYPEID_INT:
            out = var1;
            if (var2.kind == TYPEID_INT)
                out.data.integer += var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
                out.data.integer += var2.data.real;
            break;
        case TYPEID_FLOAT:
            out = var1;
            if (var2.kind == TYPEID_INT)
                out.data.real += var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
                out.data.real += var2.data.real;
            out.data.real += var2.data.real;
            break;
        default:
            assert(((void)"unknown type pair for + operator", 0));
        }
    }
    CALL_NEXT();
}

extern "C" void op_returnval(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto i = read_varlen_int(pc);
        auto & ret = vars[i];
        global->retval = std::move(ret);
    }
}

inline Interpreter::Interpreter(Global info)
{
    funcs = std::move(info.funcs);
    func_names = std::move(info.func_names);
    var_names = std::move(info.var_names);
}

inline Variable Interpreter::call_func(Shared<Function> func, Vec<Variable> args)
{
    Vec<Variable> vars;
    for (size_t i = 0; i < func->num_vars + func->num_regs; i++)
        vars.push_back(Variable());
    
    const uint8_t * pc = func->code.data();
    auto v = vars.data();
    opcode_table.t[*pc](++pc, v, this);
    return std::move(retval);
}

inline Variable Interpreter::call_func_by_name(String funcname, Vec<Variable> args)
{
    if (!func_names.count(funcname))
        throw;
    return call_func(funcs[func_names[funcname]], args);
}

#endif // MUALI_INTERPRETER
