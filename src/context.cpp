#include "context.h"

error_list errors;

void append_error(std::string str){
    errors.emplace_back(str, Locator{});
}

void append_error(std::string str, Locator loc){
    errors.push_back(std::make_pair(str, loc));
}

error_list& get_error_list(){
    return errors;
}