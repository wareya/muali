#ifndef MUALI_INTERPRETER
#define MUALI_INTERPRETER

#include <cassert>
#include <cstring>
#include <cstdint>

#include "types.hpp"
#include "grammar.hpp"
#include "vm_common.hpp"

#define OPHANDLER_ABI extern "C" [[clang::preserve_none]]
//#define OPHANDLER_ABI [[clang::preserve_none]]
//#define OPHANDLER_ABI

#define INSANELY_UNSAFE_DISABLE_VARLEN_VARREG_ENCODING
//#define NO_THROW_ALL_DANGER_LETS_GO
#define DO_NOT_TRACK_INTERPRETER_PREV_OPCODE

#ifdef NO_THROW_ALL_DANGER_LETS_GO
#define ASSERT_THROW(X) { }
#else
#define ASSERT_THROW(X) { if (!(X)) throw; }
//#define ASSERT_THROW(X) { if (!(X)) throw; }
//#define ASSERT_THROW(X) { if (!(X)) throw #X; }
//#define ASSERT_THROW(X) { assert(X); }
#endif

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
            data.integer = other.data.integer;
            break;
        case TYPEID_STRING:
            ::new((void*)(&data.string)) Shared<String>(std::move(other.data.string));
            break;
        case TYPEID_ARRAY:
            ::new((void*)(&data.array)) Shared<Vec<Variable>>(std::move(other.data.array));
            break;
        case TYPEID_DICT:
            ::new((void*)(&data.dict)) Shared<ListMap<Variable, Variable>>(std::move(other.data.dict));
            break;
        case TYPEID_FUNC:
            ::new((void*)(&data.func)) Shared<Function>(std::move(other.data.func));
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
    
    constexpr void destruct()
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
OPHANDLER_ABI typedef void (*OpHandler)(const uint8_t * pc, Variable * vars, Interpreter * global);
struct OpTable {
    OpHandler t[256];
};


#define DEF_HANDLER(X) OPHANDLER_ABI void X(const uint8_t *, Variable *, Interpreter *)

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

DEF_HANDLER(op_jinciltimm);

DEF_HANDLER(op_bitand);
DEF_HANDLER(op_bitor);
DEF_HANDLER(op_bitxor);
DEF_HANDLER(op_shl);
DEF_HANDLER(op_shr);

DEF_HANDLER(op_bitandimm);
DEF_HANDLER(op_bitorimm);
DEF_HANDLER(op_bitxorimm);
DEF_HANDLER(op_shlimm);
DEF_HANDLER(op_shrimm);

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

DEF_HANDLER(op_call);
DEF_HANDLER(op_call_indirect);
DEF_HANDLER(op_calldiscard);
DEF_HANDLER(op_calld_indirect);

DEF_HANDLER(op_j);
DEF_HANDLER(op_jif);
DEF_HANDLER(op_jifnot);
DEF_HANDLER(op_jifnull);
DEF_HANDLER(op_jifnotnull);
DEF_HANDLER(op_jcmp);
DEF_HANDLER(op_jcmpimm);
DEF_HANDLER(op_jiltimm);

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
    
    table.t[OP_SET] = op_set;
    table.t[OP_SETIMM] = op_setimm;
    table.t[OP_SETZEROI] = op_setzeroi;
    table.t[OP_ADD] = op_add;
    table.t[OP_ADDIMM] = op_addimm;
    table.t[OP_SUB] = op_sub;
    table.t[OP_SUBIMM] = op_subimm;
    table.t[OP_MUL] = op_mul;
    table.t[OP_MULIMM] = op_mulimm;
    table.t[OP_DIV] = op_div;
    table.t[OP_DIVIMM] = op_divimm;
    table.t[OP_SHL] = op_shl;
    table.t[OP_SHLIMM] = op_shlimm;
    table.t[OP_SHR] = op_shr;
    table.t[OP_SHRIMM] = op_shrimm;
    
    table.t[OP_RETURNVAL] = op_returnval;
    table.t[OP_J] = op_j;
    table.t[OP_JILTIMM] = op_jiltimm;
    
    table.t[OP_INCI] = op_inci;
    table.t[OP_DECI] = op_deci;
    table.t[OP_JINCILTIMM] = op_jinciltimm;
    table.t[OP_NEGATE] = op_negate;
    
    return table;
}

constexpr OpTable opcode_table = make_opcode_table();

struct Interpreter {
    Vec<Shared<Function>> funcs;
    Vec<Variable> vars;
    ListMap<String, size_t> func_names;
    ListMap<String, size_t> var_names;
    
    Variable retval; // return value trampoline
    
#ifndef DO_NOT_TRACK_INTERPRETER_PREV_OPCODE
    uint8_t prev_inst;
#endif
    
    Interpreter(Global info);
    Variable call_func(Shared<Function> func, Vec<Variable> args);
    Variable call_func_by_name(String funcname, Vec<Variable> args);
};

OPHANDLER_ABI void op_unk(const uint8_t * pc, Variable * vars, Interpreter * interpreter)
{
    asm("");
    (void)vars;
    (void)interpreter;
    printf("unknown instruction %02X -- breaking!\n", *(pc-1));
#ifndef DO_NOT_TRACK_INTERPRETER_PREV_OPCODE
    printf("previous instruction was %02X\n", interpreter->prev_inst);
#endif
    throw;
    return;
}

#ifdef DO_NOT_TRACK_INTERPRETER_PREV_OPCODE
#define CALL_NEXT() \
    { auto c = *pc; [[clang::musttail]] return opcode_table.t[c](++pc, vars, global); }
#else
#define CALL_NEXT() \
    if (opcode_table.t[*pc] != op_unk) global->prev_inst = *pc;\
    { auto c = *pc; [[clang::musttail]] return opcode_table.t[c](++pc, vars, global); }
#endif

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
    case TYPEID_FLOAT:
        memcpy(&ret.data.real, pc, 8);
        pc += 8;
        return ret;
    default:
        ASSERT_THROW(((void)"TODO other immediates interpreter", 0));
    }
    return Variable();
}

OPHANDLER_ABI size_t read_varlen_int(const uint8_t * & pc)
{
#ifndef INSANELY_UNSAFE_DISABLE_VARLEN_VARREG_ENCODING
    //size_t ret = 0;
    //while (1)
    //{
    //    ret += *pc++;
    //    if (pc[-1] != 255)
    //        return ret;
    //}
    
    //size_t ret = 0;
    //do { ret += *pc++; } while (pc[-1] == 255);
    
    size_t ret = *pc++;
    while (pc[-1] == 255)
        ret += *pc++;
    return ret;
#else
    return *pc++;
#endif
}
static inline uint8_t read_u8(const uint8_t * & pc)
{
    uint8_t ret;
    memcpy(&ret, pc, 1);
    pc += 1;
    return ret;
}
static inline uint32_t read_u32(const uint8_t * & pc)
{
    uint32_t ret;
    memcpy(&ret, pc, 4);
    pc += 4;
    return ret;
}
static inline uint64_t read_u64(const uint8_t * & pc)
{
    uint64_t ret;
    memcpy(&ret, pc, 8);
    pc += 8;
    return ret;
}

OPHANDLER_ABI void op_j(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        int32_t offset = read_u32(pc);
        pc += offset;
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_jiltimm(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto index = read_varlen_int(pc);
        int64_t imm = read_u64(pc);
        int32_t offset = read_u32(pc);
        
        ASSERT_THROW(vars[index].kind == TYPEID_INT);
        
        if (vars[index].data.integer < imm)
            pc += offset;
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_inci(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto index = read_varlen_int(pc);
        ASSERT_THROW(vars[index].kind == TYPEID_INT);
        vars[index].data.integer += 1;
    }
    CALL_NEXT();
}
OPHANDLER_ABI void op_deci(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto index = read_varlen_int(pc);
        ASSERT_THROW(vars[index].kind == TYPEID_INT);
        vars[index].data.integer -= 1;
    }
    CALL_NEXT();
}
OPHANDLER_ABI void op_jinciltimm(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto index = read_varlen_int(pc);
        int64_t imm = read_u64(pc);
        int32_t offset = read_u32(pc);
        
        ASSERT_THROW(vars[index].kind == TYPEID_INT);
        vars[index].data.integer += 1;
        
        if (vars[index].data.integer < imm)
            pc += offset;
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_negate(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto index = read_varlen_int(pc);
        auto & var = vars[index];
        switch (var.kind)
        {
        case TYPEID_INT:
            var.data.integer = -var.data.integer;
            break;
        case TYPEID_FLOAT:
            var.data.real = -var.data.real;
            break;
        default:
            ASSERT_THROW(((void)"unknown type pair for + operator", 0));
        }
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_setimm(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto index = read_varlen_int(pc);
        auto var = read_immediate(pc);
        vars[index] = var;
    }
    CALL_NEXT();
}
OPHANDLER_ABI void op_set(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto index = read_varlen_int(pc);
        auto varindex = read_varlen_int(pc);
        //printf("writing to index %zu from index %zu....\n", index, varindex);
        vars[index] = vars[varindex];
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_setzeroi(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto index = read_varlen_int(pc);
        vars[index] = Variable::of_type(TYPEID_INT);
        vars[index].data.integer = 0;
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_add(const uint8_t * pc, Variable * vars, Interpreter * global)
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
            {
                out.kind = TYPEID_FLOAT;
                out.data.real = out.data.integer + var2.data.real;
            }
            break;
        case TYPEID_FLOAT:
            out = var1;
            if (var2.kind == TYPEID_INT)
                out.data.real += var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
                out.data.real += var2.data.real;
            break;
        default:
            ASSERT_THROW(((void)"unknown type pair for + operator", 0));
        }
    }
    CALL_NEXT();
}
OPHANDLER_ABI void op_addimm(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto var2 = read_immediate(pc);
        auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        switch (var1.kind)
        {
        case TYPEID_INT:
            out = var1;
            if (var2.kind == TYPEID_INT)
                out.data.integer += var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
            {
                out.kind = TYPEID_FLOAT;
                out.data.real = out.data.integer + var2.data.real;
            }
            break;
        case TYPEID_FLOAT:
            out = var1;
            if (var2.kind == TYPEID_INT)
                out.data.real += var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
                out.data.real += var2.data.real;
            break;
        default:
            ASSERT_THROW(((void)"unknown type pair for + operator", 0));
        }
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_sub(const uint8_t * pc, Variable * vars, Interpreter * global)
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
                out.data.integer -= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
            {
                out.kind = TYPEID_FLOAT;
                out.data.real = out.data.integer - var2.data.real;
            }
            break;
        case TYPEID_FLOAT:
            out = var1;
            if (var2.kind == TYPEID_INT)
                out.data.real -= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
                out.data.real -= var2.data.real;
            break;
        default:
            ASSERT_THROW(((void)"unknown type pair for - operator", 0));
        }
    }
    CALL_NEXT();
}
OPHANDLER_ABI void op_subimm(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto var2 = read_immediate(pc);
        auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        switch (var1.kind)
        {
        case TYPEID_INT:
            out = var1;
            if (var2.kind == TYPEID_INT)
                out.data.integer -= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
            {
                out.kind = TYPEID_FLOAT;
                out.data.real = out.data.integer - var2.data.real;
            }
            break;
        case TYPEID_FLOAT:
            out = var1;
            if (var2.kind == TYPEID_INT)
                out.data.real -= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
                out.data.real -= var2.data.real;
            break;
        default:
            ASSERT_THROW(((void)"unknown type pair for - operator", 0));
        }
    }
    CALL_NEXT();
}


OPHANDLER_ABI void op_mul(const uint8_t * pc, Variable * vars, Interpreter * global)
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
                out.data.integer *= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
            {
                out.kind = TYPEID_FLOAT;
                out.data.real = out.data.integer * var2.data.real;
            }
            break;
        case TYPEID_FLOAT:
            out = var1;
            if (var2.kind == TYPEID_INT)
                out.data.real *= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
                out.data.real *= var2.data.real;
            break;
        default:
            ASSERT_THROW(((void)"unknown type pair for * operator", 0));
        }
    }
    CALL_NEXT();
}
OPHANDLER_ABI void op_mulimm(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto var2 = read_immediate(pc);
        auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        switch (var1.kind)
        {
        case TYPEID_INT:
            out = var1;
            if (var2.kind == TYPEID_INT)
                out.data.integer *= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
            {
                out.kind = TYPEID_FLOAT;
                out.data.real = out.data.integer * var2.data.real;
            }
            break;
        case TYPEID_FLOAT:
            out = var1;
            if (var2.kind == TYPEID_INT)
                out.data.real *= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
                out.data.real *= var2.data.real;
            break;
        default:
            ASSERT_THROW(((void)"unknown type pair for * operator", 0));
        }
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_div(const uint8_t * pc, Variable * vars, Interpreter * global)
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
                out.data.integer /= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
            {
                out.kind = TYPEID_FLOAT;
                out.data.real = out.data.integer / var2.data.real;
            }
            break;
        case TYPEID_FLOAT:
            out = var1;
            if (var2.kind == TYPEID_INT)
                out.data.real /= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
                out.data.real /= var2.data.real;
            break;
        default:
            printf("%02X vs %02X (indexes %zu %zu) \n", var1.kind, var2.kind, i_in1, i_in2);
            ASSERT_THROW(((void)"unknown type pair for / operator", 0));
        }
    }
    CALL_NEXT();
}
OPHANDLER_ABI void op_divimm(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto var2 = read_immediate(pc);
        auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        switch (var1.kind)
        {
        case TYPEID_INT:
            out = var1;
            if (var2.kind == TYPEID_INT)
                out.data.integer /= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
            {
                out.kind = TYPEID_FLOAT;
                out.data.real = out.data.integer / var2.data.real;
            }
            break;
        case TYPEID_FLOAT:
            out = var1;
            if (var2.kind == TYPEID_INT)
                out.data.real /= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
                out.data.real /= var2.data.real;
            break;
        default:
            printf("%02X vs %02X (index %zu) \n", var1.kind, var2.kind, i_in1);
            printf("%02X vs %02X\n", var1.kind, var2.kind);
            ASSERT_THROW(((void)"unknown type pair for / operator (immediate)", 0));
        }
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_shl(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto i_in2 = read_varlen_int(pc);
        auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        auto & var2 = vars[i_in2];
        assert(var1.kind == TYPEID_INT && var2.kind == TYPEID_INT);
        out = var1;
        out.data.integer <<= var2.data.integer;
    }
    CALL_NEXT();
}
OPHANDLER_ABI void op_shlimm(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto var2 = read_immediate(pc);
        auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        assert(var1.kind == TYPEID_INT && var2.kind == TYPEID_INT);
        out = var1;
        out.data.integer <<= var2.data.integer;
    }
    CALL_NEXT();
}


OPHANDLER_ABI void op_shr(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto i_in2 = read_varlen_int(pc);
        auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        auto & var2 = vars[i_in2];
        assert(var1.kind == TYPEID_INT && var2.kind == TYPEID_INT);
        out = var1;
        out.data.integer >>= var2.data.integer;
    }
    CALL_NEXT();
}
OPHANDLER_ABI void op_shrimm(const uint8_t * pc, Variable * vars, Interpreter * global)
{
    {
        auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto var2 = read_immediate(pc);
        auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        assert(var1.kind == TYPEID_INT && var2.kind == TYPEID_INT);
        out = var1;
        out.data.integer >>= var2.data.integer;
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_returnval(const uint8_t * pc, Variable * vars, Interpreter * global)
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
    (void)args;
    
    Vec<Variable> vars;
    printf("allocating %zu var-regs....\n", func->num_vars + func->num_regs);
    for (size_t i = 0; i < func->num_vars + func->num_regs; i++)
        vars.push_back(Variable());
    printf("allocated %zu var-regs\n", vars.size());
    
    const uint8_t * pc = func->code.data();
    auto v = vars.data();
#ifndef DO_NOT_TRACK_INTERPRETER_PREV_OPCODE
    prev_inst = *pc;
#else
    uint8_t prev_inst = *pc;
#endif
    opcode_table.t[prev_inst](++pc, v, this);
    return std::move(retval);
}

inline Variable Interpreter::call_func_by_name(String funcname, Vec<Variable> args)
{
    if (!func_names.count(funcname))
        throw;
    return call_func(funcs[func_names[funcname]], args);
}

#endif // MUALI_INTERPRETER