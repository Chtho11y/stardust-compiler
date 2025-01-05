#include "ast.h"
#include <map>
#include <functional>

using namespace sd;

bool is_math_op(op_type type){
    return op_type::Add <= type && type <= op_type::Xor;
}

bool is_logic_op(op_type type){
    return op_type::And == type || type == op_type::Or;
}

bool is_uary_op(op_type type){
    return op_type::Pos <= type && type <= op_type::Not;
}

bool is_cmp_op(op_type type){
    return op_type::Eq <= type && type <= op_type::Gt;
}

bool is_assign_op(op_type type){
    return op_type::Assign <= type && type <= op_type::DivEq;
}

bool is_normal_binary_op(op_type type){
    return type <= op_type::DivEq;
}

struct op_impl{
    op_type id;
    var_type_ptr ret;
    std::vector<var_type_ptr> cond;
};

std::map<op_type, std::vector<op_impl>> op_impl_list;

void impl_op(op_type id, int size, var_type_ptr tp, var_type_ptr ret = nullptr){
    op_impl impl;
    impl.id = id;
    impl.ret = ret? ret: tp;

    for(int i = 0; i < size; ++i)
        impl.cond.push_back(tp);

    op_impl_list[id].push_back(impl);    
}

void impl_prim_op(op_type id, int size, var_type_ptr ret = nullptr){
    auto& prims = sd::get_prim_list();

    for(auto t: prims){
        impl_op(id, size, t, ret);
    }
}

void impl_int_op(op_type id, int size, var_type_ptr ret = nullptr){
    auto& prims = sd::get_prim_list();

    for(auto t: prims){
        if(t->kind != sd::PrimType::Int)
            continue;

        impl_op(id, size, t, ret);
    }
}

void op_impl_init(){
    for(int i = (int)op_type::Add; i < (int)op_type::Mod; ++i)
        impl_prim_op((op_type)i, 2);
    
    auto logic_op = {op_type::And, op_type::Or, op_type::Not};
    for(auto op: logic_op)
        impl_op(op, 1 + (op != op_type::Not), PrimType::get_bool());
    
    for(int i = (int)op_type::BitAnd; i <= (int)op_type::Xor; ++i)
        impl_int_op((op_type)i, 2);

    for(int i = (int)op_type::Eq; i <= (int)op_type::Gt; ++i)
        impl_prim_op((op_type)i, 2, PrimType::get_bool());
    
    impl_int_op(op_type::Mod, 2);
    impl_prim_op(op_type::Pos, 1);
    impl_prim_op(op_type::Neg, 1);
}

var_type_ptr as_ptr_type(var_type_ptr tp){
    tp = tp->decay();
    if(tp->is_ptr())
        return tp;
    if(tp->is_array()){
        auto arr = dyn_ptr_cast<ArrayType>(tp);
        auto ptr = PointerType::get(arr->subtype);
        ptr->subtype = arr->subtype;
        return ptr;
    }
    throw std::invalid_argument("not support");
}

var_type_ptr sp_op_eval(OperatorNode* ast, std::vector<var_type_ptr>& args){
    auto op = ast->type;
    switch (op)
    {
    case op_type::Comma: {
        return args.back();
    }

    case op_type::At:{
        auto op_l = args[0]->decay();
        if(op_l->is_ptr()){
            auto ptr = dyn_ptr_cast<PointerType>(op_l);
            expect_ret_type(ast->ch[0], ptr);
            expect_ret_type(ast->ch[1], PrimType::get_int(64));
            return RefType::get(ptr->subtype);
        }else if(op_l->is_array()){
            auto arr = dyn_ptr_cast<ArrayType>(op_l);
            expect_ret_type(ast->ch[1], PrimType::get_int(64));
            return RefType::get(arr->subtype);
        }else{
            append_mismatch_op_error("array or pointer", op_l, ast->ch[0]->loc);
            // append_error("Expression should be array or pointer type, but it is "+ op_l->to_string(), loc);
            return ErrorType::get();
        }
    }

    case op_type::Call:{
        auto fn = args[0]->decay();
        if(fn->is_func_ptr()){
            fn = dyn_ptr_cast<PointerType>(fn)->subtype;
            expect_ret_type(ast->ch[0], args[0]->decay());
        }
        if(!fn->is_func() && !fn->is_mem_func()){
            // append_error("Expression should be function type, but it is " + fn->to_string(), loc);
            append_mismatch_op_error("function", fn, ast->ch[0]->loc);
            return ErrorType::get();
        }
        auto func = fn->cast<FuncType>();
        if(!func->is_callable(args[1])){
            append_call_error(func, args[1], ast->loc);
            // append_error("Cannot call function \'" + func->to_string() + "\' with args: \'" + args[1]->to_string() + "\'", loc);
        }

        for(size_t i = 0; i < func->param_list.size(); ++i){
            expect_ret_type(ast->ch[1]->ch[i], func->param_list[i]);
        }

        auto mem = dyn_ptr_cast<TupleType>(args[1]);
        for(size_t i = func->param_list.size(); i < mem->members.size(); ++i){
            expect_ret_type(ast->ch[1]->ch[i], upper_type(mem->members[i]));
        }
        return func->ret_type;
    }

    // case op_type::Access:{
    //     if(!args[0]->is_type(VarType::Struct))
    //         return ErrorType::get();
    // }

    case op_type::Convert:{
        if(!args[0]->is_force_convertable(args[1])){
            append_force_convert_error(args[0]->decay(), args[1]->decay(), ast->loc);
            // append_error("Cannot convert type \'" + args[0]->to_string() + "\' to \'" + args[1]->to_string() + "\'.", loc);
        }
        return args[1];
    }

    case op_type::Assign:{
        expect_ret_type(ast->ch[1], args[0]->decay());
        if(!args[0]->is_ref()){
            append_assign_error("lvalue required as left operand of assignment", ast->loc_list[0]);
            return ErrorType::get();
        }
        if(args[0]->decay()->is_func()){
            append_assign_error("Cannot assign to function.", ast->loc_list[0]);
            return ErrorType::get();
        }
        auto ref = dyn_ptr_cast<RefType>(args[0]);
        if(ref->is_cnst){
            append_assign_error("Cannot modify a const value.", ast->loc_list[0]);
            return ErrorType::get();
        }
        return args[0];
    }

    case op_type::AddEq:case op_type::SubEq:case op_type::MulEq:case op_type::DivEq:{
        expect_ret_type(ast->ch[1], args[0]->decay());
        if(!args[0]->is_ref()){
            append_assign_error("lvalue required as left operand of assignment", ast->loc_list[0]);
            return args[0];
        }
        if(!args[0]->decay()->is_prim())
            break;
        return args[0];
    }

    case op_type::Add:{
        if(args[0]->decay()->is_ptr() || args[0]->decay()->is_array()){
            if(args[1]->decay()->is_convertable(PrimType::get_int(64))){
                expect_ret_type(ast->ch[1], args[1]->decay());
                return expect_ret_type(ast->ch[0], as_ptr_type(args[0]));   
            }   
        }else if(args[1]->decay()->is_ptr() || args[1]->decay()->is_array()){
            if(args[0]->decay()->is_convertable(PrimType::get_int(64))){
                expect_ret_type(ast->ch[0], args[0]->decay());
                return expect_ret_type(ast->ch[1], as_ptr_type(args[1]));
            }    
        }
        break;
    }

    case op_type::Sub:{
        auto l = args[0]->decay(), r = args[1]->decay();
        if(l->is_ptr() || l->is_array()){
            if(r->is_convertable(PrimType::get_int(64))){
                expect_ret_type(ast->ch[0], as_ptr_type(l));
                expect_ret_type(ast->ch[1], r);
                return l;         
            }
            if(r->is_convertable(l) || l->is_convertable(r)){
                auto comm = as_ptr_type(greater_type(l, r));
                expect_ret_type(ast->ch[0], comm);
                expect_ret_type(ast->ch[1], comm);
                return PrimType::get_int(64);
            }
        }
        break;
    }

    case op_type::Ref:{
        if(!args[0]->is_ref()){
            append_ref_error("lvalue required as operand of reference", ast->loc);
            return ErrorType::get();
        }
        auto res = PointerType::get(args[0]->decay());
        return res;
    }

    case op_type::DeRef:{
        auto op = args[0]->decay();
        if(!op->is_ptr()){
            append_ref_error("pointer type required as operand of dereference", ast->loc);
            return ErrorType::get();
        }
        auto sub = dyn_ptr_cast<PointerType>(op)->subtype;
        if(sub->is_void()){
            append_ref_error("cannot dereference pointer of void type.", ast->loc);
            return ErrorType::get();
        }
        expect_ret_type(ast->ch[0], op);
        return RefType::get(sub);
    }

    case op_type::Eq: case op_type::Neq: 
    case op_type::Le: case op_type::Lt: case op_type::Ge: case op_type::Gt:{
        auto op_l = args[0]->decay(), op_r = args[1]->decay();
        if((op_l->is_array() || op_l->is_ptr())&&(op_r->is_array() || op_r->is_ptr())) {
            auto comm = greater_type(op_l, op_r);
            if(comm->is_void())
                break;
            expect_ret_type(ast->ch[0], comm);
            expect_ret_type(ast->ch[1], comm);
            return PrimType::get_bool();
        }
        break;
    }
    
    default:
        break;
    }
    append_nomatch_op_error("no match operator: " + get_op_name(op) + TupleType(args).to_string(), ast->loc_list[0]);
    return ErrorType::get();
}

var_type_ptr op_type_eval(OperatorNode* ast, std::vector<var_type_ptr> args){
    auto op = ast->type;
    for(auto& tp: args)
        if(tp->is_error())
            return ErrorType::get();

    auto& impl_list = op_impl_list[op];
    int cnt = 0, weight = 114514, id = -1, cur = 0;
    for(auto impl: impl_list){
        int nw = 0;
        if(impl.cond.size() != args.size())
            continue;
        for(size_t i = 0; i < args.size(); ++i){
            if(args[i]->decay() != impl.cond[i]){
                if(args[i]->decay()->is_convertable(impl.cond[i]))
                    nw += 1;
                else nw += 114514;
            }
        }
        if(nw < weight){
            id = cur;
            cnt = 0;
            weight = nw;
        }else if(nw == weight){
            cnt++;
        }
        ++cur;
    }
    if(id == -1){
        return sp_op_eval(ast, args);
    }else if(cnt > 1){
        append_nomatch_op_error("Ambiguous call for operator: "+ get_op_name(op) + ".", ast->loc);
        return ErrorType::get();
    }else{
        for(int i = 0; i < ast->ch.size(); ++i)
            expect_ret_type(ast->ch[i], impl_list[id].cond[i]);
        return impl_list[id].ret;
    }
}