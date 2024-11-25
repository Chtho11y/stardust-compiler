#include "parse.h"
#include "ast.h"
#include "var_type.h"
#include "context.h"
#include <iostream>
#include <cstdlib>
#include <cstring>

BlockNode* program_root = nullptr;

int yyparse();

void print(AstNode* rt, int dep){
    for(int i = 0; i < dep; ++i)
        std::cout << " ";
    // std::cout << rt->type << "#";
    if(rt->type == TypeDesc){
        // std::cout << "TypeDesc: " << rt->str <<std::endl;
        // for(auto ch: rt->ch)
        //     print(ch, dep + 2);
        std::cout << "TypeDesc: " << ast_to_type(rt)->to_string() << std::endl;
    }else if(rt->type == StructDecl){
        auto st = Adaptor<StructDecl>(rt);
        std::cout << "StructDecl: " << st.id << std::endl;
        for(auto [id, tp]: st.type_info->member){
            for(int i = 0; i < dep; ++i)
                std::cout << " ";
            std::cout << id << ": " << tp->to_string() << std::endl;  
        }
    }else{
        std::cout << get_node_name(rt);
        if(rt->ret_var_type)
            std::cout << "("  << rt->ret_var_type->to_string()<< ")";
        if(rt->str.size()){
            std::cout << ": " << rt->str;
        }else if(rt->type == Operator){
            std::cout << ": " << static_cast<OperatorNode*>(rt)->op_name();
        }else if(rt->is_block){
            auto bl = static_cast<BlockNode*>(rt);
            bool f = true;
            for(auto [name, info]: bl->var_table){
                if(f){
                    std::cout << ": ";
                    f = false;
                }else std::cout <<", ";

                std::cout << info.name << ": " << info.type->to_string();
            }
        }
        std::cout << std::endl;
        for(auto ch: rt->ch)
            print(ch, dep + 2);
    }
}

void plain_print(AstNode* rt, int dep){
    for(int i = 0; i < dep; ++i)
        std::cout << " ";

    if(rt == nullptr){
        std::cout << "null" << std::endl;
        return;
    }

    std::cout << get_node_name(rt);
    if(rt->str.size()){
        std::cout << ": " << rt->str;
    }if(rt->type == Operator){
        std::cout << ": " << static_cast<OperatorNode*>(rt)->op_name();
    }
    std::cout << std::endl;
    for(auto ch: rt->ch)
        plain_print(ch, dep + 2);
}


int main(int argc, char* argv[]){
    ast_info_init();

    auto file = fopen("../test/test.sd", "r");
    set_input(file);
    yyparse();
    std::string s;
    
    plain_print(program_root, 0);
    build_sym_table(program_root);
    auto& err = get_error_list();

    for(auto [s, loc]: err)
        std::cout << (s == "" ? "undefined error" : s) << " (" << loc.line_st + 1<< ", " << loc.col_l + 1 << ") " << std::endl;

    // if (!err.size())
    print(program_root, 0);

    
}

//TODO
//Locator for all AST node
//Function call
//Function overload support
//For statement
//All error handling