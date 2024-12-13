#pragma once

#include "parse.h"
#include <vector>
#include <set>
#include <iostream>



struct ParserContext{
    int id = 0;
    bool ignore;
    std::vector<std::set<std::string>> var_table, type_table;

    void push_block_env(){
        var_table.emplace_back();
        type_table.emplace_back();
    }

    void pop_block_env(){
        var_table.pop_back();
        type_table.pop_back();
    }

    void set_ignore(){
        // std::cout << "set at " << CurrentCursor.line_st << ", " << CurrentCursor.col_l << std::endl; 
        ignore = 1;
    }

    bool get_ignore(){
        auto res = ignore;
        ignore = 0;
        // std::cout << "reset at " << CurrentCursor.line_st << ", " << CurrentCursor.col_l << std::endl; 
        return res;
    }

    bool is_type(std::string name){
        int n = var_table.size();
        for(int i = n - 1; i >= 0; --i){
            if(var_table[i].count(name))
                return false;
            if(type_table[i].count(name))
                return true;
        }
        return false;
    }

    void set_var(std::string name){
        var_table.back().insert(name);
    }

    void set_type(std::string name){
        type_table.back().insert(name);
    }
};

struct AstContext{
    int type_id = 0;
    int var_id = 0;
};

extern ParserContext parser_context;
extern AstContext ast_context;
extern std::set<std::pair<Locator, std::string>> LocatorSet;
extern std::vector<Locator> LocatorStack;

void insert_locator(Locator loc, std::string val);

Locator get_next_locator(Locator loc);

Locator locator_merge(LocatorBuffer l1, LocatorBuffer l2);

Locator locator_merge(LocatorBuffer l1, Locator l2);

Locator locator_merge(Locator l1, LocatorBuffer l2);

Locator locator_merge(Locator l1, Locator l2);