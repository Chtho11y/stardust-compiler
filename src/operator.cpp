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
    for(int i = (int)op_type::Add; i < (int)op_type::Mod; ++i)
        impl_prim_op((op_type)i, 2);
    
    auto logic_op = {op_type::And, op_type::Or, op_type::Not};
    for(auto op: logic_op)
        impl_op(op, 2, get_type("bool"));
    
    for(int i = (int)op_type::BitAnd; i <= (int)op_type::Xor; ++i)
        impl_int_op((op_type)i, 2);

    for(int i = (int)op_type::Eq; i <= (int)op_type::Gt; ++i)
        impl_int_op((op_type)i, 2, get_type("bool"));
    
    impl_int_op(op_type::Mod, 2);
    impl_prim_op(op_type::Pos, 1);
    impl_prim_op(op_type::Neg, 1);
}

var_type_ptr as_ptr_type(var_type_ptr tp){
    tp = decay(tp);
    if(tp->is_ptr())
        return tp;
    if(tp->is_array()){
        auto arr = std::dynamic_pointer_cast<ArrayType>(tp);
        auto ptr = std::make_shared<PointerType>();
        ptr->subtype = arr->subtype;
        return ptr;
    }
    throw std::invalid_argument("not support");
}

var_type_ptr sp_op_eval(op_type op, std::vector<var_type_ptr>& args, Locator loc){
    switch (op)
    {
    case op_type::Comma: return args.back();

    case op_type::At:{
        auto op_l = decay(args[0]);
        if(op_l->is_ptr()){
            auto ptr = std::dynamic_pointer_cast<PointerType>(op_l);
            require_convertable(args[1], get_type("uint64"), loc);
            return ref_type(ptr->subtype);
        }else if(op_l->is_array()){
            auto arr = std::dynamic_pointer_cast<ArrayType>(op_l);
            require_convertable(args[1], get_type("uint64"), loc);
            return ref_type(arr->subtype);
        }else{
            append_mismatch_op_error("array or pointer", op_l, loc);
            // append_error("Expression should be array or pointer type, but it is "+ op_l->to_string(), loc);
            return get_type("#err");
        }
    }

    case op_type::Call:{
        auto fn = decay(args[0]);
        if(fn->is_func_ptr()){
            fn = std::dynamic_pointer_cast<PointerType>(fn)->subtype;
        }
        if(!fn->is_func()){
            // append_error("Expression should be function type, but it is " + fn->to_string(), loc);
            append_mismatch_op_error("function", fn, loc);
            return get_type("#err");
        }

        auto func = std::dynamic_pointer_cast<FuncType>(fn);
        if(!func->is_callable(args[1])){
            append_call_error(func, args[1], loc);
            // append_error("Cannot call function \'" + func->to_string() + "\' with args: \'" + args[1]->to_string() + "\'", loc);
        }
        return func->ret_type;
    }

    // case op_type::Access:{
    //     if(!args[0]->is_type(VarType::Struct))
    //         return get_type("#err");
    // }

    case op_type::Convert:{
        if(!is_force_convertable(args[0], args[1])){
            append_force_convert_error(decay(args[0]), decay(args[1]), loc);
            // append_error("Cannot convert type \'" + args[0]->to_string() + "\' to \'" + args[1]->to_string() + "\'.", loc);
        }
        return args[1];
    }

    case op_type::Assign:{
        require_convertable(args[1], args[0], loc);
        if(!args[0]->is_ref()){
            append_assign_error("lvalue required as left operand of assignment", loc);
        }
        if(decay(args[0])->is_type(VarType::Func)){
            append_assign_error("Cannot assign to function.", loc);
        }
        return args[0];
    }

    case op_type::AddEq:case op_type::SubEq:case op_type::MulEq:case op_type::DivEq:{
        require_convertable(args[1], args[0], loc);
        if(!args[0]->is_ref()){
            append_assign_error("lvalue required as left operand of assignment", loc);
            return args[0];
        }
        if(!decay(args[0])->is_prim())
            break;
        return args[0];
    }

    case op_type::Add:{
        if(decay(args[0])->is_ptr() || decay(args[0])->is_array()){
            if(is_convertable(args[1], get_type("int64"))){
                return as_ptr_type(args[0]);         
            }   
        }else if(decay(args[1])->is_ptr() || decay(args[1])->is_array()){
            if(is_convertable(args[0], get_type("int64"))){
                return as_ptr_type(args[1]);         
            }    
        }
        break;
    }

    case op_type::Sub:{
        auto l = decay(args[0]), r = decay(args[1]);
        if(l->is_ptr() || l->is_array()){
            if(is_convertable(r, get_type("int64"))){
                return l;         
            }
            if(is_convertable(r, l) || is_convertable(l, r)){
                return get_type("int64");
            }
        }else if(r->is_ptr() || r->is_array()){
            if(is_convertable(l, get_type("int64"))){
                return r;         
            }    
        }

        break;
    }

    case op_type::Ref:{
        if(!args[0]->is_ref()){
            append_ref_error("lvalue required as operand of reference", loc);
            return get_type("#err");
        }
        auto res = std::make_shared<PointerType>();
        res->subtype = decay(args[0]);
        return res;
    }

    case op_type::DeRef:{
        auto op = decay(args[0]);
        if(!op->is_ptr()){
            append_ref_error("pointer type required as operand of dereference", loc);
            return get_type("#err");
        }
        auto sub = std::dynamic_pointer_cast<PointerType>(op)->subtype;
        if(sub->is_void()){
            append_ref_error("cannot dereference pointer of void type.", loc);
            return get_type("#err");
        }        
        return ref_type(sub);
    }

    case op_type::Eq: case op_type::Neq: 
    case op_type::Le: case op_type::Lt: case op_type::Ge: case op_type::Gt:{
        auto op_l = decay(args[0]), op_r = decay(args[1]);
        if((op_l->is_array() || op_l->is_ptr())&&(op_r->is_array() || op_r->is_ptr())) 
            return get_type("bool");
        break;
    }
    
    default:
        break;
    }
    append_nomatch_op_error("no match operator: " + get_op_name(op) + TupleType(args).to_string(), loc);
    return get_type("#err");
}

var_type_ptr op_type_eval(op_type op, std::vector<var_type_ptr> args, Locator loc){

    for(auto& tp: args)
        if(tp->is_error())
            return get_type("#err");

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
        return sp_op_eval(op, args, loc);
    }else if(cnt > 1){
        append_nomatch_op_error("Ambiguous call for operator: "+ get_op_name(op) + ".", loc);
        return get_type("#err");
    }else{
        return impl_list[id].ret;
    }
}