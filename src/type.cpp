#include "var_type.h"
#include <map>
#include <memory>

std::map<std::string, std::shared_ptr<VarType>> type_pool;

void init_type_pool(){
    for(int i = 8; i <= 64; i <<= 1){
        type_pool["int" + std::to_string(i)] = std::make_shared<PrimType>(PrimType::Int, i);
        type_pool["uint" + std::to_string(i)] = std::make_shared<PrimType>(PrimType::UInt, i);
    }

    type_pool["float32"] = std::make_shared<PrimType>(PrimType::Float, 32);
    type_pool["float64"] = std::make_shared<PrimType>(PrimType::Float, 64);
    type_pool["char"] = std::make_shared<PrimType>(PrimType::Char, 8);
}

VarType* get_type(std::string name){
    if(type_pool.count(name))
        return type_pool[name].get();
    return nullptr;
}

bool set_type(std::string name, std::shared_ptr<VarType> type){
    if(type_pool.count(name))
        return false;
    type_pool[name] = type;
    return true;
}