#ifndef MUALI_COMPILER
#define MUALI_COMPILER
        
#define FOREACH_USE_IMM

#include <cassert>
#include <cstring>
#include <cstdint>

#include "types.hpp"
#include "grammar.hpp"
#include "vm_common.hpp"

struct ExprInfo {
    Option<size_t> var_reg;
    Option<int64_t> imm_int;
    Option<double> imm_float;
    Option<String> imm_string;
    Option<uint8_t> imm_null;
    Option<size_t> global_var;
    Option<size_t> func_name;
    
    TypeId static_type = TYPEID_INVALID;
    
    bool is_var_reg()
    {
        return !!var_reg;
    }
    bool is_global()
    {
        return !!global_var;
    }
    bool is_func()
    {
        return !!func_name;
    }
    bool is_immediate()
    {
        return !!imm_int || !!imm_float || !!imm_string || !!imm_null;
    }
    
    static ExprInfo of_null()
    {
        ExprInfo ret;
        ret.imm_null = {0};
        return ret;
    }
    
    static ExprInfo from_int(int64_t n)
    {
        ExprInfo ret;
        ret.imm_int = {n};
        return ret;
    }
    static ExprInfo from_float(double n)
    {
        ExprInfo ret;
        ret.imm_float = {n};
        return ret;
    }
    static ExprInfo from_var_reg(size_t n)
    {
        ExprInfo ret;
        ret.var_reg = {n};
        return ret;
    }
};

struct FuncCompInfo {
    Vec<ListMap<String, size_t>> scopes;
    ListMap<size_t, TypeId> var_types;
    size_t var_index = 0;
    size_t vardec_count = 0;
    
    size_t next_reg = 0;
    Vec<size_t> freed_registers;
    
    void push_scope()
    {
        scopes.push_back({});
    }
    void pop_scope()
    {
        scopes.pop_back();
    }
    
    size_t add_var(String name)
    {
        size_t _var_index = var_index++;
        assert(_var_index < vardec_count);
        scopes.back().insert(name, _var_index);
        //printf("added var %s\n", name.data());
        return _var_index;
    }
    
    void add_var_type(size_t index, TypeId type)
    {
        assert(index < vardec_count);
        var_types.insert(index, type);
    }
    
    size_t get_var_type(size_t index)
    {
        if (var_types.count(index))
            return var_types[index];
        return TYPEID_INVALID;
    }
    
    size_t look_up(String & name)
    {
        for (size_t i = scopes.size(); i > 0; i -= 1)
        {
            if (scopes[i-1].count(name))
                return scopes[i-1][name];
        }
        return -1;
    }
    TypeId parse_type(String & name)
    {
        if (name == "int")
            return TYPEID_INT;
        if (name == "float")
            return TYPEID_FLOAT;
        return TYPEID_INVALID;
    }
    
    // does nothing if passed a variable
    void free_register(size_t reg)
    {
        if (reg < vardec_count)
            return;
        freed_registers.push_back(reg);
    }
    
    size_t alloc_register()
    {
        if (freed_registers.size())
        {
            //printf("reusing reg %zu\n", freed_registers.back());
            return freed_registers.pop_back();
        }
        //printf("using new reg %zu\n", next_reg);
        return next_reg++;
    }
};

template<typename T>
static inline void push_op(T & buffer, uint16_t op)
{
    printf("pushing op %02X...\n", op);
#ifdef OPCODES_ALWAYS_8BIT
    assert(op <= 0xFF);
    buffer.push_back(uint8_t(op));
#else
#ifdef OPCODES_ALWAYS_16BIT
    buffer.push_back(op);
    buffer.push_back(op >> 8);
#else
    assert(op >= 0x80);
    assert((uint32_t)op < (1<<INTERPRETER_OPCODE_TABLE_BITS));
    if (op > 0xFF)
    {
        assert((op & 0xFF) < 0x80);
        buffer.push_back(uint8_t(op));
        buffer.push_back(uint8_t(op >> 8));
    }
    else
        buffer.push_back(uint8_t(op));
#endif
#endif
}

template<typename T>
static inline void push_varlen_int(T & buffer, size_t myint)
{
#ifdef VARLEN_VARREG_8BIT
    assert(myint <= 255);
#endif
#ifdef VARLEN_VARREG_16BIT
    assert(myint < (1<<16));
    buffer.push_back(myint);
    buffer.push_back(myint >> 8);
#else

#ifdef VARLEN_VARREG_SB15LE
    assert(myint < (1<<15));
    if (myint >= 0x80)
    {
        buffer.push_back((myint & 0x7F) | 0x80);
        buffer.push_back((myint >> 7) ^ 1);
    }
    else
        buffer.push_back(myint & 0x7F);
#elif defined VARLEN_VARREG_SB15
    assert(myint < (1<<15));
    buffer.push_back(myint & 0x7F);
    if (myint >= 0x80)
    {
        myint >>= 7;
        buffer.insert_at(buffer.size() - 1, (myint & 0x7F) | 0x80);
    }
#elif defined VARLEN_VARREG_LZ4LIKE
    while (myint >= 255)
    {
        myint -= 255;
        buffer.push_back(255);
    }
    buffer.push_back(myint);
#else // VARLEN_VARREG_LZ4LIKE
    size_t where = buffer.size();
    buffer.push_back(myint & 0x7F);
    while (myint >= 0x80)
    {
        myint >>= 7;
    #ifdef VARLEN_VARREG_LEB128
        (void)where;
        buffer.back() |= 0x80;
        buffer.push_back(myint & 0x7F);
    #else
        buffer.insert_at(where, (myint & 0x7F) | 0x80);
    #endif
    }
#endif // VARLEN_VARREG_LZ4LIKE
#endif
}

template<typename T>
static inline void push_u16(T & buffer, uint16_t imm)
{
    buffer.push_back(imm);
    buffer.push_back(imm >> 8);
}
template<typename T>
static inline void push_u32(T & buffer, uint32_t imm)
{
    buffer.push_back(imm);
    buffer.push_back(imm >> 8);
    buffer.push_back(imm >> 16);
    buffer.push_back(imm >> 24);
}
template<typename T>
static inline void push_u64(T & buffer, uint64_t imm)
{
    buffer.push_back(imm);
    buffer.push_back(imm >> 8);
    buffer.push_back(imm >> 16);
    buffer.push_back(imm >> 24);
    buffer.push_back(imm >> 32);
    buffer.push_back(imm >> 40);
    buffer.push_back(imm >> 48);
    buffer.push_back(imm >> 56);
}
template<typename T>
static inline void push_immediate(T & buffer, ExprInfo myimm)
{
    assert(myimm.is_immediate());
    if (!!myimm.imm_int || !!myimm.imm_float)
    {
        uint64_t imm = 0;
        if (myimm.imm_int)
        {
            buffer.push_back(TYPEID_INT);
            imm = *myimm.imm_int;
        }
        else
        {
            buffer.push_back(TYPEID_FLOAT);
            memcpy(&imm, &*myimm.imm_float, 8);
        }
        push_u64(buffer, imm);
    }
    else if (myimm.imm_null)
        buffer.push_back(TYPEID_NULL);
    else
        assert(((void)"TODO more types of immediate", 0));
}

static inline Option<ExprInfo> compile_func_inner(Shared<ASTNode> node, Shared<Function> func, FuncCompInfo & info, const Global & global)
{
    assert(node->text);
    //printf("inside of... %s\n", node->text->data());
    if (*node->text == "funcdef")
    {
        //for (auto _node : node->children[1]->children.size())
        func->num_args = node->children[1]->children.size();
        for (auto node : node->children[2]->children)
            compile_func_inner(node, func, info, global);
    }
    else if (*node->text == "statement" || *node->text == "simple_statement")
    {
        //for (auto _node : node->children[1]->children.size())
        compile_func_inner(node->children[0]->children[0], func, info, global);
    }
    else if (*node->text == "vardec")
    {
        if (node->children.size() == 1)
            info.add_var(*node->children[0]->children[0]->children[0]->text);
        else
        {
            auto _expr = compile_func_inner(node->children.back(), func, info, global);
            assert(_expr);
            auto expr = *_expr;
            
            size_t var_index = info.add_var(*node->children[0]->children[0]->children[0]->text);
            if (node->children[0]->children.size() == 2)
            {
                auto type = info.parse_type(*node->children[0]->children[1]->children[0]->text);
                info.add_var_type(var_index, type);
            }
            
            if (expr.is_immediate())
            {
                push_op(func->code, OP_SETIMM);
                push_varlen_int(func->code, var_index);
                push_immediate(func->code, expr);
            }
            else if (expr.is_var_reg())
            {
                push_op(func->code, OP_SET);
                push_varlen_int(func->code, var_index);
                push_varlen_int(func->code, *expr.var_reg);
                //printf("!!! emitting declaration assignment with %zu...\n", *expr.var_reg);
            }
            else
                assert(((void)"TODO", 0));
        }
    }
    else if (*node->text == "assign")
    {
        assert(node->children.size() == 2);
        
        auto _expr = compile_func_inner(node->children[1], func, info, global);
        assert(_expr);
        auto expr = *_expr;
        
        // TODO support globals
        size_t var_index = info.look_up(*node->children[0]->children[0]->text);
        if (var_index == -1ULL)
        {
            printf("failed to find variable %s\n", node->children[0]->text->data());
            throw;
        }
        
        if (expr.is_immediate())
        {
            push_op(func->code, OP_SETIMM);
            push_varlen_int(func->code, var_index);
            push_immediate(func->code, expr);
        }
        else if (expr.is_var_reg())
        {
            push_op(func->code, OP_SET);
            push_varlen_int(func->code, var_index);
            //printf("!!! emitting normal assignment with %zu...\n", *expr.var_reg);
            push_varlen_int(func->code, *expr.var_reg);
        }
        else
            assert(((void)"TODO", 0));
    }
    else if (*node->text == "inplace_negate")
    {
        // TODO support globals
        size_t var_index = info.look_up(*node->children[0]->children[0]->text);
        if (var_index == -1ULL)
        {
            printf("failed to find variable %s\n", node->children[0]->text->data());
            throw;
        }
        
        if (info.get_var_type(var_index) == TYPEID_FLOAT)
            push_op(func->code, OP_NEGATE_F);
        else
            push_op(func->code, OP_NEGATE);
        push_varlen_int(func->code, var_index);
    }
    else if (*node->text == "name")
    {
        // TODO support globals
        size_t var_index = info.look_up(*node->children[0]->text);
        if (var_index == -1ULL)
        {
            printf("failed to find variable %s\n", node->children[0]->text->data());
            throw;
        }
        //printf("looked up %s.... found at %zu!!!\n", node->children[0]->text->data(), var_index);
        auto ret = ExprInfo::from_var_reg(var_index);
        ret.static_type = info.get_var_type(var_index);
        return {ret};
    }
    else if (node->text->starts_with("base_unexp"))
    {
        if (node->children.size() == 1)
            return compile_func_inner(node->children[0], func, info, global);
        else
        {
            assert(((void)"TODO (base unexp)", 0));
        }
    }
    else if (node->text->starts_with("base_binexp"))
    {
        if (node->children.size() == 1)
            return compile_func_inner(node->children[0], func, info, global);
        else
        {
            auto ret = compile_func_inner(node->children.back(), func, info, global);
            auto & text = node->children[0]->children[0]->text;
            if (!!ret->imm_int || !!ret->imm_float)
            {
                if (*text == "+")
                    return ret;
                else if (*text == "-")
                {
                    if (ret->imm_int)
                        *ret->imm_int = -*ret->imm_int;
                    else
                        *ret->imm_float = -*ret->imm_float;
                    return ret;
                }
                else
                    assert(0);
            }
            else
            {
                if (*text == "+")
                    return ret; // FIXME: check that the type is int, float, or bool
                else if (*text == "-")
                {
                    assert(ret->is_var_reg());
                    if (ret->static_type == TYPEID_FLOAT)
                        push_op(func->code, OP_NEGATE_F);
                    else
                        push_op(func->code, OP_NEGATE);
                    push_varlen_int(func->code, *ret->var_reg);
                    return ret;
                }
                else
                    assert(0);
            }
        }
    }
    else if (node->text->starts_with("binexp_"))
    {
        if (node->children.size() == 1)
            return compile_func_inner(node->children[0], func, info, global);
        else
        {
            auto _expr1 = compile_func_inner(node->children[0], func, info, global);
            assert(_expr1);
            auto expr1 = *_expr1;
            
            //assert(((void)"TODO", expr1.is_var_reg()));
            
            auto _expr2 = compile_func_inner(node->children[2], func, info, global);
            assert(_expr2);
            auto expr2 = *_expr2;
            
            // TODO: constant folding
            //if (expr1.is_immediate() && expr2.is_immediate())
            //{
            //    // ...
            //}
            
            auto & op = *node->children[1]->children[0]->text;
            
            //printf("%s\n", op.data());
            uint16_t opcode;
            if (op == "+" && !expr2.is_immediate())
            {
                if (expr1.static_type == TYPEID_FLOAT && expr2.static_type == TYPEID_FLOAT)
                    opcode = OP_ADD_FF;
                else if (expr1.static_type == TYPEID_FLOAT)
                    opcode = OP_ADD_F;
                else
                    opcode = OP_ADD;
            }
            else if (op == "+" && expr2.is_immediate())
                opcode = OP_ADDIMM;
            else if (op == "-" && !expr2.is_immediate())
                opcode = OP_SUB;
            else if (op == "-" && expr2.is_immediate())
                opcode = OP_SUBIMM;
            else if (op == "*" && !expr2.is_immediate())
                opcode = OP_MUL;
            else if (op == "*" && expr2.is_immediate())
                opcode = OP_MULIMM;
            else if (op == "/" && !expr2.is_immediate())
            {
                if (expr1.static_type == TYPEID_FLOAT && expr2.static_type == TYPEID_FLOAT)
                    opcode = OP_DIV_FF;
                else if (expr1.static_type == TYPEID_FLOAT && expr2.static_type == TYPEID_INT)
                    opcode = OP_DIV_FI;
                else if (expr1.static_type == TYPEID_FLOAT)
                    opcode = OP_DIV_F;
                else
                    opcode = OP_DIV;
            }
            else if (op == "/" && expr2.is_immediate())
                opcode = OP_DIVIMM;
            else if (op == "<<" && !expr2.is_immediate())
                opcode = OP_SHL;
            else if (op == "<<" && expr2.is_immediate())
            {
                if (expr1.is_var_reg() && expr1.static_type == TYPEID_INT)
                    opcode = OP_SHLIMM_I;
                else
                    opcode = OP_SHLIMM;
            }
            else if (op == ">>" && !expr2.is_immediate())
                opcode = OP_SHR;
            else if (op == ">>" && expr2.is_immediate())
                opcode = OP_SHRIMM;
            else
                assert(((void)"TODO (binexp)", 0));
            
            if (expr1.is_var_reg())
                info.free_register(*expr1.var_reg); // does nothing if passed a variable
            
            size_t out_reg = info.alloc_register();
            auto ret = ExprInfo::from_var_reg(out_reg);
            
            if (!expr1.is_var_reg())
            {
                if (expr1.is_immediate())
                {
                    push_op(func->code, OP_SETIMM);
                    push_varlen_int(func->code, out_reg);
                    push_immediate(func->code, expr1);
                    ret.static_type = expr1.static_type;
                    expr1 = ret;
                }
                else
                    assert(((void)"tried to operate on an unsupported type of immediate", 0));
            }
            else if (*expr1.var_reg != out_reg)
            {
                if (expr1.static_type == TYPEID_INT)
                    push_op(func->code, OP_SET_I);
                else if (expr1.static_type == TYPEID_FLOAT)
                    push_op(func->code, OP_SET_F);
                else
                    push_op(func->code, OP_SET);
                push_varlen_int(func->code, out_reg);
                push_varlen_int(func->code, *expr1.var_reg);
                //printf("????? %d %d\n", out_reg, *expr1.var_reg);
                //out_reg.static_type = expr1.static_type;
            }
            
            if (expr2.imm_int)
            {
                if (op == "-" && *expr2.imm_int == 1)
                {
                    if (expr1.static_type == TYPEID_INT)
                        push_op(func->code, OP_DECI_INT);
                    else
                        push_op(func->code, OP_DECI);
                    push_varlen_int(func->code, out_reg);
                    auto ret = ExprInfo::from_var_reg(out_reg);
                    ret.static_type = expr1.static_type;
                    return {ret};
                }
                if (op == "+" && *expr2.imm_int == 1)
                {
                    push_op(func->code, OP_INCI);
                    push_varlen_int(func->code, out_reg);
                    auto ret = ExprInfo::from_var_reg(out_reg);
                    ret.static_type = expr1.static_type;
                    return {ret};
                }
            }
            
            push_op(func->code, opcode);
            push_varlen_int(func->code, out_reg);
            
            if (expr2.is_var_reg())
                push_varlen_int(func->code, *expr2.var_reg);
            else if (expr2.is_immediate())
                push_immediate(func->code, expr2);
            else
                assert(((void)"TODO (binexp non-var/imm case)", 0));
            
            if (expr2.is_var_reg())
                info.free_register(*expr2.var_reg);
            
            if (expr1.static_type == TYPEID_FLOAT)
                ret.static_type = TYPEID_FLOAT;
            if (expr1.static_type == TYPEID_INT)
                ret.static_type = TYPEID_INT;
            
            return ret;
        }
    }
    else if (*node->text == "assign_binop")
    {
        // TODO support globals
        size_t var_index = info.look_up(*node->children[0]->children[0]->text);
        
        auto _expr2 = compile_func_inner(node->children[2], func, info, global);
        assert(_expr2);
        auto expr2 = *_expr2;
        
        // TODO: constant folding
        //if (expr1.is_immediate() && expr2.is_immediate())
        //{
        //    // ...
        //}
        
        auto & op = *node->children[1]->children[0]->text;
        
        //printf("%s\n", op.data());
        uint16_t opcode;
        if (op == "+=" && !expr2.is_immediate())
        {
            if (info.get_var_type(var_index) == TYPEID_FLOAT && expr2.static_type == TYPEID_FLOAT)
                opcode = OP_ADD_FF;
            else if (info.get_var_type(var_index) == TYPEID_FLOAT)
                opcode = OP_ADD_F;
            else
                opcode = OP_ADD;
        }
        else if (op == "+=" && expr2.is_immediate())
            opcode = OP_ADDIMM;
        else if (op == "-=" && !expr2.is_immediate())
            opcode = OP_SUB;
        else if (op == "-=" && expr2.is_immediate())
            opcode = OP_SUBIMM;
        else if (op == "*=" && !expr2.is_immediate())
            opcode = OP_MUL;
        else if (op == "*=" && expr2.is_immediate())
        {
            if (info.get_var_type(var_index) == TYPEID_FLOAT && expr2.imm_float && *expr2.imm_float == -1.0)
            {
                push_op(func->code, OP_NEGATE_F);
                push_varlen_int(func->code, var_index);
                goto out;
            }
            else
                opcode = OP_MULIMM;
        }
        else if (op == "/=" && !expr2.is_immediate())
            opcode = OP_DIV;
        else if (op == "/=" && expr2.is_immediate())
            opcode = OP_DIVIMM;
        else
            assert(((void)"TODO (binexp)", 0));
        
        push_op(func->code, opcode);
        push_varlen_int(func->code, var_index);
        //push_varlen_int(func->code, var_index);
        
        if (expr2.is_var_reg())
            push_varlen_int(func->code, *expr2.var_reg);
        else if (expr2.is_immediate())
            push_immediate(func->code, expr2);
        else
            assert(((void)"TODO (binexp non-var/imm case)", 0));
        
        if (expr2.is_var_reg())
            info.free_register(*expr2.var_reg);
        out: {}
    }
    else if (node->text->starts_with("return"))
    {
        if (node->children.size() == 0)
        {
            push_op(func->code, OP_RETURNIMM);
            push_immediate(func->code, ExprInfo::of_null());
        }
        else
        {
            auto _expr = compile_func_inner(node->children[0], func, info, global);
            assert(_expr);
            auto expr = *_expr;
            
            if (expr.is_immediate())
            {
                push_op(func->code, OP_RETURNIMM);
                push_immediate(func->code, expr);
            }
            else if (expr.is_var_reg())
            {
                push_op(func->code, OP_RETURNVAL);
                push_varlen_int(func->code, *expr.var_reg);
                info.free_register(*expr.var_reg);
            }
            else
                assert(((void)"TODO assign to value slot", 0));
        }
    }
    else if (*node->text == "expr" || *node->text == "simple_expr")
    {
        return compile_func_inner(node->children[0], func, info, global);
    }
    else if (*node->text == "int")
    {
        int64_t n = strtoll(node->children[0]->text->data(), 0, 10);
        //printf("%zd\n", n);
        //assert(((void)"TODO", 0));
        return {ExprInfo::from_int(n)};
    }
    else if (*node->text == "float")
    {
        double n = strtod(node->children[0]->text->data(), 0);
        //assert(((void)"TODO", 0));
        return {ExprInfo::from_float(n)};
    }
    else if (*node->text == "block" || *node->text == "simple_block")
    {
        info.push_scope();
        for (auto node : node->children)
            compile_func_inner(node, func, info, global);
        info.pop_scope();
    }
    else if (*node->text == "foreach")
    {
        info.push_scope();
        
        size_t var_index = info.add_var(*node->children[0]->children[0]->children[0]->text);
        if (node->children[0]->children.size() == 2)
        {
            auto type = info.parse_type(*node->children[0]->children[1]->children[0]->text);
            info.add_var_type(var_index, type);
        }
        
        size_t n = 1;
        if (node->children.size() == 4)
            n = 2;
        
        auto _expr = compile_func_inner(node->children[n], func, info, global);
        assert(_expr);
        auto expr = *_expr;
        
        #ifndef FOREACH_USE_IMM
        size_t t_var_index = info.add_var("");
        push_op(func->code, OP_SETIMM);
        push_varlen_int(func->code, t_var_index);
        push_immediate(func->code, expr);
        #endif
        
        if (expr.imm_int)
        {
            if (info.get_var_type(var_index) != TYPEID_INVALID)
            {
                if (info.get_var_type(var_index) != TYPEID_INT)
                    assert(((void)"type of foreach variable with int range must also be an int", 0));
            }
            else
                info.add_var_type(var_index, TYPEID_INT);
            
            if (node->children.size() == 4)
            {
                auto _expr = compile_func_inner(node->children[1], func, info, global);
                assert(_expr);
                auto expr = *_expr;
                
                *expr.imm_int -= 1;
                push_op(func->code, OP_SETIMM);
                push_varlen_int(func->code, var_index);
                push_immediate(func->code, expr);
            }
            else
            {
                push_op(func->code, OP_SETZEROI);
                push_varlen_int(func->code, var_index);
                push_op(func->code, OP_DECI);
                push_varlen_int(func->code, var_index);
            }
            
            push_op(func->code, OP_J);
            size_t offset_pos = func->code.size();
            push_u32(func->code, 0);
            
            compile_func_inner(node->children.back(), func, info, global);
            
            //push_op(func->code, OP_INCI);
            //func->code.push_back(var_index);
            
            int64_t diff = (ptrdiff_t)func->code.size() - (ptrdiff_t)(offset_pos + 4);
            assert(diff >= -2147483647 && diff <= 2147483647);
            int32_t diff32 = diff;
            memcpy(func->code.data() + offset_pos, &diff32, 4);
            //int16_t diff16 = diff;
            //memcpy(func->code.data() + offset_pos, &diff16, 2);
            //int8_t diff8 = diff;
            //memcpy(func->code.data() + offset_pos, &diff8, 1);
            
            #ifndef FOREACH_USE_IMM
            push_op(func->code, OP_JINCILT);
            push_varlen_int(func->code, var_index);
            push_varlen_int(func->code, t_var_index);
            #else
            if (info.get_var_type(var_index) == TYPEID_INT)
                push_op(func->code, OP_JINCILTIMM_INT);
            else
                push_op(func->code, OP_JINCILTIMM);
            push_varlen_int(func->code, var_index);
            push_u64(func->code, *expr.imm_int);
            #endif
            push_u32(func->code, (ptrdiff_t)offset_pos - (ptrdiff_t)func->code.size());
            //push_u16(func->code, (ptrdiff_t)offset_pos - (ptrdiff_t)func->code.size() + 2);
        }
        else
        {
            //size_t expr_var_index = info.add_var("");
            
            push_op(func->code, OP_SETIMM);
            push_varlen_int(func->code, var_index);
            
            info.free_register(*expr.var_reg);
            assert(((void)"TODO non-immediate-integer foreach", 0));
        }
        
        //assert(((void)"TODO", 0));
        info.pop_scope();
        //assert(((void)"TODO", 0));
    }
    else
    {
        if (node->text)
            printf("culprit: %s\n", node->text->data());
        else
            printf("culprit: (none)\n");
        assert(((void)"TODO", 0));
    }
    
    return {};
}
static inline void count_vardecs(Shared<ASTNode> node, size_t * vardecs)
{
    if (node->text && *node->text == "vardec")
        *vardecs += 1;
    else if (node->text && *node->text == "foreach")
        //*vardecs += 1;
        *vardecs += 2;
    
    for (auto node : node->children)
        count_vardecs(node, vardecs);
}
static inline Option<ExprInfo> compile_func(Shared<ASTNode> node, Shared<Function> func, const Global & global)
{
    FuncCompInfo info;
    info.scopes.push_back({});
    count_vardecs(node, &info.vardec_count);
    info.next_reg = info.vardec_count;
    auto ret = compile_func_inner(node, func, info, global);
    func->num_vars = info.vardec_count;
    func->num_regs = info.next_reg - info.vardec_count;
    func->code.push_back(0x00);
    return ret;
}
static inline void optimize_ast(Shared<ASTNode> & node)
{
    if (!node)
        return;
    
    for (auto & child : node->children)
        optimize_ast(child);
    
    if (node->children.size() == 1 && node->text && *node->text == "expr")
        node = node->children[0];
    if (node->text && *node->text == "assign" && node->children.size() == 2 && *node->children[1]->text == "base_binexp"
        && *node->children[1]->children[0]->children[0]->text == "-"
        && *node->children[1]->children[1]->children[0]->text == *node->children[0]->children[0]->text)
    {
        *node->text = "inplace_negate";
        node->children.erase_at(1);
    }
}
static inline Global compile_root(Shared<ASTNode> root)
{
    optimize_ast(root);
    
    Global global;
    
    for (auto _node : root->children)
    {
        auto & node = _node->children[0];
        if (*node->text == "funcdef")
        {
            auto name = node->children[0]->children[0]->text;
            global.func_names.insert(*name, global.funcs.size());
            global.funcs.push_back(Function{});
        }
        // TODO: set up global variables
    }
    for (auto _node : root->children)
    {
        auto & node = _node->children[0];
        if (*node->text == "funcdef")
        {
            auto name = node->children[0]->children[0]->text;
            auto index = global.func_names[*name];
            compile_func(node, global.funcs[index], global);
        }
    }
    
    return global;
}

#endif // MUALI_COMPILER
