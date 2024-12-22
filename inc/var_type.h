#pragma once

#include"parse.h"
#include<vector>
#include<string>
#include<memory>

struct VarType{

    static size_t ptr_size;

    enum var_kind{
        Prim, Void, Pointer, Array, Struct, Func, FuncList, Tuple, Generic, GenericParam, Ref, Error, Auto, Lambda
    };

    var_kind kind;

    VarType(var_kind kind):kind(kind){}

    virtual std::string to_string() const = 0;

    virtual bool is_same(VarType* type) const = 0;

    virtual size_t size() const{
        return 0;
    }

    bool is_type(var_kind _kind) const {return _kind == kind;}
    bool is_func() const {return is_type(Func);}
    bool is_error()const {return is_type(Error);}
    bool is_void() const {return is_type(Void);}
    bool is_array()const {return is_type(Array);}
    bool is_ptr()  const {return is_type(Pointer);}
    bool is_prim() const {return is_type(Prim);}
    bool is_auto() const{return is_type(Auto);}
    bool is_ref() const {return is_type(Ref);}
    bool is_struct() const{return is_type(Struct);}
    bool is_generic() const{return is_type(Generic);}
    bool is_generic_param() const{
        return is_type(GenericParam);
    }

    bool is_func_ptr() const;

    virtual bool is_base() const{
        return !(is_type(Struct) || is_type(Array));
    }
    virtual bool is_signed() const{
        return true;
    }

    bool contain_loop() const{
        std::set<size_t> reg;
        return loop_check(reg);
    }

    virtual bool loop_check(std::set<size_t>&) const{
        return false;
    }
};

using var_type_ptr = std::shared_ptr<VarType>;

bool is_convertable(var_type_ptr from, var_type_ptr to);
bool is_force_convertable(var_type_ptr from, var_type_ptr to);
void require_convertable(var_type_ptr from, var_type_ptr to, Locator loc);
var_type_ptr greater_type(var_type_ptr a, var_type_ptr b);
var_type_ptr decay(var_type_ptr ptr);
var_type_ptr deref(var_type_ptr ptr);

struct VoidType: VarType{
    VoidType(): VarType(Void){};

    std::string to_string() const override{
        return "void";
    }

    bool is_same(VarType* type) const override{
        return type->kind == Void;
    } 
};

struct AutoType: VarType{
    AutoType(): VarType(Auto){};

    std::string to_string() const override{
        return "#auto";
    }

    bool is_same(VarType* type) const override{
        return false;
    }
};

struct ErrorType: VarType{
    ErrorType(): VarType(Error){};

    std::string to_string() const override{
        return "error";
    }

    bool is_same(VarType* type) const override{
        return false;
    }
};

struct PrimType:VarType{
    enum prim_kind{
        Bool, Char, Int, Float
    }pr_kind;

    int siz;
    bool unsig;

    PrimType(prim_kind kind, int siz, bool unsig = false)
        :VarType(Prim), pr_kind(kind), siz(siz), unsig(unsig){};

    std::string to_string() const override{
        std::string type_name;
        if(unsig)
            type_name = "u";
        switch (pr_kind){
            case Bool: return "bool";
            case Int: type_name += "int";break;
            case Float: type_name = "float";break;
            case Char: return "char";
        }
        return type_name + std::to_string(siz);
    }

    bool is_same(VarType* type) const{
        auto pm = dynamic_cast<PrimType*>(type);
        if(!pm)
            return false;
        return pm->pr_kind == pr_kind && pm->size() == size() && pm->unsig == unsig;
    }

    bool is_signed() const override{
        return pr_kind != Bool && !unsig;
    }

    size_t size() const override{
        return siz/8;
    }
};

struct RefType: VarType{
    std::shared_ptr<VarType> subtype;
    bool is_cnst = false;

    RefType(): VarType(Ref){}

    RefType(std::shared_ptr<VarType> subtype, bool is_cnst)
        : VarType(Ref), subtype(decay(subtype)), is_cnst(is_cnst){}

    std::string to_string() const{
        return "&"+subtype->to_string();
    }

    bool is_same(VarType* type) const{
        auto ref = dynamic_cast<RefType*>(type);
        return subtype->is_same(ref->subtype.get());
    }

    size_t size() const override{
        return subtype->size();
    }

    bool is_base() const override{
        return subtype->is_base();
    }

    bool is_signed() const override{
        return subtype->is_signed();
    }

    bool loop_check(std::set<size_t>& reg) const override{
        return subtype->loop_check(reg);
    }
};

struct PointerType:VarType{
    std::shared_ptr<VarType> subtype;

    PointerType(): VarType(Pointer){}

    PointerType(std::shared_ptr<VarType> subtype): 
        VarType(Pointer), subtype(decay(subtype)){}

    std::string to_string() const{
        if(subtype->kind == Func){
            return subtype->to_string();
        }else if(subtype->is_func_ptr()){
            return "(" + subtype->to_string() + ")*";
        }
        return subtype->to_string() + "*";
    }

    bool is_same(VarType* type) const{
        auto ptr = dynamic_cast<PointerType*>(type);
        if(!ptr)
            return false;
        return subtype->is_same(ptr->subtype.get());
    }

    size_t size() const{
        return ptr_size;
    }

    bool is_signed() const override{
        return false;
    }
};

struct ArrayType:VarType{
    std::shared_ptr<VarType> subtype;
    size_t len;
    
    ArrayType(std::shared_ptr<VarType> type = nullptr, size_t len = 0): 
        VarType(Array), subtype(type), len(len){}

    std::string to_string() const{
        if(subtype->kind == Func)
            return "(" + subtype->to_string() + ")[" + std::to_string(len) + "]";
        return subtype->to_string() + "[" + std::to_string(len) + "]";
    }

    bool is_same(VarType* type) const{
        auto arr = dynamic_cast<ArrayType*>(type);
        if(!arr || len != arr->len)
            return false;
        return subtype->is_same(arr->subtype.get());
    }

    size_t size() const override{
        return len * subtype->size();
    }

    bool loop_check(std::set<size_t>& reg) const override{
        return subtype->loop_check(reg);
    }
};

struct TupleType: VarType{
    std::vector<std::shared_ptr<VarType>> members;

    TupleType(): VarType(Tuple){}

    TupleType(const std::vector<std::shared_ptr<VarType>>& mem): VarType(Tuple){
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

    bool is_same(VarType* type) const override{
        auto tp = dynamic_cast<TupleType*>(type);
        if(!tp || tp->members.size() != members.size())
            return false;
        for(size_t i = 0; i < members.size(); ++i)
            if(!members[i]->is_same(tp->members[i].get()))
                return false;
        return true;
    } 

    size_t size() const override{
        size_t res = 0;
        for(auto& tp: members)
            res += tp->size();
        return res;
    }
};

struct FuncType: VarType{
    std::vector<std::shared_ptr<VarType>> param_list;
    std::shared_ptr<VarType> ret_type;

    bool is_vary;

    FuncType(bool is_vary = false):VarType(Func), ret_type(nullptr), is_vary(is_vary){}

    std::string to_string() const override{
        std::string res = "(";
        bool flag = false;
        for(auto ch: param_list){
            if(flag)
                res += ", ";
            else flag = true;
            res += ch->to_string();
        }
        if(is_vary)
            res += flag? ", ..." : "...";
        res += ") -> ";
        res += ret_type->to_string();
        return res;
    }

    bool is_same(VarType* type) const{
        auto fn = dynamic_cast<FuncType*>(type);
        if(!fn || fn->param_list.size() != param_list.size() || !ret_type->is_same(fn->ret_type.get()))
            return false;
        for(size_t i = 0; i < param_list.size(); ++i)
            if(!param_list[i]->is_same(fn->param_list[i].get()))
                return false;
        return true;
    }

    bool is_callable(var_type_ptr args_list) const{
        if(!args_list->is_type(Tuple))
            return false;
        auto& args = std::dynamic_pointer_cast<TupleType>(args_list)->members;
        bool flag = true;
        
        if(!is_vary){
            if(args.size() > param_list.size())
                return false;
        }
        if(args.size() < param_list.size())
            return false;

        for(size_t i = 0; i < param_list.size(); ++i)
            if(!is_convertable(args[i], param_list[i]))
                flag = false;
        return flag;
    }

    size_t size() const override{
        return ptr_size;
    }
};

// struct FuncListType: VarType{
//     std::vector<std::shared_ptr<FuncType>> func_list;

//     FuncListType():VarType(FuncList){}

//     bool is_same(VarType* type) const{
//         return false;
//     }

//     std::string to_string() const{
//         return "func list";
//     }

//     bool append(std::shared_ptr<FuncType> func){

//     }
// };

struct StructType: VarType{
    using member_list = std::vector<std::pair<std::string, std::shared_ptr<VarType>>>;
    member_list member;
    std::string name;
    size_t id;

    StructType(): VarType(Struct){}

    bool match(const member_list& init){
        size_t cnt = 0;
        if(init.size() != member.size())
            return false;
        for(auto& [id, tp]: init){
            bool flag = 0;
            for(auto& [m_id, m_tp]: member)
                if(id == m_id){
                    flag = 1;
                    if(is_convertable(tp, m_tp))
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

    std::string to_string() const override{
        return name + "#" + std::to_string(id);
    }

    bool is_same(VarType* type) const{
        auto st = dynamic_cast<StructType*>(type);
        return st && id == st->id;
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
};

template<class SrcTy>
struct GenericStructType: VarType{
    SrcTy* src;
    size_t id;
    std::string name;
    std::vector<std::string> param_list;

    GenericStructType(SrcTy* src, size_t id, std::string& name, std::vector<std::string>& param_list):
        src(src), id(id), name(name), param_list(param_list), VarType(Generic){}

    std::string to_string() const override{
        std::string res = name;
        for(size_t i = 0; i < param_list.size(); ++i){
            res += (i == 0) ? "<": ", ";
            res += param_list[i];
        }
        res.push_back('>');
        res += "#" + std::to_string(id);
        return res;
    }

    bool is_same(VarType* type) const override{
        auto gen = dynamic_cast<StructType*>(type);
        return gen && id == gen->id;
    }
};

struct GenericParamType: VarType{
    std::string name;

    GenericParamType(std::string name):
        VarType(GenericParam), name(name){};

    std::string to_string() const override{
        return name;
    }

    bool is_same(VarType* type) const override{
        return false;
    }
};

struct LambdaType: FuncType {
    std::shared_ptr<VarType> obj;
    // std::shared_ptr<FuncType> func;
    LambdaType(std::shared_ptr<VarType> obj, std::shared_ptr<FuncType> func):  obj(obj) {
        param_list = func->param_list;
        ret_type = func->ret_type;
    }
    std::string to_string() const override {
        return obj->to_string() + "$" + FuncType::to_string();
    }
    bool is_same(VarType* type) const override {
        auto st = dynamic_cast<LambdaType*>(type);
        return st && obj->is_same(st->obj.get()) && FuncType::is_same(dynamic_cast<FuncType*>(type));
    } 
};

struct VarInfo{
    std::string name;
    std::shared_ptr<VarType> type;
    bool is_top = false;
    int var_id;

    std::string id_name() const{
        return is_top && type->is_type(VarType::Func) ? name: "var_" + name + "_" + std::to_string(var_id);
    }

    std::string ptr_name() const{
        return "ptr_" + name + "_" + std::to_string(var_id);
    }

    size_t size() const{
        return decay(type)->size();
    }

    bool is_base(){
        return decay(type)->is_base();
    }
};

using var_info_ptr = std::shared_ptr<VarInfo>;

void init_type_pool();
var_type_ptr get_type(std::string name);
std::vector<std::shared_ptr<PrimType>>& get_prim_list();
bool set_type(std::string name, var_type_ptr type);

var_type_ptr as_ptr_type(var_type_ptr tp);
var_type_ptr ref_type(var_type_ptr ptr, bool is_cnst = false);

template<class T>
std::shared_ptr<T> dyn_ptr_cast(var_type_ptr ptr){
    return std::dynamic_pointer_cast<T>(ptr);
}