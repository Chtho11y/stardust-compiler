#include "context.h"

error_list errors;

ParserContext parser_context;

void append_error(Locator loc){
    errors.emplace_back(std::string(), loc);
}

void append_error(std::string str) {
    errors.emplace_back(str, Locator());
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

Locator locator_merge(LocatorBuffer l1, LocatorBuffer l2){
    Locator a, b;
    a = l1, b = l2;
    return locator_merge(a, b);
}

Locator locator_merge(LocatorBuffer l1, Locator l2){
    Locator a;
    a = l1;
    return locator_merge(a, l2);
}

Locator locator_merge(Locator l1, LocatorBuffer l2){
    Locator b;
    b = l2;
    return locator_merge(l1, b);
}

Locator locator_merge(Locator l1, Locator l2){
    Locator loc = l1;
    if(loc.line_st > l2.line_st){
        loc.line_st = l2.line_st;
        loc.col_l = l2.col_l;
    }else if(loc.line_st == l2.line_st && loc.col_l > l2.col_l){
        loc.line_st = l2.line_st;
        loc.col_l = l2.col_l;
    }
    if(loc.line_ed < l2.line_ed){
        loc.line_ed = l2.line_ed;
        loc.col_r = l2.col_r;
    }else if(loc.line_ed == l2.line_ed && loc.col_r < l2.col_r){
        loc.line_ed = l2.line_ed;
        loc.col_r = l2.col_r;
    }
    return loc;
}