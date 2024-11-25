#include "ast.h"
#include <map>
#include <functional>

// enum class op_type{
//     Add, Sub, Mul, Div, Mod, And, Or,
//     BitAnd, BitOr, Xor, Eq, Neq, Le, Ge, Lt, Gt,
//     Assign, At, Call, Comma, Access,
//     Pos, Neg, Not, Convert
// };


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
    auto& prims = get_prim_list();

    for(auto t: prims){
        impl_op(id, size, t, ret);
    }
}

void impl_int_op(op_type id, int size, var_type_ptr ret = nullptr){
    auto& prims = get_prim_list();

    for(auto t: prims){
        if(t->pr_kind != PrimType::Int)
            continue;

        impl_op(id, size, t, ret);
    }
}

void op_impl_init(){
    for(int i = (int)op_type::Add; i <= (int)op_type::Mod; ++i)
        impl_prim_op((op_type)i, 2);
    
    auto logic_op = {op_type::And, op_type::Or, op_type::Not};
    for(auto op: logic_op)
        impl_op(op, 2, get_type("bool"));
    
    for(int i = (int)op_type::BitAnd; i <= (int)op_type::Xor; ++i)
        impl_int_op((op_type)i, 2);

    for(int i = (int)op_type::Eq; i <= (int)op_type::Gt; ++i)
        impl_int_op((op_type)i, 2, get_type("bool"));
    
    impl_prim_op(op_type::Pos, 2);
    impl_prim_op(op_type::Neg, 2);
}

// #include<iostream>

var_type_ptr sp_op_eval(op_type op, std::vector<var_type_ptr>& args){
    switch (op)
    {
    case op_type::Comma: return args.back();

    case op_type::At:{
        auto op_l = decay(args[0]);
        if(op_l->is_ptr()){
            auto ptr = std::dynamic_pointer_cast<PointerType>(op_l);
            require_convertable(args[1], get_type("uint64"));
            return ref_type(ptr->subtype);
        }else if(op_l->is_array()){
            auto arr = std::dynamic_pointer_cast<ArrayType>(op_l);
            require_convertable(args[1], get_type("uint64"));
            return ref_type(arr->subtype);
        }else{
            return get_type("#err");
        }
    }

    case op_type::Call:{
        auto fn = decay(args[0]);
        if(!fn->is_type(VarType::Func))
            return get_type("#err");
        auto func = std::dynamic_pointer_cast<FuncType>(fn);
        return func->ret_type;
    }

    // case op_type::Access:{
    //     if(!args[0]->is_type(VarType::Struct))
    //         return get_type("#err");
    // }

    case op_type::Convert:{
        if(!is_force_convertable(args[0], args[1]))
            append_error("Cannot convert type \'" + args[0]->to_string() + "\' to \'" + args[1]->to_string() + "\'.");
        return args[1];
    }

    case op_type::Assign:{
        require_convertable(args[1], args[0]);
        if(!args[0]->is_ref()){
            append_error("lvalue required as left operand of assignment");
        }
        return args[0];
    }

    case op_type::Ref:{
        if(!args[0]->is_ref()){
            append_error("lvalue required as operand of reference");
            return get_type("#err");
        }
        auto res = std::make_shared<PointerType>();
        res->subtype = decay(args[0]);
        return res;
    }

    case op_type::DeRef:{
        auto op = decay(args[0]);
        if(!op->is_ptr()){
            append_error("pointer type required as operand of dereference");
            return get_type("#err");
        }
        auto sub = std::dynamic_pointer_cast<PointerType>(op)->subtype;
        if(sub->is_void()){
            append_error("cannot dereference pointer of void type.");
            return get_type("#err");
        }        
        return ref_type(sub);
    }


    // case op_type::Eq: case op_type::Neq:{
        
    //     return get_type("bool");
    // }
    
    default:
        return get_type("#err");
    }
}

var_type_ptr op_type_eval(op_type op, std::vector<var_type_ptr> args){
    auto& impl_list = op_impl_list[op];
    int cnt = 0, weight = 114514, id = -1, cur = 0;
    for(auto impl: impl_list){
        int nw = 0;
        if(impl.cond.size() != args.size())
            continue;
        for(size_t i = 0; i < args.size(); ++i){
            if(!decay(args[i])->is_same(impl.cond[i].get())){
                if(is_convertable(args[i], impl.cond[i]))
                    nw += 1;
                else nw += 114514;
            }
        }
        // if(op == op_type::Gt)
        //     std::cout << impl.ret->to_string() << ": " << nw << std::endl;
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
        return sp_op_eval(op, args);
    }else if(cnt > 1){
        append_error("Ambiguous call for operator.");
        return get_type("#err");
    }else{
        return impl_list[id].ret;
    }
}