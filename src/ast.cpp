#include "ast.h"
#include "context.h"
#include <iostream>

AstNode* create_node_from(node_type type, AstNode* ch){
    auto p = new AstNode(type, nullptr);
    p->append(ch);
    return p;
}

std::string ast_node_name []= {
     "Program", "ExtDecl",
    "FuncDecl", "VarDecl", "StructDecl", "StructMem",
    "TypeDesc", "ConstDesc",
    "ArrayInstance", "StructInstance", "StructInstanceMems", "StructInstanceMem",
    "FuncParams", "FuncArgs", "TypeList", 
    "Stmts", "StmtsRet", "Stmt", "Expr", 
    "Operator", "Identifier", 
    "IntLiteral","Hex", "BoolLiteral", "DoubleLiteral", 
    "IfStmt", "WhileStmt"
};
std::string ast_op_name[] = {"Add", "Sub", "Mul", "Div", "Mod", "And", "Or",
                             "BitAnd", "BitOr", "Xor", "Eq", "Neq", "Le", "Ge", "Lt", "Gt",
                             "Assign", "At", "Call", "Comma", "Access",
                             "Pos", "Neg", "Not", "Convert"};

void ast_info_init(){
    // ast_node_name[Program]      = "Program";
    // ast_node_name[ExtDecl]      = "ExtDecl";
    // ast_node_name[FuncDecl]     = "FuncDecl";
    // ast_node_name[VarDecl]      = "VarDecl";
    // ast_node_name[StructDecl]   = "StructDecl";
    // ast_node_name[StructMem]    = "StructMem";
    // ast_node_name[TypeDesc]     = "TypeDesc";
    // ast_node_name[ConstDesc]    = "ConstDesc";
    // ast_node_name[FuncParams]   = "FuncParams";
    // ast_node_name[FuncArgs]     = "FuncArgs";
    // ast_node_name[TypeList]     = "TypeList";
    // ast_node_name[Stmts]        = "Stmts";
    // ast_node_name[StmtsRet]     = "StmtsRet";
    // ast_node_name[Stmt]         = "Stmt";
    // ast_node_name[Expr]         = "Expr";
    // ast_node_name[Operator]     = "Operator";
    // ast_node_name[Identifier]   = "Identifier";
    // ast_node_name[IntLiteral]   = "IntLiteral";
    // ast_node_name[DoubleLiteral]= "DoubleLiteral";
    // ast_node_name[BoolLiteral]  = "BoolLiteral";
    // ast_node_name[IfStmt]       = "Statement-If";
    // ast_node_name[WhileStmt]    = "Statement-While";
}

std::string get_node_name(AstNode* node){
    return ast_node_name[node->type];
}

std::string OperatorNode::get_op_name(OperatorNode* node){
    return ast_op_name[(int)(node->type)];
}

size_t const_eval(AstNode* node){
    if(node->type == IntLiteral)
        return atoi(node->str.c_str());
    return 114514;
}

std::shared_ptr<VarType> ast_to_func_type(AstNode* node){
    AstNode* args = node->ch[0];
    AstNode* ret = node->ch[1];
    auto res = std::make_shared<FuncType>();
    for(auto ch: args->ch){
        res->param_list.push_back(ast_to_type(ch));
    }
    res->ret_type = ast_to_type(ret);
    return res;
}

std::shared_ptr<VarType> ast_to_type(AstNode* node){

    if(node->type == TypeDesc){
        if(node->str == "()"){
            return ast_to_func_type(node);
        }else if(node->str == "[]"){
            auto res = std::make_shared<ArrayType>();
            res->subtype = ast_to_type(node->ch[0]);
            res->size = const_eval(node->ch[1]);
            return res;
        }else if(node->str == "*"){
            auto res = std::make_shared<PointerType>();
            res->subtype = ast_to_type(node->ch[0]);
            return res;
        }else if(node->str == "#auto"){
            return std::make_shared<AutoType>();
        }else return ast_to_type(node->ch[0]);
    }else if(node->type == TypeList){
        if(node->ch.size() == 1){
            return ast_to_type(node->ch[0]);
        }else{
            return get_type("#err");
        }
    }else if(node->type == Identifier){
        return get_type(node->str);
    }
    return nullptr;
}

std::shared_ptr<VarType> lookup_id(AstNode* node, std::string name){
    // std::cout << "lookup begin" << std::endl;
    int cnt = 0;
    while(node != nullptr){
        // std::cout << get_node_name(node) << std::endl;
        if(node->is_block){
            auto block = static_cast<BlockNode*>(node);
            if(block->var_table.count(name))
                return block->var_table[name].type;
        }
        node = node->parent;
        // std::cout << cnt++ << " round finished @" << node << std::endl;
    }
    // std::cout << "lookup failed" << std::endl;
    append_error("Undefined identifier \'" + name + "\'.");
    return get_type("#err");
}

std::shared_ptr<VarType> build_sym_table(AstNode* node, sym_table& table){
    if(node->is_block){

        auto block = static_cast<BlockNode*>(node);
        std::shared_ptr<VarType> res;
        for(auto ch: block->ch)
            res = build_sym_table(ch, block->var_table);
        if(node->type == StmtsRet)
            return res;
        return get_type("void");  

    }else if(node->type == FuncDecl){

        auto func = Adaptor<FuncDecl>(node);

        if(table.count(func.id)){
            append_error("Function " + func.id + " has been declared.");
            return get_type("#err");
        }

        table[func.id] = {func.id, func.type_info};

        for(auto ch: func.params->ch){
            auto var = Adaptor<VarDecl>(ch).check_type();

            if(!var.type_info->is_error())
                func.body->var_table[var.id] = {var.id, var.type_info};
        }

        auto res = build_sym_table(func.body, table);

        if(!res->is_error() && !func.no_return()){
            require_convertable(res, func.type_info->ret_type);
        }

        return func.type_info;

    }else if(node->type == VarDecl){

        auto var = Adaptor<VarDecl>(node).check_type();

        if(table.count(var.id)){
            append_error("Variable " + var.id + " has been declared.");
            return get_type("#err");
        }

        std::shared_ptr<VarType> res_type = nullptr;
        if(var.init_val)
            res_type = build_sym_table(var.init_val, table);
        if(var.type_info->is_auto()){
            if(res_type == nullptr){
                append_error("Failed to infer the type of \'" + var.id + "\'.");
                return get_type("#err");
            }
            var.type_info = res_type;
        }

        if(res_type){
            require_convertable(res_type, var.type_info);
        }

        table[var.id] = {var.id, var.type_info};
        return get_type("void");

    }else if(node->type == StructDecl){

        auto st = Adaptor<StructDecl>(node);
        if(!set_type(st.id, st.type_info)){
            append_error("type " + st.id + " is declared.");
            return get_type("#err");
        }
        return get_type("void");

    }else if(node->type == Identifier){

        return lookup_id(node, node->str);

    }else if(node->type == Operator){

        for(auto ch: node->ch)
            build_sym_table(ch, table);        
        return get_type("int32");

    }else if(node->type == IfStmt){

        auto cond = build_sym_table(node->ch[0], table);
        require_convertable(cond, get_type("bool"));
        auto ret1 = build_sym_table(node->ch[1], table);
        auto ret2 = build_sym_table(node->ch[2], table);
        return greater_type(ret1, ret2);

    }else if(node->type == WhileStmt){

        auto cond = build_sym_table(node->ch[0], table);
        require_convertable(cond, get_type("bool"));
        build_sym_table(node->ch[1], table);
        return get_type("void");

    }else if(node->is_literal()){

        switch (node->type){
            case IntLiteral: return get_type("int32");
            case DoubleLiteral: return get_type("float64");
            case BoolLiteral: return get_type("bool");
            default: return get_type("#err");
        }

    }else{
        for(auto ch: node->ch)
            build_sym_table(ch, table);
        return get_type("void");
    }
}