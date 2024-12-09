#include "parse.h"
#include "ast.h"
#include "var_type.h"
#include "context.h"
#include "arg_parse.h"
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

void print_aligned(int len, std::string str){
    for(int i = 0; i < len; ++i)
        std::cout << ' ';
    std::cout << str;
}

void print_sym_table(AstNode* rt, int dep = 0){
    if(rt->is_block){
        auto block = static_cast<BlockNode*>(rt);
        print_aligned(dep, "{\n");
        print_aligned(dep+2, "#type_table:\n");
        for(auto [name, t]: block->type_table){
            print_aligned(dep+2, "");
            std::cout << name << ": " << t->to_string() << std::endl;
        }
        print_aligned(dep+2, "#var_table:\n");
        for(auto [name, t]: block->var_table){
            print_aligned(dep+2, "");
            std::cout << name << ": " << t.type->to_string() << std::endl;
        }
        for(auto ch: rt->ch)
            print_sym_table(ch, dep + 2);
        print_aligned(dep, "}\n");
    }else{
        for(auto ch: rt->ch)
            print_sym_table(ch, dep);
    }
}

int main(int argc, char* argv[]){
    ast_info_init();
    ArgParser parser;
    parser.parse(argc, argv);
    
    FILE* file;
    std::string name = parser.input_path;
    file = fopen(name.c_str(), "r");

    if(!file){
        std::cout << "Failed to open file: " << name << std::endl;
        return 0;
    }

    set_input(file);
    yyparse();

    if(parser.output_path != ""){
        freopen(parser.output_path.c_str(), "w", stdout);
    }

    bool err_flag = get_error_list().size();
    if(parser.print_ast){
        std::cout << "/****************************Full AST Structure****************************/" << std::endl;
        plain_print(program_root, 0);
        std::cout << std::endl;
    }

    auto err = get_error_list();
    build_sym_table(program_root);
    if(parser.print_ast_sym){
        std::cout << "/**************************AST with symbols table**************************/" << std::endl;
        print(program_root, 0);
        std::cout << std::endl;
    }
    // print_sym_table(program_root);
    // if (!err.size())
    
    std::cout << "/******************************Error Detected******************************/" << std::endl;
    for(auto [s, loc]: err)
        std::cout << (s == "" ? "undefined error" : s) << " (" << loc.line_st + 1<< ", " << loc.col_l + 1 << ") " << std::endl;
}