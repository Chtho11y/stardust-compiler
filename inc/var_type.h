#pragma once

#include"parse.h"
#include<vector>
#include<string>
#include<memory>

struct VarType{

    enum var_kind{
        Prim, Void, Pointer, Array, Struct, Func, Tuple, Ref, Error, Auto
    };

    var_kind kind;

    VarType(var_kind kind):kind(kind){}

    virtual std::string to_string() const = 0;

    virtual bool is_same(VarType* type) const = 0;

    bool is_type(var_kind _kind) const {return _kind == kind;}
    bool is_error()const {return is_type(Error);}
    bool is_void() const {return is_type(Void);}
    bool is_array()const {return is_type(Array);}
    bool is_ptr()  const {return is_type(Pointer);}
    bool is_prim() const {return is_type(Prim);}
    bool is_auto() const{return is_type(Auto);}
    bool is_ref() const {return is_type(Ref);}
};

using var_type_ptr = std::shared_ptr<VarType>;

bool is_convertable(var_type_ptr from, var_type_ptr to);
bool is_force_convertable(var_type_ptr from, var_type_ptr to);
void require_convertable(var_type_ptr from, var_type_ptr to, Locator loc = Locator());
var_type_ptr greater_type(var_type_ptr a, var_type_ptr b);
var_type_ptr ref_type(var_type_ptr ptr);
var_type_ptr decay(var_type_ptr ptr);

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

    int size;
    bool unsig;

    PrimType(prim_kind kind, int size, bool unsig = false)
        :VarType(Prim), pr_kind(kind), size(size), unsig(unsig){};

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
        return type_name + std::to_string(size);
    }

    bool is_same(VarType* type) const{
        auto pm = dynamic_cast<PrimType*>(type);
        if(!pm)
            return false;
        return pm->pr_kind == pr_kind && pm->size == size && pm->unsig == unsig;
    }
};

struct RefType: VarType{
    std::shared_ptr<VarType> subtype;

    RefType(): VarType(Ref){}

    RefType(std::shared_ptr<VarType> subtype)
        : VarType(Ref), subtype(decay(subtype)){}

    std::string to_string() const{
        return "&"+subtype->to_string();
    }

    bool is_same(VarType* type) const{
        auto ref = dynamic_cast<RefType*>(type);
        return subtype->is_same(ref->subtype.get());
    }
};

struct PointerType:VarType{
    std::shared_ptr<VarType> subtype;

    PointerType(): VarType(Pointer){}

    std::string to_string() const{
        if(subtype->kind == Func)
            return "(" + subtype->to_string() + ")*";
        return subtype->to_string() + "*";
    }

    bool is_same(VarType* type) const{
        auto ptr = dynamic_cast<PointerType*>(type);
        if(!ptr)
            return false;
        return subtype->is_same(ptr->subtype.get());
    } 
};

struct ArrayType:VarType{
    std::shared_ptr<VarType> subtype;
    size_t size;
    
    ArrayType(std::shared_ptr<VarType> type = nullptr, size_t size = 0): 
        VarType(Array), subtype(type), size(size){}

    std::string to_string() const{
        if(subtype->kind == Func)
            return "(" + subtype->to_string() + ")[" + std::to_string(size) + "]";
        return subtype->to_string() + "[" + std::to_string(size) + "]";
    }

    bool is_same(VarType* type) const{
        auto arr = dynamic_cast<ArrayType*>(type);
        if(!arr || size != arr->size)
            return false;
        return subtype->is_same(arr->subtype.get());
    } 
};

struct FuncType: VarType{
    std::vector<std::shared_ptr<VarType>> param_list;
    std::shared_ptr<VarType> ret_type;

    FuncType():VarType(Func), ret_type(nullptr){}

    std::string to_string() const override{
        std::string res = "(";
        bool flag = false;
        for(auto ch: param_list){
            if(flag)
                res += ", ";
            else flag = true;
            res += ch->to_string();
        }
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
};

struct TupleType: VarType{
    std::vector<std::shared_ptr<VarType>> members;

    TupleType(): VarType(Tuple){}

    std::string to_string() const override{
        std::string res = "(";
        bool flag = false;
        for(auto ch: members){
            if(flag)
                res += ", ";
            else flag = true;
            res += ch->to_string();
        }
        res += ")";
        return res;
    }

    bool is_same(VarType* type) const{
        auto tp = dynamic_cast<TupleType*>(type);
        if(!tp || tp->members.size() != members.size())
            return false;
        for(size_t i = 0; i < members.size(); ++i)
            if(!members[i]->is_same(tp->members[i].get()))
                return false;
        return true;
    } 
};

struct StructType: VarType{
    std::vector<std::pair<std::string, std::shared_ptr<VarType>>> member;
    std::string name;
    size_t id;

    StructType(): VarType(Struct){}

    std::string to_string() const override{
        return name;
    }

    bool is_same(VarType* type) const{
        auto st = dynamic_cast<StructType*>(type);
        return st && id == st->id;
    }
};

struct VarInfo{
    std::string name;
    std::shared_ptr<VarType> type;
};

void init_type_pool();
var_type_ptr get_type(std::string name);
std::vector<std::shared_ptr<PrimType>>& get_prim_list();
bool set_type(std::string name, var_type_ptr type);

