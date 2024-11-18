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
        std::cout << "TypeDesc: " << rt->str <<std::endl;
        for(auto ch: rt->ch)
            print(ch, dep + 2);
    // }else if(rt->type == VarDecl){
    //     auto var = Adaptor<VarDecl>(rt);
    //     std::cout << "VarDecl: " << var.id << ": " << var.type_info->to_string() <<std::endl;
    //     // print(var.type, dep + 2);
    //     if(var.init_val)
    //         print(var.init_val, dep + 2);
    // }else if(rt->type == FuncDecl){
    //     auto func = Adaptor<FuncDecl>(rt);
    //     std::cout << "FuncDecl: " << func.id << ": " << func.type_info->to_string() << std::endl;
    //     print(func.body, dep + 2);
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

int main(int argc, char* argv[]){
    if (argc == 1) {
        ast_info_init();
        init_type_pool();

        auto file = fopen("../test/test.sd", "r");
        set_input(file);
        yyparse();
        std::string s;
        
        // build_sym_table(program_root, program_root->var_table);
        auto& err = get_error_list();
        if (!err.size())
            print(program_root, 0);

        // auto& err = get_error_list();
        for(auto [s, loc]: err)
            std::cout << (s == "" ? "undefined error" : s) << " (" << loc.line_st + 1<< ", " << loc.col_l + 1 << ") " << std::endl;
    }
    else {
        for (int i = 1; i < argc; i++) {
            ast_info_init();
            init_type_pool();
            // char tmp[1024] = "../test/";
            // strcat(tmp, argv[i]);
            auto file = fopen(argv[i], "r");
            set_input(file);
            yyparse();
            std::string s;
            // build_sym_table(program_root, program_root->var_table);
            auto& err = get_error_list();
            if (!err.size())
                print(program_root, 0);

            // auto& err = get_error_list();
            for(auto [s, loc]: err)
                std::cout << (s == "" ? "undefined error" : s)  << " (" << loc.line_st + 1 << ", " << loc.col_l + 1<< ") " << std::endl;
            delete program_root;
            err.clear();
        }
    }
    
}