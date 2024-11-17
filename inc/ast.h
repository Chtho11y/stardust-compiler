#pragma once

#include "parse.h"
#include "var_type.h"
#include "context.h"
#include <map>
#include <vector>

enum node_type{
    Program, 
    ExtDecl, FuncDecl, VarDecl, StructDecl, StructMem,
    TypeDesc, ConstDesc,
    ArrayInstance, StructInstance, StructInstanceMems, StructInstanceMem,
    FuncParams, FuncArgs, TypeList,
    Stmts, StmtsRet, Stmt, Expr, Operator,
    Identifier,
    IntLiteral,
    BoolLiteral,
    DoubleLiteral,
    IfStmt, WhileStmt
};

enum class op_type{
    Add, Sub, Mul, Div, Mod, And, Or,
    BitAnd, BitOr, Xor, Eq, Neq, Le, Ge, Lt, Gt,
    Assign, At, Call, Comma, Access,
    Pos, Neg, Not, Convert,
    Ref, DeRef
};

struct AstNode{
    node_type type;
    std::string str;
    Locator loc;
    AstNode* parent;
    std::vector<AstNode*> ch;

    bool is_block = false;

    AstNode(){
        parent = nullptr;
    }

    AstNode(node_type type):type(type){
        parent = nullptr;
    }

    AstNode(node_type type, AstNode* parent):
        type(type), parent(parent){}

    AstNode(node_type type, std::string str):
        type(type), parent(nullptr), str(str){}

    AstNode(node_type type, Token tok):
        type(type), parent(nullptr), str(tok.val), loc(tok.loc){}

    void append(AstNode* c){
        ch.push_back(c);
        c->parent = this;
    }

    ~AstNode(){
        for(auto c: ch)
            delete c;
    }

    bool is_value() const{
        return type == Identifier || is_literal();
    }

    bool is_literal() const{
        return type >= IntLiteral && type <= DoubleLiteral;
    }
};

using sym_table = std::map<std::string, VarInfo>;

struct OperatorNode: AstNode{
    op_type type;

    OperatorNode(op_type type):AstNode(Operator), type(type){}

    OperatorNode(op_type type, AstNode* ch1, AstNode* ch2 = nullptr):AstNode(Operator), type(type){
        append(ch1);
        if(ch2)
            append(ch2);
    }

    static std::string get_op_name(OperatorNode* node);

    std::string op_name(){
        return get_op_name(this);
    }
};

struct BlockNode: AstNode{
    sym_table var_table;

    BlockNode(node_type type): AstNode(type){
        is_block = true;
    }
};

void ast_info_init();
AstNode* create_node_from(node_type type, AstNode* ch);
std::string get_node_name(AstNode* node);

std::shared_ptr<VarType> ast_to_type(AstNode* node);
std::shared_ptr<VarType> build_sym_table(AstNode* node, sym_table& table);

template<node_type type>
struct Adaptor{};

template<>
struct Adaptor<VarDecl>{
    std::string id;
    AstNode *init_val;
    AstNode *type;
    std::shared_ptr<VarType> type_info;
    Adaptor(AstNode* rt){
        if(rt->type != VarDecl){
            throw "adaptor type mismatch";
        }
        id = rt->ch[0]->str;
        type = rt->ch[1];
        init_val = rt->ch[2];

        type_info = ast_to_type(type);
        
    }

    Adaptor<VarDecl>& check_type(){
        if(type_info->is_void()){
            append_error("Variable cannot be declared as void type.");
            type_info = get_type("#err");
        }else if(type_info->is_error()){
            append_error("Unknown type of variable \'" + id + "\'.");
        }
        return *this;
    }
};

template<>
struct Adaptor<StructDecl>{
    std::string id;
    std::shared_ptr<StructType> type_info;
    AstNode* mem;

    Adaptor(AstNode* node){
        if(node->type != StructDecl)
            throw "adaptor type mismatch";
        id = node->ch[0]->str;
        mem = node->ch[1];

        type_info = std::make_shared<StructType>();
        type_info->name = id;

        for(auto ch: mem->ch){
            Adaptor<VarDecl> var(ch);
            type_info->member.emplace_back(var.id, var.type_info);
        }
    }
};

template<>
struct Adaptor<FuncDecl>{
    std::string id;
    BlockNode* body;
    AstNode* params;
    AstNode* ret;

    std::shared_ptr<FuncType> type_info;
    std::vector<std::string> param_name;

    Adaptor(AstNode* node){
        if(node->type != FuncDecl)
            throw "adaptor type mismatch";
        id = node->ch[0]->str;
        params = node->ch[1];
        ret = node->ch[2];
        body = static_cast<BlockNode*>(node->ch[3]);

        type_info = std::make_shared<FuncType>();
        type_info->ret_type = ast_to_type(ret);
        if(type_info->ret_type->is_auto())
            type_info->ret_type = std::make_shared<VoidType>();

        for(auto ch: params->ch){
            auto var = Adaptor<VarDecl>(ch);
            type_info->param_list.push_back(var.type_info);
            param_name.push_back(var.id);
        }
    }

    bool no_return() const{
        return type_info->ret_type->is_void();
    }
};

extern BlockNode* program_root;