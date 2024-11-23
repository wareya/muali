#ifndef MUALI_INTERPRETER
#define MUALI_INTERPRETER

#include <cassert>
#include <cstring>
#include <cstdint>

#include "types.hpp"
#include "grammar.hpp"
#include "vm_common.hpp"


#define OPHANDLER_ABI extern "C" [[clang::preserve_none]]
//#define OPHANDLER_ABI extern "C"
//#define OPHANDLER_ABI [[clang::preserve_none]]
//#define OPHANDLER_ABI

#define USE_EXTRA_ASSERTS
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


extern "C" uint16_t read_op(const uint8_t * & pc)
{
    uint16_t c;
    memcpy(&c, pc, 2);
    c &= (1<<INTERPRETER_OPCODE_TABLE_BITS) - 1;
    //if (*pc >= 0x80) [[likely]]
        //return *pc;
        //return *pc++;
    //pc += 2;
    return c;
}


//#define OPHANDLER_ARGS const uint8_t * pc, Variable * vars, Interpreter * global
#define OPHANDLER_ARGS Variable * vars, const uint8_t * pc, Interpreter * global
#define CALL_ORDER vars, pc, global

    //global->accum += 1; 
#ifdef DO_NOT_TRACK_INTERPRETER_PREV_OPCODE
#define CALL_NEXT() \
    { uint16_t c = read_op(pc); [[clang::musttail]] return opcode_table.t[c](CALL_ORDER); }
#else
#define CALL_NEXT() \
    if (opcode_table.t[*pc] != op_unk) global->prev_inst = *pc;\
    { uint16_t c = read_op(pc); [[clang::musttail]] return opcode_table.t[c](CALL_ORDER); }
#endif

struct Interpreter;
OPHANDLER_ABI typedef void (*OpHandler)(OPHANDLER_ARGS);
struct OpTable {
    OpHandler t[(1<<INTERPRETER_OPCODE_TABLE_BITS)];
};


#define DEC_HANDLER(X) OPHANDLER_ABI void X(OPHANDLER_ARGS)

DEC_HANDLER(op_set);
DEC_HANDLER(op_add);
DEC_HANDLER(op_sub);
DEC_HANDLER(op_mul);
DEC_HANDLER(op_div);
DEC_HANDLER(op_mod);
DEC_HANDLER(op_inci);
DEC_HANDLER(op_deci);

DEC_HANDLER(op_setimm);
DEC_HANDLER(op_addimm);
DEC_HANDLER(op_subimm);
DEC_HANDLER(op_mulimm);
DEC_HANDLER(op_divimm);
DEC_HANDLER(op_modimm);
DEC_HANDLER(op_inciimm);
DEC_HANDLER(op_deciimm);

DEC_HANDLER(op_incf);
DEC_HANDLER(op_decf);

DEC_HANDLER(op_jincilt);
DEC_HANDLER(op_jinciltimm);

DEC_HANDLER(op_bitand);
DEC_HANDLER(op_bitor);
DEC_HANDLER(op_bitxor);
DEC_HANDLER(op_shl);
DEC_HANDLER(op_shr);

DEC_HANDLER(op_bitandimm);
DEC_HANDLER(op_bitorimm);
DEC_HANDLER(op_bitxorimm);
DEC_HANDLER(op_shlimm);
DEC_HANDLER(op_shrimm);

DEC_HANDLER(op_negate);
DEC_HANDLER(op_not);

DEC_HANDLER(op_cmpi0);
DEC_HANDLER(op_cmpi1);
DEC_HANDLER(op_cmpf0);
DEC_HANDLER(op_cmpf1);

DEC_HANDLER(op_cmpe);
DEC_HANDLER(op_cmpne);
DEC_HANDLER(op_cmpgt);
DEC_HANDLER(op_cmplt);
DEC_HANDLER(op_cmpgte);
DEC_HANDLER(op_cmplte);

DEC_HANDLER(op_and);
DEC_HANDLER(op_or);

DEC_HANDLER(op_setnull);
DEC_HANDLER(op_setzeroi);
DEC_HANDLER(op_setzerof);
DEC_HANDLER(op_setonei);
DEC_HANDLER(op_setonef);
DEC_HANDLER(op_setnegonei);
DEC_HANDLER(op_setnegonef);
DEC_HANDLER(op_setemptystr);
DEC_HANDLER(op_setemptyarray);
DEC_HANDLER(op_setemptydict);
DEC_HANDLER(op_settrue);
DEC_HANDLER(op_setfalse);

DEC_HANDLER(op_tostring);
DEC_HANDLER(op_toint);
DEC_HANDLER(op_tofloat);
DEC_HANDLER(op_ftoibits);
DEC_HANDLER(op_itofbits);

DEC_HANDLER(op_get_index);
DEC_HANDLER(op_set_index);
DEC_HANDLER(op_set_indeximm);

DEC_HANDLER(op_get_member);
DEC_HANDLER(op_set_member);
DEC_HANDLER(op_set_memberimm);

DEC_HANDLER(op_get_global);
DEC_HANDLER(op_set_global);
DEC_HANDLER(op_set_globalimm);

DEC_HANDLER(op_sqrt);

DEC_HANDLER(op_returnval);
DEC_HANDLER(op_returnimm);
DEC_HANDLER(op_returnnull);

DEC_HANDLER(op_call);
DEC_HANDLER(op_call_indirect);
DEC_HANDLER(op_calldiscard);
DEC_HANDLER(op_calld_indirect);

DEC_HANDLER(op_j);
DEC_HANDLER(op_jif);
DEC_HANDLER(op_jifnot);
DEC_HANDLER(op_jifnull);
DEC_HANDLER(op_jifnotnull);
DEC_HANDLER(op_jcmp);
DEC_HANDLER(op_jcmpimm);
DEC_HANDLER(op_jiltimm);

DEC_HANDLER(op_become);
DEC_HANDLER(op_become_indirect);

DEC_HANDLER(op_noop);
DEC_HANDLER(op_exit);
DEC_HANDLER(op_fault);

DEC_HANDLER(op_unk);

constexpr OpTable make_opcode_table()
{
    OpTable table;
    for (size_t i = 0; i < (1<<INTERPRETER_OPCODE_TABLE_BITS); i++)
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
    table.t[OP_JINCILT] = op_jincilt;
    table.t[OP_NEGATE] = op_negate;
    
    for (uint16_t n = 0; n < 256; n++)
    {
        if (table.t[n] != op_unk)
        {
            for (uint32_t n2 = 256; n2 < (1<<INTERPRETER_OPCODE_TABLE_BITS); n2 += 256)
            {
                table.t[n + n2] = table.t[n];
            }
        }
    }
    
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
    uint16_t prev_inst;
#endif
    
    size_t accum = 0;
    
    Interpreter(Global info);
    Variable call_func(Shared<Function> func, Vec<Variable> args);
    Variable call_func_by_name(String funcname, Vec<Variable> args);
};


OPHANDLER_ABI void op_unk(OPHANDLER_ARGS)
{
    //pc += (OP_UNK > 0xFF) ? 2 : 1;
    (void)vars;
    (void)global;
    printf("unknown instruction %02X -- breaking!\n", *(pc-1));
#ifndef DO_NOT_TRACK_INTERPRETER_PREV_OPCODE
    printf("previous instruction was %02X\n", global->prev_inst);
#endif
    throw;
    return;
}

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
static inline uint32_t read_u16(const uint8_t * & pc)
{
    uint16_t ret;
    memcpy(&ret, pc, 2);
    pc += 2;
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

OPHANDLER_ABI void op_j(OPHANDLER_ARGS)
{
    pc += (OP_J > 0xFF) ? 2 : 1;
    {
        int32_t offset = read_u32(pc);
        pc += offset;
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_jiltimm(OPHANDLER_ARGS)
{
    pc += (OP_JILTIMM > 0xFF) ? 2 : 1;
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

OPHANDLER_ABI void op_inci(OPHANDLER_ARGS)
{
    pc += (OP_INCI > 0xFF) ? 2 : 1;
    {
        auto index = read_varlen_int(pc);
        ASSERT_THROW(vars[index].kind == TYPEID_INT);
        vars[index].data.integer += 1;
    }
    CALL_NEXT();
}
OPHANDLER_ABI void op_deci(OPHANDLER_ARGS)
{
    pc += (OP_DECI > 0xFF) ? 2 : 1;
    {
        auto index = read_varlen_int(pc);
        ASSERT_THROW(vars[index].kind == TYPEID_INT);
        vars[index].data.integer -= 1;
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_jinciltimm(OPHANDLER_ARGS)
{
    pc += (OP_JINCILTIMM > 0xFF) ? 2 : 1;
    {
        auto index = read_varlen_int(pc);
        int64_t imm = read_u64(pc);
        int32_t offset = read_u32(pc);
        //int16_t offset = read_u16(pc);
        
        #ifdef USE_EXTRA_ASSERTS
        ASSERT_THROW(vars[index].kind == TYPEID_INT);
        #endif
        vars[index].data.integer += 1;
        
        if (vars[index].data.integer < imm)
            pc += offset;
    }
    CALL_NEXT();
}
OPHANDLER_ABI void op_jincilt(OPHANDLER_ARGS)
{
    pc += (OP_JINCILT > 0xFF) ? 2 : 1;
    {
        auto index = read_varlen_int(pc);
        auto i_index = read_varlen_int(pc);
        int32_t offset = read_u32(pc);
        //int16_t offset = read_u16(pc);
        
        #ifdef USE_EXTRA_ASSERTS
        ASSERT_THROW(vars[index].kind == TYPEID_INT && vars[i_index].kind == TYPEID_INT);
        #endif
        vars[index].data.integer += 1;
        
        if (vars[index].data.integer < vars[i_index].data.integer)
            pc += offset;
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_negate(OPHANDLER_ARGS)
{
    pc += (OP_NEGATE > 0xFF) ? 2 : 1;
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

OPHANDLER_ABI void op_setimm(OPHANDLER_ARGS)
{
    pc += (OP_SETIMM > 0xFF) ? 2 : 1;
    {
        auto index = read_varlen_int(pc);
        auto var = read_immediate(pc);
        vars[index] = var;
    }
    CALL_NEXT();
}
OPHANDLER_ABI void op_set(OPHANDLER_ARGS)
{
    pc += (OP_SET > 0xFF) ? 2 : 1;
    {
        auto index = read_varlen_int(pc);
        auto varindex = read_varlen_int(pc);
        //printf("writing to index %zu from index %zu....\n", index, varindex);
        vars[index] = vars[varindex];
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_setzeroi(OPHANDLER_ARGS)
{
    pc += (OP_SETZEROI > 0xFF) ? 2 : 1;
    {
        auto index = read_varlen_int(pc);
        vars[index] = Variable::of_type(TYPEID_INT);
        vars[index].data.integer = 0;
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_add(OPHANDLER_ARGS)
{
    pc += (OP_ADD > 0xFF) ? 2 : 1;
    {
        //auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto i_in2 = read_varlen_int(pc);
        //auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        auto & var2 = vars[i_in2];
        switch (var1.kind)
        {
        case TYPEID_INT:
            //out = var1;
            if (var2.kind == TYPEID_INT)
                var1.data.integer += var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
            {
                var1.kind = TYPEID_FLOAT;
                var1.data.real = var1.data.integer + var2.data.real;
            }
            break;
        case TYPEID_FLOAT:
            //out = var1;
            if (var2.kind == TYPEID_INT)
                var1.data.real += var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
                var1.data.real += var2.data.real;
            break;
        default:
            ASSERT_THROW(((void)"unknown type pair for + operator", 0));
        }
    }
    CALL_NEXT();
}
OPHANDLER_ABI void op_addimm(OPHANDLER_ARGS)
{
    pc += (OP_ADDIMM > 0xFF) ? 2 : 1;
    {
        //auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto var2 = read_immediate(pc);
        //auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        switch (var1.kind)
        {
        case TYPEID_INT:
            //out = var1;
            if (var2.kind == TYPEID_INT)
                var1.data.integer += var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
            {
                var1.kind = TYPEID_FLOAT;
                var1.data.real = var1.data.integer + var2.data.real;
            }
            break;
        case TYPEID_FLOAT:
            //out = var1;
            if (var2.kind == TYPEID_INT)
                var1.data.real += var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
                var1.data.real += var2.data.real;
            break;
        default:
            ASSERT_THROW(((void)"unknown type pair for + operator", 0));
        }
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_sub(OPHANDLER_ARGS)
{
    pc += (OP_SUB > 0xFF) ? 2 : 1;
    {
        //auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto i_in2 = read_varlen_int(pc);
        //auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        auto & var2 = vars[i_in2];
        switch (var1.kind)
        {
        case TYPEID_INT:
            //out = var1;
            if (var2.kind == TYPEID_INT)
                var1.data.integer -= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
            {
                var1.kind = TYPEID_FLOAT;
                var1.data.real = var1.data.integer - var2.data.real;
            }
            break;
        case TYPEID_FLOAT:
            //out = var1;
            if (var2.kind == TYPEID_INT)
                var1.data.real -= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
                var1.data.real -= var2.data.real;
            break;
        default:
            ASSERT_THROW(((void)"unknown type pair for - operator", 0));
        }
    }
    CALL_NEXT();
}
OPHANDLER_ABI void op_subimm(OPHANDLER_ARGS)
{
    pc += (OP_SUBIMM > 0xFF) ? 2 : 1;
    {
        //auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto var2 = read_immediate(pc);
        //auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        switch (var1.kind)
        {
        case TYPEID_INT:
            //out = var1;
            if (var2.kind == TYPEID_INT)
                var1.data.integer -= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
            {
                var1.kind = TYPEID_FLOAT;
                var1.data.real = var1.data.integer - var2.data.real;
            }
            break;
        case TYPEID_FLOAT:
            //out = var1;
            if (var2.kind == TYPEID_INT)
                var1.data.real -= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
                var1.data.real -= var2.data.real;
            break;
        default:
            ASSERT_THROW(((void)"unknown type pair for - operator", 0));
        }
    }
    CALL_NEXT();
}


OPHANDLER_ABI void op_mul(OPHANDLER_ARGS)
{
    pc += (OP_MUL > 0xFF) ? 2 : 1;
    {
        //auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto i_in2 = read_varlen_int(pc);
        //auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        auto & var2 = vars[i_in2];
        switch (var1.kind)
        {
        case TYPEID_INT:
            //out = var1;
            if (var2.kind == TYPEID_INT)
                var1.data.integer *= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
            {
                var1.kind = TYPEID_FLOAT;
                var1.data.real = var1.data.integer * var2.data.real;
            }
            break;
        case TYPEID_FLOAT:
            //out = var1;
            if (var2.kind == TYPEID_INT)
                var1.data.real *= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
                var1.data.real *= var2.data.real;
            break;
        default:
            ASSERT_THROW(((void)"unknown type pair for * operator", 0));
        }
    }
    CALL_NEXT();
}
OPHANDLER_ABI void op_mulimm(OPHANDLER_ARGS)
{
    pc += (OP_MULIMM > 0xFF) ? 2 : 1;
    {
        //auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto var2 = read_immediate(pc);
        //auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        switch (var1.kind)
        {
        case TYPEID_INT:
            //out = var1;
            if (var2.kind == TYPEID_INT)
                var1.data.integer *= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
            {
                var1.kind = TYPEID_FLOAT;
                var1.data.real = var1.data.integer * var2.data.real;
            }
            break;
        case TYPEID_FLOAT:
            //out = var1;
            if (var2.kind == TYPEID_INT)
                var1.data.real *= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
                var1.data.real *= var2.data.real;
            break;
        default:
            ASSERT_THROW(((void)"unknown type pair for * operator", 0));
        }
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_div(OPHANDLER_ARGS)
{
    pc += (OP_DIV > 0xFF) ? 2 : 1;
    {
        //auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto i_in2 = read_varlen_int(pc);
        //auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        auto & var2 = vars[i_in2];
        switch (var1.kind)
        {
        case TYPEID_INT:
            //out = var1;
            if (var2.kind == TYPEID_INT)
                var1.data.integer /= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
            {
                var1.kind = TYPEID_FLOAT;
                var1.data.real = var1.data.integer / var2.data.real;
            }
            break;
        case TYPEID_FLOAT:
            //out = var1;
            if (var2.kind == TYPEID_INT)
                var1.data.real /= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
                var1.data.real /= var2.data.real;
            break;
        default:
            printf("%02X vs %02X (indexes %zu %zu) \n", var1.kind, var2.kind, i_in1, i_in2);
            ASSERT_THROW(((void)"unknown type pair for / operator", 0));
        }
    }
    CALL_NEXT();
}
OPHANDLER_ABI void op_divimm(OPHANDLER_ARGS)
{
    pc += (OP_DIVIMM > 0xFF) ? 2 : 1;
    {
        //auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto var2 = read_immediate(pc);
        //auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        switch (var1.kind)
        {
        case TYPEID_INT:
            //out = var1;
            if (var2.kind == TYPEID_INT)
                var1.data.integer /= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
            {
                var1.kind = TYPEID_FLOAT;
                var1.data.real = var1.data.integer / var2.data.real;
            }
            break;
        case TYPEID_FLOAT:
            //out = var1;
            if (var2.kind == TYPEID_INT)
                var1.data.real /= var2.data.integer;
            else if (var2.kind == TYPEID_FLOAT)
                var1.data.real /= var2.data.real;
            break;
        default:
            printf("%02X vs %02X (index %zu) \n", var1.kind, var2.kind, i_in1);
            printf("%02X vs %02X\n", var1.kind, var2.kind);
            ASSERT_THROW(((void)"unknown type pair for / operator (immediate)", 0));
        }
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_shl(OPHANDLER_ARGS)
{
    pc += (OP_SHL > 0xFF) ? 2 : 1;
    {
        //auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto i_in2 = read_varlen_int(pc);
        //auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        auto & var2 = vars[i_in2];
        assert(var1.kind == TYPEID_INT && var2.kind == TYPEID_INT);
        //out = var1;
        var1.data.integer <<= var2.data.integer;
    }
    CALL_NEXT();
}
OPHANDLER_ABI void op_shlimm(OPHANDLER_ARGS)
{
    pc += (OP_SHLIMM > 0xFF) ? 2 : 1;
    {
        //auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto var2 = read_immediate(pc);
        //auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        assert(var1.kind == TYPEID_INT && var2.kind == TYPEID_INT);
        //out = var1;
        var1.data.integer <<= var2.data.integer;
    }
    CALL_NEXT();
}


OPHANDLER_ABI void op_shr(OPHANDLER_ARGS)
{
    pc += (OP_SHR > 0xFF) ? 2 : 1;
    {
        //auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto i_in2 = read_varlen_int(pc);
        //auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        auto & var2 = vars[i_in2];
        assert(var1.kind == TYPEID_INT && var2.kind == TYPEID_INT);
        //out = var1;
        var1.data.integer >>= var2.data.integer;
    }
    CALL_NEXT();
}
OPHANDLER_ABI void op_shrimm(OPHANDLER_ARGS)
{
    pc += (OP_SHRIMM > 0xFF) ? 2 : 1;
    {
        //auto i_out = read_varlen_int(pc);
        auto i_in1 = read_varlen_int(pc);
        auto var2 = read_immediate(pc);
        //auto & out = vars[i_out];
        auto & var1 = vars[i_in1];
        assert(var1.kind == TYPEID_INT && var2.kind == TYPEID_INT);
        //out = var1;
        var1.data.integer >>= var2.data.integer;
    }
    CALL_NEXT();
}

OPHANDLER_ABI void op_returnval(OPHANDLER_ARGS)
{
    pc += (OP_RETURNVAL > 0xFF) ? 2 : 1;
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
    
    Vec<Variable> _vars;
    printf("allocating %zu var-regs....\n", func->num_vars + func->num_regs);
    for (size_t i = 0; i < func->num_vars + func->num_regs; i++)
        _vars.push_back(Variable());
    printf("allocated %zu var-regs\n", _vars.size());
    
    const uint8_t * pc = func->code.data();
    auto vars = _vars.data();
#ifndef DO_NOT_TRACK_INTERPRETER_PREV_OPCODE
    prev_inst = read_op(pc);
#else
    uint16_t prev_inst = read_op(pc);
#endif
    auto global = this;
    opcode_table.t[prev_inst](CALL_ORDER);
    return std::move(retval);
}

inline Variable Interpreter::call_func_by_name(String funcname, Vec<Variable> args)
{
    if (!func_names.count(funcname))
        throw;
    return call_func(funcs[func_names[funcname]], args);
}

#endif // MUALI_INTERPRETER
