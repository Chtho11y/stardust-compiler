#include "ast.h"
#include "context.h"
#include <iostream>

AstContext ast_context;

AstNode* create_node_from(node_type type, AstNode* ch){
    auto p = new AstNode(type, nullptr);
    p->append(ch);
    return p;
}

std::string ast_node_name[] = {
    "Program", "ExtDecl",
    "FuncDecl", "VarDecl", "StructDecl", "StructMem",
    "TypeDesc", "ConstDesc",
    "ArrayInstance", "StructInstance", "StructInstanceMems", "StructInstanceMem",
    "FuncParams", "FuncArgs", "TypeList", 
    "Stmts", "StmtsRet", "Stmt", "Expr", "ExprList",
    "Operator", "Identifier", 
    "IntLiteral", "CharLiteral", "StringLiteral", "BoolLiteral", "DoubleLiteral", 
    "IfStmt", "WhileStmt", "ForStmt",
    "Err"
};
std::string ast_op_name[] = {"Add", "Sub", "Mul", "Div", "Mod", "And", "Or",
                             "BitAnd", "BitOr", "Xor", "Eq", "Neq", "Le", "Ge", "Lt", "Gt",
                             "Assign", "AddEq", "SubEq", "MulEq", "DivEq", "At", "Call", "Comma", "Access",
                             "Pos", "Neg", "Not", "Convert", "Ref", "Deref"};

void ast_info_init(){
    init_type_pool();
    op_impl_init();
    ast_context = AstContext();
}

std::string get_node_name(AstNode* node){
    return ast_node_name[node->type];
}

std::string OperatorNode::get_op_name(OperatorNode* node){
    return ast_op_name[(int)(node->type)];
}

std::string get_op_name(op_type type){
    return ast_op_name[(int)(type)];
}

var_type_ptr AstNode::get_type(std::string name){
    AstNode *rt = this;
    // std::cout << "@get_type: begin" << std::endl;
    while(rt){
        // std::cout << "@get_type: #" << rt << std::endl;
        if(rt->is_block){
            auto block = static_cast<BlockNode*>(rt);
            if(block->type_table.count(name)){
                return block->type_table[name];
            }
        }
        rt = rt->parent;
    }
    // std::cout << "@get_type: end" << std::endl;
    return ::get_type(name);
}

bool AstNode::set_type(std::string name, var_type_ptr type){
    AstNode *rt = this;
    while(rt){
        if(rt->is_block){
            auto block = static_cast<BlockNode*>(rt);
            if(block->type_table.count(name)){
                return false;
            }else{
                block->type_table[name] = type;
                return true;
            }
        }
        rt = rt->parent;
    }
    return false;
}

var_type_ptr AstNode::get_id(std::string name){
    AstNode *rt = this;
    // std::cout << "@get_id: begin" << std::endl;
    while(rt){
        // std::cout << "@get_id: #" << rt << std::endl;
        if(rt->is_block){
            auto block = static_cast<BlockNode*>(rt);
            if(block->var_table.count(name)){
                return ref_type(block->var_table[name].type);
            }
        }
        rt = rt->parent;
        
    }
    // std::cout << "@get_id: end" << std::endl;
    this->set_id(name, ::get_type("#err")); //add assumption
    return nullptr; //not found
}

bool AstNode::set_id(std::string name, var_type_ptr type){
    AstNode *rt = this;
    while(rt){
        if(rt->is_block){
            auto block = static_cast<BlockNode*>(rt);
            if(block->var_table.count(name)){
                if(block->var_table[name].type->is_error()){
                    block->var_table[name].type = type;
                    return true;
                }
                return false;
            }else{
                block->var_table[name] = {name, type};
                return true;
            }
        }
        rt = rt->parent;
    }
    return false;
}

AstNode* AstNode::get_loop_parent(){
    AstNode *rt = this;
    while(rt){
        if(rt->type == WhileStmt || rt->type == ForStmt)
            break;
        rt = rt->parent;
    }
    return rt;
}

AstNode* AstNode::get_func_parent(){
    AstNode *rt = this;
    while(rt){
        if(rt->type == FuncDecl)
            break;
        rt = rt->parent;
    }
    return rt;
}

//TODO: 
//support for different base (i.e. 0x123)
//error report for error-type or non-literal.
size_t const_eval(AstNode* node){
    if(node->type == IntLiteral){
        return atoi(node->str.c_str());
    }else{
        append_array_len_error(node->loc);
    }
    return 0;
}

var_type_ptr lookup_struct(AstNode* node){
    bool err = false;
    StructType::member_list mem;
    for(auto ch: node->ch){
        auto id = ch->ch[0]->str;
        auto tp = build_sym_table(ch->ch[1]);
        for(auto& [s, t]: mem)
            if(s == id){
                // append_error("member + \'" + id + "\' is defined twice.", ch->ch[0]->loc);
                append_multidef_error("Member", id, ch->ch[0]->loc);
                err = 1;
            }
        if(tp->is_error())
            err = 1;
        if(!err)
            mem.emplace_back(id, tp);
    }
    if(err)
        return get_type("#err");

    auto p = node->parent;
    while(p){
        if(p->is_block){
            auto block = static_cast<BlockNode*>(p);
            auto& tb = block->type_table;
            var_type_ptr res;
            int cnt = 0;
            for(auto& [s, tp]: tb){
                if(tp->is_type(VarType::Struct)){
                    auto st = std::dynamic_pointer_cast<StructType>(tp);
                    if(st->match(mem)){
                        res = tp;
                        cnt++;
                    }
                }
            }
            if(cnt == 1){
                return res;
            }else if(cnt > 1){
                append_infer_failed_error("Failed to infer the type of struct.", node->loc);
                // append_error("Failed to infer the type of struct.", node->loc);
                return get_type("#err");
            }
        }
        p = p->parent;
    }
    append_infer_failed_error("No matched type for struct instance.", node->loc);
    // append_error("No matched type for struct instance.", node->loc);
    return get_type("#err");
}

std::shared_ptr<VarType> ast_to_type(AstNode* node){
    // std::cout << get_node_name(node) << ": " << node->str << std::endl;
    if(node->type == TypeDesc){
        if(node->str == "()"){
            auto res = std::make_shared<FuncType>();
            auto params = node->ch[0];
            auto ret = node->ch[1];
            for(auto ch: params->ch)
                res->param_list.push_back(ast_to_type(ch));
            res->ret_type = ast_to_type(ret);
            return res;
        }else if(node->str == "[]"){
            auto res = std::make_shared<ArrayType>();
            res->subtype = ast_to_type(node->ch[0]);
            res->size = const_eval(node->ch[1]);
            if(res->size <= 0)
                return get_type("#err");
            return res;
        }else if(node->str == "*"){
            auto res = std::make_shared<PointerType>();
            res->subtype = ast_to_type(node->ch[0]);
            return res;
        }else if(node->str == "#auto"){
            return std::make_shared<AutoType>();
        }else if(node->str == "#err"){
            return get_type("#err");
        }else return ast_to_type(node->ch[0]);
    }else if(node->type == TypeList){
        if(node->ch.size() == 0){
            return get_type("void");
        }else if(node->ch.size() == 1){
            return ast_to_type(node->ch[0]);
        }else{
            return get_type("#err");
        }
    }else if(node->type == Identifier){
        return node->get_type(node->str);
    }
    return nullptr;
}

std::shared_ptr<VarType> build_sym_table(AstNode* node){
    // std::cout << get_node_name(node) << std::endl;
    if(node->is_block){

        auto block = static_cast<BlockNode*>(node);
        std::shared_ptr<VarType> res;
        for(auto ch: block->ch)
            res = build_sym_table(ch);
        
        if(node->type == StmtsRet)
            return node->ret_var_type = decay(res);
        return node->ret_var_type = get_type("void");  

    }else if(node->type == FuncDecl){

        auto func = Adaptor<FuncDecl>(node);
        auto flag = node->set_id(func.id, func.type_info);
        node->ret_var_type = func.type_info;

        for(auto ch: func.params->ch){
            auto var = Adaptor<VarDecl>(ch).check_type();

            if(!var.type_info->is_error())
                func.body->var_table[var.id] = {var.id, var.type_info};
        }

        auto res = build_sym_table(func.body);

        bool tail_ret = false;
        if(func.body->ch.size()){
            auto tail = func.body->ch.back();
            if(tail->type == Stmt && tail->str == "return")
                tail_ret = true;
        }
        if(!tail_ret && !res->is_error() && !func.no_return()){
            require_convertable(res, func.type_info->ret_type, func.id_loc);
        }

        if(!flag){
            // append_error("Function " + func.id + " has been declared.", func.id_loc);
            append_multidef_error("Function ", func.id, func.id_loc);
            node->ret_var_type = get_type("#err");
        }

        return node->ret_var_type;

    }else if(node->type == VarDecl){

        auto var = Adaptor<VarDecl>(node).check_type();

        std::shared_ptr<VarType> res_type = nullptr;
        if(var.init_val)
            res_type = build_sym_table(var.init_val);
        if(var.type_info->is_auto()){
            if(res_type == nullptr){
                // append_error("Failed to infer the type of \'" + var.id + "\'.", var.id_loc);
                append_infer_failed_error("Failed to infer the type of \'" + var.id + "\'.", var.id_loc);
                return node->ret_var_type = get_type("#err");
            }
            var.type_info = res_type;
        }

        if(res_type){
            require_convertable(res_type, var.type_info, node->loc);
        }

        if(!node->set_id(var.id, var.type_info)){
            // append_error("Variable \'" + var.id + "\' has been declared.", var.id_loc);
            append_multidef_error("Variable", var.id, var.id_loc);   
            return node->ret_var_type = get_type("#err");
        };
        return node->ret_var_type = get_type("void");

    }else if(node->type == StructDecl){

        auto st = Adaptor<StructDecl>(node).check_type();
        if(!st.type_info)
            return node->ret_var_type = get_type("#err");
        st.type_info->id = ++ast_context.type_id;
        if(!node->set_type(st.id, st.type_info)){
            // append_error("type " + st.id + " has been declared.", st.id_loc);
            append_multidef_error("Type", st.id, st.id_loc);
            return node->ret_var_type = get_type("#err");
        }
        return node->ret_var_type = get_type("void");

    }else if(node->type == Identifier){

        auto res = node->get_id(node->str);
        if(res == nullptr){
            // append_error("variable \'" + node->str + "\' is not declared", node->loc);
            append_nodef_error("Variable", node->str, node->loc);
            res = get_type("#err");
        }
        return node->ret_var_type = res;
    }else if(node->type == Operator){

        auto op = (OperatorNode*)node;
        std::vector<var_type_ptr> tp;

        if(op->type == op_type::Convert){
            tp.push_back(build_sym_table(op->ch[0]));
            tp.push_back(ast_to_type(op->ch[1]));
        }else if(op->type == op_type::Access){
            auto t = build_sym_table(op->ch[0]);
            auto id = op->ch[1]->str;
            if(t->is_error())
                return node->ret_var_type = t;
            bool is_ref = t->is_ref();
            t = decay(t);
            if(t->is_type(VarType::Struct)){
                auto st = std::dynamic_pointer_cast<StructType>(t);
                for(auto [n, tp]: st->member){
                    if(n == id)
                        return node->ret_var_type = is_ref ? ref_type(tp): tp;
                }
            }else if(t->is_array()){
                if(id == "length")
                    return get_type("int32");
            }
            append_invalid_access_error(t, id, node->loc);
            return node->ret_var_type = get_type("#err");
        }else{     
            for(auto ch: node->ch)
                tp.push_back(build_sym_table(ch));
        }

        return node->ret_var_type = op_type_eval(op->type, tp, node->loc);

    }else if(node->type == ArrayInstance) {

        return node->ret_var_type = get_type("#err");
    }else if(node->type == StructInstance) {
        return node->ret_var_type = lookup_struct(node);
    }else if(node->type == IfStmt){

        auto cond = build_sym_table(node->ch[0]);
        require_convertable(cond, get_type("bool"), node->loc);
        auto ret1 = build_sym_table(node->ch[1]);
        auto ret2 = get_type("void");
        if(node->ch[2])
            ret2 = build_sym_table(node->ch[2]);

        return node->ret_var_type = greater_type(ret1, ret2);

    }else if(node->type == WhileStmt){

        auto cond = build_sym_table(node->ch[0]);
        require_convertable(cond, get_type("bool"), node->loc);
        build_sym_table(node->ch[1]);
        return node->ret_var_type = get_type("void");

    }else if(node->type == ForStmt){

        build_sym_table(node->ch[0]);
        if(node->ch[1]->type != Stmt){
            auto cond = build_sym_table(node->ch[1]);
            require_convertable(cond, get_type("bool"), node->loc);
        }
        build_sym_table(node->ch[2]);
        build_sym_table(node->ch[3]);
        return node->ret_var_type = get_type("void");

    }else if(node->is_literal()){

        switch (node->type){
            case IntLiteral: return node->ret_var_type = get_type("int32");
            case DoubleLiteral: return node->ret_var_type = get_type("float64");
            case BoolLiteral: return node->ret_var_type = get_type("bool");
            case CharLiteral: return node->ret_var_type = get_type("char");
            case StringLiteral:{
                //FIXME: escape the string.
                auto res = std::make_shared<ArrayType>();
                res->size = node->str.size();
                res->subtype = get_type("char");
                return node->ret_var_type = res;
            }
            default: return node->ret_var_type = get_type("#err");
        }

    }else if(node->type == ExprList){
        var_type_ptr res;
        for(auto ch: node->ch)
            res = build_sym_table(ch);
        return node->ret_var_type = decay(res);
    }else if(node->type == Stmt){
        if(node->str == "return"){
            auto p = node->get_func_parent();
            if(!p){
                // append_error("return statement out of function body.", node->loc);
                append_misplace_error("return", "function", node->loc);
                return node->ret_var_type = get_type("void");
            }
            auto ret_type = std::dynamic_pointer_cast<FuncType>(p->ret_var_type)->ret_type;
            var_type_ptr res = get_type("void");
            if(node->ch.size())
                res = build_sym_table(node->ch[0]);
            res = decay(res);
            require_convertable(res, ret_type, node->loc);
            return node->ret_var_type = get_type("void");
        }else if(node->str == "break" || node->str == "continue"){
            auto p = node->get_loop_parent();
            if(!p){
                // append_error(node->str + " statement out of loop statement.", node->loc);
                append_misplace_error(node->str, "loop", node->loc);
            }
            return node->ret_var_type = get_type("void");
        }else{
            //noop
            return node->ret_var_type = get_type("void");
        }
    }else if(node->type == FuncArgs){
        auto args = std::make_shared<TupleType>();
        for(auto ch: node->ch){
            args->members.push_back(build_sym_table(ch));
        }
        return node->ret_var_type = args;
    }else if(node->type == Err){
        return node->ret_var_type = get_type("#err");
    }else{
        //unreachable
        for(auto ch: node->ch)
            build_sym_table(ch);
        return node->ret_var_type = get_type("void");
    }

}