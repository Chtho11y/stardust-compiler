#include "context.h"

error_list errors;

void append_error(Locator loc){
    errors.emplace_back(std::string(), loc);
}

void append_error(std::string str) {
    errors.emplace_back(std::string(), Locator());
}

void set_error_message(std::string str) {
    (*--errors.end()).first = str;
}

void append_error(std::string str, Locator loc){
    errors.push_back(std::make_pair(str, loc));
}

error_list& get_error_list(){
    return errors;
}