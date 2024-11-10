#pragma once

#include<vector>
#include<string>
#include<memory>

struct VarType{
    VarType* subtype;

    enum var_kind{
        Prim, Pointer, Array, Struct, Func
    };

    var_kind kind;

    VarType(var_kind kind):kind(kind), subtype(nullptr){}

    bool support_call(){
        return kind == Func;
    }

    bool support_deref(){
        return kind == Pointer;
    }

    bool support_at(){
        return kind == Array;
    }
};

void init_type_pool();
VarType* get_type(std::string name);
bool set_type(std::string name, std::shared_ptr<VarType> type);

struct PrimType:VarType{
    enum prim_kind{
        Int, UInt, Float, Char
    }pr_kind;

    int size;

    PrimType(prim_kind kind, int size):VarType(Prim), pr_kind(kind), size(size){};
};

struct FuncType: VarType{
    std::vector<VarType*> param_list;

    FuncType():VarType(Func){}
};

struct StructType: VarType{
    std::vector<std::pair<std::string, VarType*>> member;
    std::string name;
};

struct VarInfo{
    VarType *type;
    std::string name;
};