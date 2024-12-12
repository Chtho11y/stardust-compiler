#include "parse.h"
#include "ast.h"
#include "var_type.h"
#include "context.h"
#include "arg_parse.h"
#include "util.h"
#include "ir_node.h"
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

                std::cout << info->name << "#" << info->var_id << ": " << info->type->to_string();
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
            std::cout << name << ": " << t->type->to_string() << std::endl;
        }
        for(auto ch: rt->ch)
            print_sym_table(ch, dep + 2);
        print_aligned(dep, "}\n");
    }else{
        for(auto ch: rt->ch)
            print_sym_table(ch, dep);
    }
}

std::vector<std::string> raw_code;

using ansi::color;

void read_raw_code(FILE* file){
    char c = fgetc(file);
    std::string str = "";
    while(c != EOF){
        if(c == '\n'){
            raw_code.push_back(str);
            str = "";
        }else{
            str.push_back(c);
        }
        c = fgetc(file);
    }
    if(!str.empty())
        raw_code.push_back(str);
    fseek(file, 0, SEEK_SET);
}

void pretty_print_line(Locator loc){
    if(!loc.has_value())
        return;
    char ch = '^';
    for(int i = loc.line_st; i <= loc.line_ed; ++i){
        printf("%5d | ", i + 1);
        int len = raw_code[i].length();
        int st = 0, ed = len;
        if(i == loc.line_st)
            st = loc.col_l;
        if(i == loc.line_ed)
            ed = loc.col_r + 1;
        for(int j = 0; j < st; ++j)
            putchar(raw_code[i][j]);
        printf("%s", color(ansi::RED, ansi::BOLD).c_str());
        for(int j = st; j < ed; ++j)
            putchar(raw_code[i][j]);
        printf("%s", color(ansi::RESET).c_str());
        for(int j = ed; j < len; ++j)
            putchar(raw_code[i][j]);
        puts("");
        printf("%5s | ", " ");
        for(int j = 0; j < st; ++j)
            putchar(' ');
        printf("%s", color(ansi::RED, ansi::BOLD).c_str());
        for(int j = st; j < ed; ++j){
            putchar(ch);
            ch = '~';
        }
        printf("%s", color(ansi::RESET).c_str());
        for(int j = ed; j < len; ++j)
            putchar(' ');
        puts("");
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

    read_raw_code(file);
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

    build_sym_table(program_root);
    auto err = get_error_list();
    if(parser.print_ast_sym){
        std::cout << "/**************************AST with symbols table**************************/" << std::endl;
        print(program_root, 0);
        std::cout << std::endl;
    }
    // print_sym_table(program_root);
    // if (!err.size())

    if(err.size() > 0){
        std::cout << "/******************************Error Detected******************************/" << std::endl;
        for(auto [s, loc]: err){
            if(s == ""){
                std::cout << "undefined error" << std::endl;
            }else{
                std::cout << color(ansi::BOLD) << parser.input_path << ":" << loc.line_st + 1 << ":" << loc.col_l + 1 << ": "
                        << color(ansi::RESET) << s << std::endl;
                // std::cout  << loc.col_l << " " << loc.col_r << std::endl;
                pretty_print_line(loc);
            }
            // std::cout << (s == "" ? "undefined error" : s) << " (" << loc.line_st + 1<< ", " << loc.col_l + 1 << ") " << std::endl;
        }
        return 0;
    }
    
    try{
        IRContext context;
        auto ir_root = ast_to_spl_ir(program_root);
        ir_root->gen_code(context);
        context.export_ir_code(stdout);
    }catch(std::string s){
        std::cout << s << std::endl;
    }
}
