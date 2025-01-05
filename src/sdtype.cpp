#include "sdtype.h"
#include "error.h"
#include <map>

namespace sd{

TypeContext sd_type_ctx;

TypeContext& get_type_context(){
    return sd_type_ctx;
}

std::vector<PrimType*>& get_prim_list(){
    return sd_type_ctx.prim_type_list;
}

void type_ctx_init(){
    sd_type_ctx = TypeContext();
    sd_type_ctx.init();
}

bool PrimType::is_convertable(Type* tp){
    if(tp->is_error()) return true;
    if(tp->is_trait() && with_trait(dyn_ptr_cast<TraitType>(tp)))
        return true;
    auto pm = dynamic_cast<PrimType*>(tp);
    if(!pm) return false;

    if(kind < pm->kind)
        return true;
    else if(kind == pm->kind){
        return (siz*2 + unsig) <= (pm->siz*2 + pm->unsig);
    }
    return false;
}

bool PrimType::is_force_convertable(Type* tp){
    if(tp->is_error()) return true;
    if(tp->is_prim())
        return true;
    if(kind == prim_kind::Int && tp->is_ptr())
        return true;
    return false;
}

Type* Type::deref(){
    if(!is_ptr())
        return this;
    return dyn_ptr_cast<PointerType>(this)->subtype;
}

int Type::trait_index(size_t id){
    auto base = 0;
    for(auto trait: traits){
        if(trait->id == id)
            return base;
        base += trait->trait_size();
    }
    return -1;
}

bool Type::with_trait(TraitType* trait) {
    return trait_index(trait->id) != -1;
}

bool Type::is_func_ptr(){
    auto ptr = dyn_ptr_cast<PointerType>(this);
    return ptr && ptr->subtype->is_func();
}

FuncType::FuncType(std::vector<Type*>& param_list, Type* ret, bool is_vary):
    Type(type_kind::Func), param_list(param_list), ret_type(ret), is_vary(is_vary){}

MemFuncType::MemFuncType(Type* parent, std::vector<Type*>& param_list, Type* ret):
    FuncType(type_kind::MemFunc, param_list, ret), parent_type(parent){
        auto fn = FuncType::get(param_list, ret);
        // traits.push_back(TraitType::get_callable(fn));
    }

template<class Tp, class... Args>
Tp* get_or_create(Args... args){
    Tp* tp = new Tp(args...);
    if(auto ptr = sd_type_ctx.get(tp->unique_type_name())){
        delete tp;
        return dynamic_cast<Tp*>(ptr);
    }
    sd_type_ctx.create(tp);

    return tp;
}

VoidType* VoidType::get(){
    return get_or_create<VoidType>();
}

AutoType* AutoType::get(){
    return get_or_create<AutoType>();
}    

ErrorType* ErrorType::get(){
    return get_or_create<ErrorType>();
}

PrimType* PrimType::get(prim_kind kind, int siz, bool unsig){
    return get_or_create<PrimType>(kind, siz, unsig);
}

PrimType* PrimType::get_bool(){
    return get(prim_kind::Bool, 8, true);
}

PrimType* PrimType::get_char(){
    return get(prim_kind::Char, 8, true);
}

PrimType* PrimType::get_uint(int siz){
    return get(prim_kind::Int, siz, true);
}

PrimType* PrimType::get_int(int siz){
    return get(prim_kind::Int, siz, false);
}

PrimType* PrimType::get_float(int siz){
    return get(prim_kind::Float, siz, false);
}

RefType* RefType::get(Type* subtype, bool is_cnst){
    return get_or_create<RefType>(subtype->decay(), is_cnst);
}

PointerType* PointerType::get(Type* subtype){
    return get_or_create<PointerType>(subtype->decay());
}

ArrayType* ArrayType::get(Type* subtype, size_t len){
    return get_or_create<ArrayType>(subtype->decay(), len);
}

FuncType* FuncType::get(std::vector<Type*> param_list, Type* ret, bool is_vary){
    return get_or_create<FuncType>(param_list, ret, is_vary);
}

MemFuncType* MemFuncType::get(Type* parent, std::vector<Type*> param_list, Type* ret){
    return get_or_create<MemFuncType>(parent, param_list, ret);
}

MemFuncType* MemFuncType::get(Type* parent, FuncType* func){
    return get(parent, func->param_list, func->ret_type);
}

TupleType* TupleType::get(const std::vector<Type*>& mem){
    return get_or_create<TupleType>(mem);
}

StructType* StructType::create(std::string name){
    auto st = new StructType();
    sd_type_ctx.create(st);
    st->name = name;
    return st;
}

TraitType* TraitType::create(std::string name, std::vector<std::pair<std::string, FuncType*>> func_list){
    auto trait = new TraitType(name, func_list);
    sd_type_ctx.create(trait);
    return trait;
}

TraitType* TraitType::get_callable(FuncType* fn){
    std::vector<std::pair<std::string, FuncType*>> func = {{"call", fn}};
    return get_or_create<TraitType>("@Callable", func);
}

StructType* StructType::create(){
    auto st = new StructType();
    sd_type_ctx.create(st);
    return st;
}

GenericParamType* GenericParamType::get(std::string name){
    return get_or_create<GenericParamType>(name);
}

void require_convertable(var_type_ptr from, var_type_ptr to, Locator loc){
    if(!from->is_convertable(to)){
        append_convert_error(from, to, loc);
    }  
}

var_type_ptr greater_type(var_type_ptr a, var_type_ptr b){
    a = a->decay();
    b = b->decay();
    if(a->is_convertable(b)) return b;
    if(b->is_convertable(a)) return a;
    return VoidType::get();
}

var_type_ptr upper_type(var_type_ptr tp){
    tp = tp->decay();
    if(tp->is_array()){
        auto ptr = PointerType::get(dyn_ptr_cast<ArrayType>(tp)->subtype);
        return tp;
    }
    if(tp->is_prim()){
        auto prim_tp = dyn_ptr_cast<PrimType>(tp);
        auto lim = (prim_tp->kind == PrimType::Float ? 64: 32);
        return PrimType::get(std::max(prim_tp->kind, PrimType::Int), std::max(lim, prim_tp->siz), prim_tp->unsig);
    }
    return tp;
}

}

