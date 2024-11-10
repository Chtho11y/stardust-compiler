#include "parse.h"
#include "ast.h"
#include "var_type.h"
#include <iostream>
#include <cstdlib>

AstNode* program_root = nullptr;

int yyparse();

void print(AstNode* rt, int dep){
    for(int i = 0; i < dep; ++i)
        std::cout << " ";
    // std::cout << rt->type << "#";
    if(rt->type == TypeDesc){
        std::cout << "TypeDesc: " << rt->str <<std::endl;
        for(auto ch: rt->ch)
            print(ch, dep + 2);
    }else if(rt->type == VarDecl){
        auto var = Adaptor<VarDecl>(rt);
        std::cout << "VarDecl: " << var.id <<std::endl;
            print(var.type, dep + 2);
        if(var.init_val)
            print(var.init_val, dep + 2);
    }else{
        std::cout << get_node_name(rt);
        if(rt->str.size()){
            std::cout << ": " << rt->str;
        }else if(rt->type == Operator){
            std::cout << ": " << static_cast<OperatorNode*>(rt)->op_name();
        }
        std::cout << std::endl;
        for(auto ch: rt->ch)
            print(ch, dep + 2);
    }
}

int main(){
    ast_info_init();
    init_type_pool();

    auto file = fopen("../test/test.sd", "r");
    set_input(file);
    yyparse();
    std::string s;
    print(program_root, 0);
}