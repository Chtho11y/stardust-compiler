#include "var_type.h"
#include "context.h"
#include "error.h"
#include <map>
#include <memory>

std::map<std::string, var_type_ptr> type_pool;
std::vector<std::shared_ptr<PrimType>> prim_type_list;

void init_type_pool(){
    for(int i = 8; i <= 64; i <<= 1){
        type_pool["int" + std::to_string(i)] = std::make_shared<PrimType>(PrimType::Int, i);
        type_pool["uint" + std::to_string(i)] = std::make_shared<PrimType>(PrimType::Int, i, true);
    }

    type_pool["float32"] = std::make_shared<PrimType>(PrimType::Float, 32);
    type_pool["float64"] = std::make_shared<PrimType>(PrimType::Float, 64);
    type_pool["char"] = std::make_shared<PrimType>(PrimType::Char, 8);
    type_pool["bool"] = std::make_shared<PrimType>(PrimType::Bool, 8);

    for(auto [s, p]: type_pool)
        prim_type_list.push_back(std::dynamic_pointer_cast<PrimType>(p));

    type_pool["int"] = type_pool["int32"];

    type_pool["void"] = std::make_shared<VoidType>();
    type_pool["#err"] = std::make_shared<ErrorType>();
}

std::vector<std::shared_ptr<PrimType>>& get_prim_list(){
    return prim_type_list;
}

var_type_ptr get_type(std::string name){
    if(type_pool.count(name))
        return type_pool[name];
    return std::make_shared<ErrorType>();
}

bool set_type(std::string name, var_type_ptr type){
    if(type_pool.count(name))
        return false;
    type = decay(type);
    type_pool[name] = type;
    return true;
}

bool is_convertable(var_type_ptr from, var_type_ptr to){
    from = decay(from), to = decay(to);
    // std::cout << "cmp: " << from->to_string() << ", " << to->to_string() << std::endl;
    if(to->is_error() || from->is_error() || from->is_same(to.get())){
        return true;
    }else if(to->is_prim() && from->is_prim()){

        auto fp = std::dynamic_pointer_cast<PrimType>(from);
        auto tp = std::dynamic_pointer_cast<PrimType>(to);

        if(fp->pr_kind < tp->pr_kind)
            return true;
        else if(fp->pr_kind == tp->pr_kind){
            return (fp->size*2 + fp->unsig) <= (tp->size*2 + tp->unsig) ;
        }
    
    }else if(to->is_ptr()){
        auto to_ptr = std::dynamic_pointer_cast<PointerType>(to);
        if(from->is_array()){
            auto from_arr = std::dynamic_pointer_cast<ArrayType>(from);
            if(from_arr->subtype->is_same(to_ptr->subtype.get()))
                return true;
        }else if(from->is_ptr()){
            auto from_ptr = std::dynamic_pointer_cast<PointerType>(from);
            if(from_ptr->subtype->is_void() || to_ptr->subtype->is_void())
                return true;
        }
    }
    return false;
}

bool is_force_convertable(var_type_ptr from, var_type_ptr to){
    from = decay(from), to = decay(to);
    if(to->is_error() || from->is_error() || to->is_same(from.get())){
        return true;
    }else if(to->is_prim()){
        return from->is_prim() || from->is_ptr();
    }else if(to->is_ptr()){
        if(from->is_prim()){
            auto fp = std::dynamic_pointer_cast<PrimType>(from);
            return fp->pr_kind == PrimType::Int;
        }
        return from->is_ptr() || from->is_array();
    }
    return false;
}

void require_convertable(var_type_ptr from, var_type_ptr to, Locator loc){
    from = decay(from), to = decay(to);
    if(!is_convertable(from, to)){
        append_convert_error(from, to, loc);
        // append_error("Cannot convert type \'" + from->to_string() + "\' to \'" + to->to_string() + "\'.", loc);
    }
}

var_type_ptr greater_type(var_type_ptr a, var_type_ptr b){
    a = decay(a), b = decay(b);
    if(a->is_error() || b->is_error())
        return get_type("#err");
    if(is_convertable(a, b))
        return b;
    else if(is_convertable(b, a))
        return a;
    else return get_type("void");
}

var_type_ptr ref_type(var_type_ptr ptr){
    if(ptr->is_void() || ptr->is_error())
        return ptr;
    return std::make_shared<RefType>(ptr);
}

var_type_ptr decay(var_type_ptr ptr){
    while(ptr->is_ref())
        ptr = std::dynamic_pointer_cast<RefType>(ptr)->subtype;
    return ptr;
}