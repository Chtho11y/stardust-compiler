#pragma once

#include "parse.h"
#include <vector>

enum node_type{
    Program, 
    ExtDecl, FuncDecl, VarDecl, StructDecl, StructMem,
    TypeDesc, ConstDesc,
    FuncParams, FuncArgs, TypeList,
    Stmts, StmtsRet, Stmt, Expr, Operator,
    Identifier,
    IntLiteral,
    DoubleLiteral,
    IfStmt
};

enum class op_type{
    Add, Sub, Mul, Div, Mod, And, Or, Assign, At, Call, Comma
};

struct AstNode{
    node_type type;
    std::string str;
    Locator loc;
    AstNode* parent;
    std::vector<AstNode*> ch;

    AstNode(){
        parent = nullptr;
    }

    AstNode(node_type type):type(type){
        parent = nullptr;
    }

    AstNode(node_type type, AstNode* parent):
        type(type), parent(parent){}

    AstNode(node_type type, std::string str):
        type(type), parent(parent), str(str){}

    AstNode(node_type type, Token tok):
        type(type), parent(parent), str(tok.val), loc(tok.loc){}

    void append(AstNode* c){
        ch.push_back(c);
        c->parent = this;
    }

    ~AstNode(){
        for(auto c: ch)
            delete c;
    }
};

struct OperatorNode: AstNode{
    op_type type;

    OperatorNode(op_type type):AstNode(Operator), type(type){}

    static std::string get_op_name(OperatorNode* node);

    std::string op_name(){
        return get_op_name(this);
    }
};

template<node_type type>
struct Adaptor{
    
};

template<>
struct Adaptor<VarDecl>{
    std::string id;
    AstNode *init_val;
    AstNode *type;
    Adaptor(AstNode* rt){
        if(rt->type != VarDecl){
            throw "adaptor type mismatch";
        }
        id = rt->ch[0]->str;
        type = rt->ch[1];
        init_val = rt->ch[2];
    }
};

void ast_info_init();
AstNode* create_node_from(node_type type, AstNode* ch);
std::string get_node_name(AstNode* node);

extern AstNode* program_root;