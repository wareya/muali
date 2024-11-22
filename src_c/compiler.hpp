#ifndef BBEL_COMPILER
#define BBEL_COMPILER

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
    Option<size_t> global_var;
    Option<size_t> func_name;
    
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
        return !!imm_int || !!imm_float || !!imm_string;
    }
    
    static ExprInfo from_int(int64_t n)
    {
        ExprInfo ret;
        ret.imm_int = {n};
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
    size_t var_index = 0;
    size_t vardec_count = 0;
    
    size_t next_reg = 0;
    Vec<size_t> freed_registers;
    
    size_t add_var(String name)
    {
        size_t _var_index = var_index++;
        scopes.back().insert(name, _var_index);
        printf("added var %s\n", name.data());
        return _var_index;
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
            return freed_registers.pop_back();
        return next_reg++;
    }
};

template<typename T>
static inline void push_varlen_int(T & buffer, size_t myint)
{
    while (myint >= 255)
    {
        myint -= 255;
        buffer.push_back(255);
    }
    buffer.push_back(myint);
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
        buffer.push_back(imm);
        buffer.push_back(imm >> 8);
        buffer.push_back(imm >> 16);
        buffer.push_back(imm >> 24);
        buffer.push_back(imm >> 32);
        buffer.push_back(imm >> 40);
        buffer.push_back(imm >> 48);
        buffer.push_back(imm >> 56);
    }
    else
        assert(((void)"TODO string immediate", 0));
}

static inline Option<ExprInfo> compile_func_inner(Shared<ASTNode> node, Shared<Function> func, FuncCompInfo & info, const Global & global)
{
    assert(node->text);
    printf("%s\n", node->text->data());
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
            info.add_var(*node->children[0]->children[0]->text);
        else
        {
            auto _expr = compile_func_inner(node->children[1], func, info, global);
            assert(_expr);
            auto expr = *_expr;
            
            size_t var_index = info.add_var(*node->children[0]->children[0]->text);
            
            if (expr.is_immediate())
            {
                func->code.push_back(OP_SETIMM);
                push_varlen_int(func->code, var_index);
                push_immediate(func->code, expr);
            }
            else if (expr.is_var_reg())
            {
                func->code.push_back(OP_SET);
                push_varlen_int(func->code, var_index);
                push_varlen_int(func->code, *expr.var_reg);
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
        size_t var_index = info.look_up(*node->children[0]->text);
        if (var_index == -1ULL)
        {
            printf("failed to find variable %s\n", node->children[0]->text->data());
            throw;
        }
        
        if (expr.is_immediate())
        {
            func->code.push_back(OP_SETIMM);
            push_varlen_int(func->code, var_index);
            push_immediate(func->code, expr);
        }
        else if (expr.is_var_reg())
        {
            func->code.push_back(OP_SET);
            push_varlen_int(func->code, var_index);
            push_varlen_int(func->code, *expr.var_reg);
        }
        else
            assert(((void)"TODO", 0));
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
        return {ExprInfo::from_var_reg(var_index)};
    }
    else if (node->text->starts_with("base_unexp"))
    {
        if (node->children.size() == 1)
            return compile_func_inner(node->children[0], func, info, global);
        else
        {
            auto ret = compile_func_inner(node->children[0], func, info, global);
            assert(((void)"TODO (base unexp)", 0));
        }
    }
    else if (node->text->starts_with("base_binexp"))
    {
        if (node->children.size() == 1)
            return compile_func_inner(node->children[0], func, info, global);
        else
        {
            auto ret = compile_func_inner(node->children[0], func, info, global);
            assert(((void)"TODO (base binexp)", 0));
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
            assert(((void)"TODO", expr1.is_var_reg()));
            
            auto _expr2 = compile_func_inner(node->children[2], func, info, global);
            assert(_expr2);
            auto expr2 = *_expr2;
            
            auto & op = *node->children[1]->children[0]->text;
            
            printf("%s\n", op.data());
            uint8_t op_byte;
            if (op == "+" && !expr2.is_immediate())
                op_byte = OP_ADD;
            else if (op == "+" && expr2.is_immediate())
                op_byte = OP_ADDIMM;
            else
                assert(((void)"TODO (binexp)", 0));
            
            puts("a");
            
            if (expr1.is_var_reg())
                info.free_register(*expr1.var_reg); // does nothing if passed a variable
            if (expr2.is_var_reg())
                info.free_register(*expr2.var_reg);
            
            puts("b");
            
            size_t out_reg = info.alloc_register();
            
            puts("c");
            func->code.push_back(op_byte);
            push_varlen_int(func->code, out_reg);
            puts("d");
            push_varlen_int(func->code, *expr1.var_reg);
            puts("e");
            if (expr2.is_var_reg())
                push_varlen_int(func->code, *expr2.var_reg);
            else
                push_immediate(func->code, expr2);
            puts("f");
            
            return {ExprInfo::from_var_reg(op_byte)};
        }
    }
    else if (node->text->starts_with("return"))
    {
        if (node->children.size() == 0)
            func->code.push_back(OP_RETURNNULL);
        else
        {
            auto _expr = compile_func_inner(node->children[0], func, info, global);
            assert(_expr);
            auto expr = *_expr;
            
            if (expr.is_immediate())
            {
                func->code.push_back(OP_RETURNIMM);
                push_immediate(func->code, expr);
            }
            else if (expr.is_var_reg())
            {
                func->code.push_back(OP_RETURNVAL);
                push_varlen_int(func->code, *expr.var_reg);
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
        printf("%zd\n", n);
        //assert(((void)"TODO", 0));
        return {ExprInfo::from_int(n)};
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
    return ret;
}
static inline Global compile_root(Shared<ASTNode> root)
{
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

#endif // BBEL_COMPILER
