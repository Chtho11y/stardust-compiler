#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>
#include <cassert>
#include <memory>

#include "context.h"

namespace sd{

void type_ctx_init();

enum class type_kind{
    Prim, Void, Pointer, Array, Struct, Func, FuncList, Tuple, Generic, GenericParam, Trait, MemFunc, Ref, Error, Auto
};

struct TraitType;
struct FuncType;

struct Type{
    type_kind type;
    // std::vector<FuncType*> mem_func_table; 
    std::vector<TraitType*> traits;

    size_t id;

    bool is_type(type_kind kind) const {return type == kind;}

    bool is_prim() const {return is_type(type_kind::Prim);}
    bool is_void() const {return is_type(type_kind::Void);}
    bool is_ptr()  const {return is_type(type_kind::Pointer);}
    bool is_array()const {return is_type(type_kind::Array);}
    bool is_struct() const{return is_type(type_kind::Struct);}
    bool is_func() const {return is_type(type_kind::Func);}
    bool is_mem_func() const {return is_type(type_kind::MemFunc);}
    bool is_tuple() const{return is_type(type_kind::Tuple);}
    bool is_generic() const{return is_type(type_kind::Generic);}
    bool is_generic_param() const{return is_type(type_kind::GenericParam);}
    bool is_error()const {return is_type(type_kind::Error);}
    bool is_auto() const{return is_type(type_kind::Auto);}
    bool is_ref() const {return is_type(type_kind::Ref);}
    bool is_trait() const {return is_type(type_kind::Trait);}

    bool is_func_ptr();

    //human-readable type name
    virtual std::string to_string() const = 0;
    virtual std::string to_barket_string() const{return to_string();}

    //unique identifier in the type table
    virtual std::string unique_type_name() const{
        return to_string();
    }

    //name in symbol table
    virtual std::string sym_name() const{
        return to_string();
    }

    //unique label supported by the LLVM IR
    virtual std::string to_label() const {
        return to_string();
    }

    std::string get_prefix() const{
        return  "type." + std::to_string(id) + "$"; 
    }

    virtual bool is_signed() const{return true;}
    virtual size_t size() const{return 0;}

    bool contain_loop() const{
        std::set<size_t> reg;
        return loop_check(reg);
    }

    virtual bool loop_check(std::set<size_t>&) const{
        return false;
    }

    virtual Type* decay() {
        return this;
    }

    template<class T>
    T* cast(){
        return dynamic_cast<T*>(this);
    }

    Type* deref();

    virtual bool is_convertable(Type* tp) = 0;

    virtual bool is_force_convertable(Type* tp) = 0; 

    int trait_index(size_t id);

    bool with_trait(TraitType* trait);

    virtual ~Type(){}

protected:
    Type(){}
    Type(type_kind type): type(type), id(id){}
};


using sd_type_ptr = sd::Type*;
using var_type_ptr = sd::Type*;

template<class T>
T* dyn_ptr_cast(var_type_ptr ptr);

struct VoidType: Type{
    static VoidType* get();

    std::string to_string() const override{
        return "void";
    }

    VoidType(): Type(type_kind::Void){};

    virtual bool is_convertable(Type* tp){
        return tp->is_void();
    }

    virtual bool is_force_convertable(Type* tp){
        return tp->is_void();
    }
};

struct AutoType: Type{

    static AutoType* get();

    std::string to_string() const override{
        return "#auto";
    }

    AutoType(): Type(type_kind::Auto){};

    virtual bool is_convertable(Type* tp){
        assert(false && "cannot convert a '#auto' type");
        return false;
    }

    virtual bool is_force_convertable(Type* tp){
        assert(false && "cannot convert a '#auto' type");
        return false;
    }
};

struct ErrorType: Type{

    static ErrorType* get();

    std::string to_string() const override{
        return "#error";
    }

    ErrorType(): Type(type_kind::Error){};

    virtual bool is_convertable(Type* tp){
        return true;
    }

    virtual bool is_force_convertable(Type* tp){
        return true;
    }
};

struct PrimType: Type{

    enum prim_kind{
        Bool, Char, Int, Float
    }kind;

    static PrimType* get(prim_kind kind, int siz, bool unsig = false);
    static PrimType* get_bool();
    static PrimType* get_char();
    static PrimType* get_uint(int siz);
    static PrimType* get_int(int siz);    
    static PrimType* get_float(int siz);

    int siz;
    bool unsig;

    std::string to_string() const override{
        std::string type_name;
        if(unsig)
            type_name = "u";
        switch (kind){
            case Bool: return "bool";
            case Int: type_name += "int";break;
            case Float: type_name = "float";break;
            case Char: return "char";
        }
        return type_name + std::to_string(siz);
    }

    bool is_signed() const override{
        return kind != Bool && !unsig;
    }

    size_t size() const override{
        return siz/8;
    }  

    PrimType(prim_kind kind, int siz, bool unsig = false)
        :Type(type_kind::Prim), kind(kind), siz(siz), unsig(unsig){};
    
    bool is_convertable(Type* tp) override;

    bool is_force_convertable(Type* tp) override;
};

struct RefType: Type{
    Type* subtype;
    bool is_cnst = false;

    static RefType* get(Type* subtype, bool is_cnst = false);

    RefType(Type* subtype, bool is_cnst)
        : Type(type_kind::Ref), subtype(subtype), is_cnst(is_cnst){}

    std::string to_string() const override{
        return "&" + subtype->to_barket_string();
    }

    std::string unique_type_name() const override{
        return "&" + std::to_string(subtype->id);
    }

    size_t size() const override{
        return subtype->size();
    }

    bool is_signed() const override{
        return subtype->is_signed();
    }

    bool loop_check(std::set<size_t>& reg) const override{
        return subtype->loop_check(reg);
    }

    virtual Type* decay() override{
        return subtype;
    }

    bool is_convertable(Type* tp)  override{
        if(tp == this)
            return true;
        return subtype->is_convertable(tp);
    }

    bool is_force_convertable(Type* tp)  override{
        if(tp == this)
            return true;
        return subtype->is_force_convertable(tp);
    }
};

struct PointerType: Type{
    Type* subtype;

    static PointerType* get(Type* subtype);

    PointerType(Type* subtype): Type(type_kind::Pointer), subtype(subtype){}

    std::string to_string() const override{
        if(subtype->is_func())
            return subtype->to_string().substr(2);
        else return subtype->to_barket_string() + "*";
    }

    std::string to_barket_string() const override{
        if(subtype->is_func())
            return "("  + to_string() + ")";
        return to_string();
    }

    std::string unique_type_name() const override{
        return std::to_string(subtype->id) + "*";
    }

    size_t size() const{
        return 8;
    }

    bool is_signed() const override{
        return false;
    }

    bool is_convertable(Type* tp)  override{
        if(tp->is_error())
            return true;
        auto ptr = dynamic_cast<PointerType*>(tp);
        return ptr && (ptr == this || ptr->subtype->is_void() || subtype->is_void());
    }

    bool is_force_convertable(Type* tp)  override{
        if(tp->is_ptr() || tp->is_error())
            return true;
        if(auto pm = dynamic_cast<PrimType*>(tp))
            return pm->kind == PrimType::Int;
        return false;
    }
};

struct ArrayType: Type{
    Type* subtype;
    size_t len;

    static ArrayType* get(Type* subtype, size_t len);
    
    ArrayType(Type* type, size_t len): 
        Type(type_kind::Array), subtype(type), len(len){}

    std::string to_string() const{
        return subtype->to_barket_string() + "[" + std::to_string(len) + "]";
    }

    std::string unique_type_name() const override{
        return std::to_string(subtype->id) + "[" + std::to_string(len) + "]";
    }

    size_t size() const override{
        return len * subtype->size();
    }

    bool loop_check(std::set<size_t>& reg) const override{
        return subtype->loop_check(reg);
    }

    bool is_convertable(Type* tp)  override{
        if(tp == this || tp->is_error())
            return true;
        if(tp->is_trait() && with_trait(dyn_ptr_cast<TraitType>(tp)))
            return true;
        if(auto ptr = dynamic_cast<PointerType*>(tp)){
            return ptr->subtype == subtype;
        }

        return false;
    }

    bool is_force_convertable(Type* tp)  override{
        if(is_convertable(tp))
            return true;
        if(auto pm = dynamic_cast<PrimType*>(tp))
            return pm->kind == PrimType::Int;
        return false;
    }
};

struct TupleType: Type{
    std::vector<Type*> members;

    static TupleType* get(const std::vector<Type*>& mem);

    TupleType(std::vector<Type*> mem): Type(type_kind::Tuple){
        members = mem;
    }

    std::string to_string() const override{
        std::string res = "(";
        for(size_t i = 0; i < members.size(); ++i){
            if(i) res += ", ";
            res += members[i]->to_string();
        }
        res += ")";
        return res;
    }

    std::string unique_type_name() const override{
        std::string res = "(";
        for(size_t i = 0; i < members.size(); ++i){
            if(i) res += ", ";
            res += std::to_string(members[i]->id);
        }
        res += ")";
        return res;
    }

    size_t size() const override{
        size_t res = 0;
        for(auto& tp: members)
            res += tp->size();
        return res;
    }

    bool is_convertable(Type* tp)  override{
        return tp == this || tp->is_error() || tp->is_trait() && with_trait(dyn_ptr_cast<TraitType>(tp));
    }

    bool is_force_convertable(Type* tp)  override{
        return is_convertable(tp);
    }

    bool loop_check(std::set<size_t>& reg) const override{
        if(reg.count(id))
            return true;
        reg.insert(id);
        for(auto tp: members)
            if(tp->loop_check(reg))
                return true;
        reg.erase(id);
        return false;
    }
};

struct FuncType: Type{
    std::vector<Type*> param_list;
    Type* ret_type;

    static FuncType* get(std::vector<Type*> param_list, Type* ret, bool is_vary = false);

    bool is_vary;

    FuncType(std::vector<Type*>& param_list, Type* ret, bool is_vary = false);
    
    FuncType(type_kind sub_kind, std::vector<Type*>& param_list, Type* ret):
        Type(sub_kind), param_list(param_list), ret_type(ret), is_vary(false){}

    std::string to_string() const override{
        std::string res = "fn(";
        for(size_t i = 0; i < param_list.size(); ++i){
            if(i) res += ", ";
            res += param_list[i]->to_string();
        }
        if(is_vary)
            res += param_list.size()? ", ..." : "...";
        res += ") -> ";
        res += ret_type->to_string();
        return res;
    }

    std::string to_barket_string() const override{
        return "(" + to_string() + ")";
    }

    std::string unique_type_name() const override{
        std::string res = "(";
        for(size_t i = 0; i < param_list.size(); ++i){
            if(i) res += ", ";
            res += std::to_string(param_list[i]->id);
        }
        if(is_vary)
            res += param_list.size()? ", ..." : "...";
        res += ") -> ";
        res += std::to_string(ret_type->id);
        return res;
    }

    bool is_callable(Type* args_list) const{
        if(!args_list->is_tuple())
            return false;
        auto& args = dynamic_cast<TupleType*>(args_list)->members;
        bool flag = true;
        
        if(!is_vary){
            if(args.size() > param_list.size())
                return false;
        }
        if(args.size() < param_list.size())
            return false;

        for(size_t i = 0; i < param_list.size(); ++i)
            if(!args[i]->is_convertable(args[i]))
                flag = false;
        return flag;
    }

    bool is_convertable(Type* tp) override{
        if(this == tp || tp->is_error())
            return true;
        if(tp->is_trait() && with_trait(dyn_ptr_cast<TraitType>(tp)))
            return true;
        if(auto ptr = dynamic_cast<PointerType*>(tp)){
            return ptr->subtype == this || ptr->subtype->is_void();
        }
        return false;
    }

    bool is_force_convertable(Type* tp) override{
        return is_convertable(tp);
    }

    size_t size() const override{
        return 8;
    }
};

struct MemFuncType: FuncType{

    Type* parent_type;

    static MemFuncType* get(Type* parent, std::vector<Type*> param_list, Type* ret);
    static MemFuncType* get(Type* parent, FuncType* func);

    MemFuncType(Type* parent, std::vector<Type*>& param_list, Type* ret);
    
    std::string to_string() const override{
        std::string res = parent_type->to_barket_string()+ "::(";
        for(size_t i = 0; i < param_list.size(); ++i){
            if(i) res += ", ";
            res += param_list[i]->to_string();
        }
        res += ") -> ";
        res += ret_type->to_string();
        return res;
    }

    std::string unique_type_name() const override{
        std::string res = std::to_string(parent_type->id) + "::(";
        for(size_t i = 0; i < param_list.size(); ++i){
            if(i) res += ", ";
            res += std::to_string(param_list[i]->id);
        }
        res += ") -> ";
        res += std::to_string(ret_type->id);
        return res;
    }
};

struct StructType: Type{
    using member_list = std::vector<std::pair<std::string, Type*>>;
    member_list member;
    std::string name;
    std::vector<Type*> generic_params;

    static StructType* create(std::string name);
    static StructType* create();

    StructType(): Type(type_kind::Struct){}

    bool match(const member_list& init){
        size_t cnt = 0;
        if(init.size() != member.size())
            return false;
        for(auto& [id, tp]: init){
            bool flag = 0;
            for(auto& [m_id, m_tp]: member)
                if(id == m_id){
                    flag = 1;
                    if(tp->is_convertable(m_tp))
                        cnt++;
                    else return false;
                }
            if(!flag)
                return false;
        }
        return cnt == member.size();
    }

    size_t offset_of(std::string& mem) const{
        size_t res = 0;
        for(auto [s, tp]: member){
            if(s == mem)
                return res;
            res += tp->size();
        }
        return res;
    }

    size_t index_of(std::string& nam) const{
        size_t res = 0;
        for(auto [s, tp]: member){
            if(s == nam)
                return res;
            res++;
        }
        return res;       
    }

    Type* type_of(std::string& nam) const{
        for(auto& [s, tp]: member){
            if(s == nam)
                return tp;
        }
        return nullptr;
    }

    std::string to_string() const override{
        if(generic_params.size()){
            auto tp = TupleType(generic_params);
            auto desc = tp.to_string();
            desc.front() = '<';
            desc.back() = '>';
            return name + desc;
        }
        return name;
    }

    std::string unique_type_name() const override{
        return std::to_string(id);
    }

    size_t size() const override{
        size_t res = 0;
        for(auto& [s, tp]: member)
            res += tp->size();
        return res;
    }

    bool loop_check(std::set<size_t>& reg) const override{
        if(reg.count(id))
            return true;
        reg.insert(id);
        for(auto [s, tp]: member)
            if(tp->loop_check(reg))
                return true;
        reg.erase(id);
        return false;
    }

    bool is_convertable(Type* tp) override{
        return tp->is_error() || tp == this || tp->is_trait() && with_trait(dyn_ptr_cast<TraitType>(tp));
    }

    bool is_force_convertable(Type* tp) override{
        return tp->is_error() || tp == this;
    }
};

template<class SrcTy>
struct GenericStructType: Type{

    using self = GenericStructType<SrcTy>;

    SrcTy* src;
    std::string name;
    std::vector<std::string> param_list;

    static self* create(SrcTy* src, std::string& name, std::vector<std::string>& param_list);

    GenericStructType(SrcTy* src, std::string& name, std::vector<std::string>& param_list):
        src(src), name(name), param_list(param_list), Type(type_kind::Generic){}

    std::string to_string() const override{
        std::string res = name;
        for(size_t i = 0; i < param_list.size(); ++i){
            res += (i == 0) ? "<": ", ";
            res += param_list[i];
        }
        res.push_back('>');
        return res;
    }

    std::string unique_type_name() const override{
        return std::to_string(id);
    }

    bool is_convertable(Type* tp) override{
        assert(false && "cannot convert generic type");
        return false;
    }

    bool is_force_convertable(Type* tp) override{
        assert(false && "cannot convert generic type");
        return false;
    }

};

struct GenericParamType: Type{
    std::string name;

    static GenericParamType* get(std::string name);

    GenericParamType(std::string name):
        Type(type_kind::GenericParam), name(name){};

    std::string to_string() const override{
        return name;
    }

    std::string unique_type_name() const override{
        return "<" + name + ">";
    }

    bool is_convertable(Type* tp) override{
        assert(false && "cannot convert generic param type");
        return false;
    }

    bool is_force_convertable(Type* tp) override{
        assert(false && "cannot convert generic param type");
        return false;
    }
};

struct TraitType: Type{
    std::vector<std::pair<std::string, FuncType*>> func_list;
    std::string name;

    static TraitType* create(std::string name, std::vector<std::pair<std::string, FuncType*>> func_list);
    static TraitType* get_callable(FuncType* fn);

    TraitType(std::string name, std::vector<std::pair<std::string, FuncType*>> func): Type(type_kind::Trait), name(name), func_list(func){}

    size_t trait_size() const{
        return func_list.size();
    }

    std::vector<std::pair<std::string, FuncType*>> get_func_list() {
        return func_list;
    }

    std::string to_string() const override{
        return name + "#" + std::to_string(id);
    }

    std::string unique_type_name() const override{
        if(name == "@Callable")
            return name + "<" + std::to_string(func_list[0].second->id) + ">";
        return std::to_string(id);
    }

    bool is_convertable(Type* tp) override{
        return tp == this;
    }

    bool is_force_convertable(Type* tp) override{
        return is_convertable(tp);
    }

    bool loop_check(std::set<size_t>& reg) const override{
        return true;
    }
};


struct VarInfo{
    std::string name;
    sd_type_ptr type;
    bool is_top = false;
    int var_id;

    std::string id_name() const{
        return is_top ? name: name + "." + std::to_string(var_id);
    }

    std::string ptr_name() const{
        return "ptr_" + name + "_" + std::to_string(var_id);
    }

    size_t size() const{
        return type->decay()->size();
    }
};

using var_info_ptr = std::shared_ptr<VarInfo>;

struct TypeContext{
    std::map<std::string, sd_type_ptr> type_ptr_table;
    std::map<size_t, sd_type_ptr> id_ptr_table;
    std::vector<PrimType*> prim_type_list;
    size_t tp_counter = 0;

    void init(){
        for(int i = 8; i <= 64; i <<= 1){
            prim_type_list.push_back(PrimType::get_int(i));
            prim_type_list.push_back(PrimType::get_uint(i));
        }

        prim_type_list.push_back(PrimType::get_float(32));
        prim_type_list.push_back(PrimType::get_float(64));
        prim_type_list.push_back(PrimType::get_bool());
        prim_type_list.push_back(PrimType::get_char());

        auto _ = VoidType::get();

        create_global_alias("int", PrimType::get_int(32));
    }

    ~TypeContext(){
        for(auto [_, tp]: id_ptr_table){
            delete tp;
        }
    }

    sd_type_ptr get(std::string id){
        auto it = type_ptr_table.find(id);
        return it == type_ptr_table.end() ? nullptr: it->second;
    }

    sd_type_ptr get(size_t id){
        auto it = id_ptr_table.find(id);
        return it == id_ptr_table.end() ? nullptr: it->second;
    }

    void create(sd_type_ptr tp){
        tp->id = tp_counter++;
        type_ptr_table[tp->unique_type_name()] = tp;
        id_ptr_table[tp->id] = tp;
        // if(tp->is_func_ptr() || tp->is_func()){
        //     tp->traits.push_back(TraitType::get_callable(tp->deref()->cast<FuncType>()));
        //     ast_context.gen_callable_list.push_back(tp->id);
        // }
    }

    void create_global_alias(std::string name, sd_type_ptr type){
        assert(type_ptr_table.count(name) == 0);
        type_ptr_table[name] = type;
    }
};

TypeContext& get_type_context();
std::vector<PrimType*>& get_prim_list();

template<class SrcTy>
auto GenericStructType<SrcTy>::create(SrcTy* src, std::string& name, 
                        std::vector<std::string>& param_list) -> self* {
    auto tp = new self(src, name, param_list);
    get_type_context().create(tp);
    return tp;
}

void require_convertable(var_type_ptr from, var_type_ptr to, Locator loc);
var_type_ptr greater_type(var_type_ptr a, var_type_ptr b);
var_type_ptr upper_type(var_type_ptr var);

template<class T>
T* dyn_ptr_cast(var_type_ptr ptr){
    return dynamic_cast<T*>(ptr);
}

}

using var_type_ptr = sd::Type*;
using var_info_ptr = std::shared_ptr<sd::VarInfo>;