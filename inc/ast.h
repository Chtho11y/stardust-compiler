#pragma once

#include "parse.h"
#include "var_type.h"
#include "context.h"
#include "error.h"
#include <map>
#include <vector>

enum node_type{
    Program, 
    ExtDecl, FuncDecl, VarDecl, StructDecl, StructMem, TypeDef,
    GenericParams, GenericBlock, GenericImpl,
    TypeDesc, ConstDesc,
    ArrayInstance, StructInstance, StructInstanceMems, StructInstanceMem, StructImpl, MemFuncList,
    FuncParams, FuncArgs, TypeList,
    Stmts, StmtsRet, Stmt, Expr, ExprList, Operator,
    Identifier,
    IntLiteral,
    CharLiteral,
    StringLiteral,
    BoolLiteral,
    DoubleLiteral,
    IfStmt, WhileStmt, ForStmt,
    Err
};

enum class op_type{
    Add, Sub, Mul, Div, Mod, And, Or,
    BitAnd, BitOr, Xor, Eq, Neq, Le, Ge, Lt, Gt,
    Assign, AddEq, SubEq, MulEq, DivEq, At, Call, Comma, Access,
    Pos, Neg, Not, Convert,
    Ref, DeRef
};

struct AstNode{
    node_type type;
    std::string str;
    Locator loc;
    AstNode* parent;
    std::vector<AstNode*> ch;
    std::vector<Locator> loc_list;

    var_type_ptr ret_var_type = nullptr;

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

    AstNode(node_type type, Locator loc):
        type(type), loc(loc){}

    AstNode(node_type type, LocatorBuffer loc):
        type(type), loc(Locator{loc.line_st, loc.line_ed, loc.col_l, loc.col_r}){}

    void append(AstNode* c){
        ch.push_back(c);
        this->loc.merge(c->loc);
        c->parent = this;
    }

    void append_loc(Locator loc) {
        loc_list.push_back(loc);
        this->loc.merge(loc);
    }
    
    void append_loc(LocatorBuffer loc) {
        append_loc(Locator{loc.line_st, loc.line_ed, loc.col_l, loc.col_r});
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

    var_type_ptr get_type(std::string name);
    bool set_type(std::string name, var_type_ptr type);
    void del_type(std::string name);

    var_type_ptr get_id(std::string name);
    bool set_id(std::string name, var_type_ptr type);

    std::shared_ptr<VarInfo> get_info(std::string name);

    AstNode* get_loop_parent();
    AstNode* get_func_parent();

    virtual AstNode* clone() const{
        AstNode* res = new AstNode();
        for(auto node: ch){
            res->append(node->clone());
        }
        res->type = type;
        res->str = str;
        res->loc = loc;
        res->loc_list = loc_list;
        res->ret_var_type = nullptr;
        res->parent = nullptr;
        res->is_block = is_block;
        return res;
    }
};

using sym_table = std::map<std::string, std::shared_ptr<VarInfo>>;
using GenericType = GenericStructType<AstNode>;

struct OperatorNode: AstNode{
    op_type type;

    OperatorNode(op_type type):AstNode(Operator), type(type){}

    OperatorNode(op_type type, AstNode* ch1, Locator loc = {}):AstNode(Operator), type(type){
        append(ch1);
        append_loc(loc);
    }

    OperatorNode(op_type type, AstNode* ch1, AstNode* ch2, Locator loc = {}):AstNode(Operator), type(type){
        append(ch1);
        append(ch2);
        append_loc(loc);
    }

    OperatorNode(op_type type, AstNode* ch1, LocatorBuffer loc):AstNode(Operator), type(type){
        append(ch1);
        append_loc(loc);
    }

    OperatorNode(op_type type, AstNode* ch1, AstNode* ch2, LocatorBuffer loc):AstNode(Operator), type(type){
        append(ch1);
        append(ch2);
        append_loc(loc);
    }

    static std::string get_op_name(OperatorNode* node);

    std::string op_name(){
        return get_op_name(this);
    }

    virtual AstNode* clone() const{
        OperatorNode* res = new OperatorNode(type);
        for(auto node: ch){
            res->append(node->clone());
        }
        res->AstNode::type = AstNode::type;
        res->str = str;
        res->loc = loc;
        res->loc_list = loc_list;
        res->ret_var_type = nullptr;
        res->parent = nullptr;
        res->is_block = is_block;
        return res;
    }
};

struct BlockNode: AstNode{
    sym_table var_table;
    std::map<std::string, var_type_ptr> type_table;

    BlockNode(node_type type): AstNode(type){
        is_block = true;
    }

    virtual AstNode* clone() const{
        BlockNode* res = new BlockNode(type);
        for(auto node: ch){
            res->append(node->clone());
        }
        res->str = str;
        res->loc = loc;
        res->loc_list = loc_list;
        res->ret_var_type = nullptr;
        res->parent = nullptr;
        res->is_block = is_block;
        return res;
    }
};

void ast_info_init();
void op_impl_init();
AstNode* create_node_from(node_type type, AstNode* ch);
std::string get_node_name(AstNode* node);
std::string get_op_name(op_type type);

std::shared_ptr<VarType> ast_to_type(AstNode* node);
std::shared_ptr<VarType> build_sym_table(AstNode* node);

std::shared_ptr<VarType> op_type_eval(op_type op, std::vector<std::shared_ptr<VarType>> args, Locator loc);

void inject_builtin_func(BlockNode* block);

template<node_type type>
struct Adaptor{};

template<>
struct Adaptor<VarDecl>{
    std::string id;
    AstNode *init_val;
    AstNode *type;
    std::shared_ptr<VarType> type_info;
    bool is_const;

    Locator id_loc;

    Adaptor(AstNode* rt){
        if(rt->type != VarDecl){
            // throw "adaptor type mismatch";
            id = "@ERROR";
            type = new AstNode(TypeDesc, "#err");
            type_info = get_type("#err");
            init_val = nullptr;
            id_loc = rt->loc;
            return;
        }
        int p = 0;
        if(rt->ch[0]->type == ConstDesc)
            is_const = rt->ch[p++]->str == "const";
        id_loc = rt->ch[p]->loc;
        id = rt->ch[p++]->str;
        type = rt->ch[p++];
        if(rt->ch.size() > p)
            init_val = rt->ch[p];
        else init_val = nullptr;

        type_info = ast_to_type(type);
        
    }

    Adaptor<VarDecl>& check_type(){
        if(type_info->is_void()){
            append_invalid_decl_error("Variable cannot be declared as void type.", id_loc);
            type_info = get_type("#err");
        }else if(type_info->is_error()){
            append_invalid_decl_error("Unknown type of variable \'" + id + "\'.", id_loc);
        }
        return *this;
    }

    bool is_signed() const{
        return init_val->ret_var_type->is_signed();
    }
};

template<>
struct Adaptor<StructDecl>{
    std::string id;
    std::shared_ptr<StructType> type_info;
    AstNode* mem;

    Locator id_loc;
    std::vector<Locator> mem_loc;

    Adaptor(AstNode* node){
        if(node->type != StructDecl)
            throw "adaptor type mismatch";
        id = node->ch[0]->str;
        id_loc = node->ch[0]->loc;
        mem = node->ch[1];

        type_info = std::make_shared<StructType>();
        type_info->name = id;
    }

    Adaptor<StructDecl>& build_type(){
    
        for(auto ch: mem->ch){
            Adaptor<VarDecl> var(ch);
            type_info->member.emplace_back(var.id, var.type_info);
            mem_loc.push_back(var.id_loc);
        }
        return *this;
    }

    Adaptor<StructDecl>& check_type(){
        int cnt = type_info->member.size();

        if(type_info->is_error())
            return *this;
        
        for(int i = 0; i < cnt; ++i){
            auto tp = type_info->member[i].second;
            if(tp->is_error()){
                type_info = nullptr;
                return *this;
            }
            if(tp->is_void()){
                append_invalid_decl_error("Struct member cannot be declared as void type.", mem_loc[i]);
                type_info = nullptr;
                return *this;
            }
        }

        for(int i = 0; i < cnt; ++i)
            for(int j = i + 1; j < cnt; ++j)
                if(type_info->member[i].first == type_info->member[j].first){
                    auto nam = type_info->member[i].first;
                    // append_error("member \'"+ nam + "\' is defined twice.", mem_loc[i]);
                    append_multidef_error("Member", nam, mem_loc[i]);
                    type_info = nullptr;
                }
        
        if(type_info->contain_loop()){
            append_invalid_decl_error("Struct " + id + " has infinity size.", id_loc);
            type_info = nullptr;
        }

        return *this;
    }
};

template<>
struct Adaptor<FuncDecl>{
    std::string id;
    BlockNode* body;
    AstNode* params;
    AstNode* ret;

    Locator id_loc;

    std::shared_ptr<FuncType> type_info;
    std::vector<std::string> param_name;

    Adaptor(AstNode* node){
        // std::cout << "func adaptor build begin" << std::endl;
        if(node->type != FuncDecl)
            throw "adaptor type mismatch";
        id = node->ch[0]->str;
        id_loc = node->ch[0]->loc;
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
        // std::cout << "func adaptor built" << std::endl;
    }

    bool no_return() const{
        return type_info->ret_type->is_void();
    }
};

template<>
struct Adaptor<GenericBlock>{

    std::string name;
    BlockNode* rt;
    AstNode *proto, *impl_list;
    std::vector<std::string> params;

    Adaptor<GenericBlock>(AstNode* node){
        if(node->type != GenericBlock)
            throw "adaptor type mismatch";
        rt = (BlockNode*)node;
        proto = node->ch[0];
        impl_list = node->ch[2];
        for(auto ch: node->ch[1]->ch){
            params.push_back(ch->str);
        }
        name = proto->ch[0]->str;
    }
};

extern BlockNode* program_root;