#include "ast.h"

AstNode* create_node_from(node_type type, AstNode* ch){
    auto p = new AstNode(type, nullptr);
    p->append(ch);
    return p;
}

std::string ast_node_name[200];
std::string ast_op_name[] = {"Add", "Sub", "Mul", "Div", "Mod", "And", "Or",
                             "BitAnd", "BitOr", "Xor", "Eq", "Neq", "Le", "Ge", "Lt", "Gt",
                             "Assign", "At", "Call", "Comma", "Access",
                             "Pos", "Neg", "Not"};

void ast_info_init(){
    ast_node_name[Program]      = "Program";
    ast_node_name[ExtDecl]      = "ExtDecl";
    ast_node_name[FuncDecl]     = "FuncDecl";
    ast_node_name[VarDecl]      = "VarDecl";
    ast_node_name[StructDecl]   = "StructDecl";
    ast_node_name[StructMem]    = "StructMem";
    ast_node_name[TypeDesc]     = "TypeDesc";
    ast_node_name[ConstDesc]    = "ConstDesc";
    ast_node_name[FuncParams]   = "FuncParams";
    ast_node_name[FuncArgs]     = "FuncArgs";
    ast_node_name[TypeList]     = "TypeList";
    ast_node_name[Stmts]        = "Stmts";
    ast_node_name[StmtsRet]     = "StmtsRet";
    ast_node_name[Stmt]         = "Stmt";
    ast_node_name[Expr]         = "Expr";
    ast_node_name[Operator]     = "Operator";
    ast_node_name[Identifier]   = "Identifier";
    ast_node_name[IntLiteral]   = "IntLiteral";
    ast_node_name[DoubleLiteral]= "DoubleLiteral";
    ast_node_name[IfStmt]       = "Statement-If";
    ast_node_name[WhileStmt]    = "Statement-While";
}

std::string get_node_name(AstNode* node){
    return ast_node_name[node->type];
}

std::string OperatorNode::get_op_name(OperatorNode* node){
    return ast_op_name[(int)(node->type)];
}

std::shared_ptr<VarType> ast_to_type(AstNode* node){
    
}

void build_sym_table(AstNode* node){

}