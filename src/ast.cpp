#include "ast.h"
#include "context.h"
#include "sdtype.h"
#include "literal_process.h"
#include <iostream>

using namespace sd;

AstContext ast_context;

AstNode* create_node_from(node_type type, AstNode* ch){
    auto p = new AstNode(type, nullptr);
    p->append(ch);
    // p->loc = ch->loc;
    return p;
}

var_type_ptr build_sym_table_with_assum(AstNode* node, var_type_ptr assum);

std::string ast_node_name[] = {
    "Program", "ExtDecl",
    "FuncDecl", "VarDecl", "StructDecl", "StructMem", "TypeDef", "FuncType", 
    "GenericParams", "GenericBlock", "GenericImpl",
    "TypeDesc", "ConstDesc",
    "ArrayInstance", "StructInstance", "StructInstanceMems", "StructInstanceMem", "StructImpl", "MemFuncList", 
    "TraitDecl", "TraitImpl", "TraitFuncList",
    "FuncParams", "FuncArgs", "TypeList", 
    "Stmts", "StmtsRet", "Stmt", "Expr", "ExprList",
    "Operator", "Identifier", 
    "IntLiteral", "CharLiteral", "StringLiteral", "BoolLiteral", "PointerLiteral", "DoubleLiteral", 
    "IfStmt", "WhileStmt", "ForStmt",
    "Err"
};
std::string ast_op_name[] = {"Add", "Sub", "Mul", "Div", "Mod", "And", "Or",
                             "BitAnd", "BitOr", "Xor", "Eq", "Neq", "Le", "Ge", "Lt", "Gt",
                             "Assign", "AddEq", "SubEq", "MulEq", "DivEq", "At", "Call", "Comma", "Access",
                             "Pos", "Neg", "Not", "ImplicitConvert", "Convert", "Ref", "Deref"};

void ast_info_init(){
    sd::type_ctx_init();
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

var_type_ptr expect_ret_type(AstNode* node, var_type_ptr expect){
    if(node->ret_var_type == nullptr)
        build_sym_table_with_assum(node, expect);
    if(node->ret_var_type == expect)
        return expect;
    require_convertable(node->ret_var_type, expect, node->loc);
    auto parent = node->parent;
    auto cvt = new OperatorNode(op_type::ImpConvert, node, node->loc);
    cvt->ret_var_type = expect;
    for(auto& ch: parent->ch)
        if(ch == node){
            ch = cvt;
            cvt->parent = parent;
        }
    return expect;
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
    if(auto res = get_type_context().get(name))
        return res;
    return ErrorType::get();
}

bool AstNode::set_type(std::string name, var_type_ptr type){
    AstNode *rt = this;
    while(rt){
        if(rt->type != GenericBlock && rt->is_block){
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

void AstNode::del_type(std::string name){
    AstNode *rt = this;
    while(rt){
        if(rt->type != GenericBlock && rt->is_block){
            auto block = static_cast<BlockNode*>(rt);
            block->type_table.erase(name);
            return;
        }
        rt = rt->parent;
    }
}

var_type_ptr AstNode::get_id(std::string name){
    AstNode *rt = this;
    bool skip = false;
    while(rt){
        // std::cout << "@get_id: #" << rt << std::endl;
        if(rt->is_block && (rt->type == ExtDecl || !skip)){
            auto block = static_cast<BlockNode*>(rt);
            if(block->var_table.count(name)){
                return RefType::get(block->var_table[name]->type);
            }
        }
        
        rt = rt->parent;
        if(rt && rt->type == FuncDecl)
            skip = true;
    }
    // std::cout << "@get_id: end" << std::endl;
    this->set_id(name, ErrorType::get()); //add assumption
    return nullptr; //not found
}

var_info_ptr AstNode::get_info(std::string name){
    AstNode *rt = this;
    // std::cout << "@get_info: begin: " << name << std::endl;
    while(rt){
        // std::cout << "@get_info: #" << rt << std::endl;
        if(rt->is_block){
            auto block = static_cast<BlockNode*>(rt);
            if(block->var_table.count(name)){
                return block->var_table[name];
            }
        }
        rt = rt->parent;
    }
    return nullptr;
}

bool AstNode::set_id(std::string name, var_type_ptr type){
    AstNode *rt = this;
    while(rt){
        if(rt->is_block){
            auto block = static_cast<BlockNode*>(rt);
            if(block->var_table.count(name)){
                if(block->var_table[name]->type->is_error()){
                    block->var_table[name]->type = type;
                    return true;
                }
                return false;
            }else{
                block->var_table[name] = std::make_shared<VarInfo>(VarInfo{name, type, rt->type == ExtDecl, ast_context.var_id++});
                return true;
            }
        }
        rt = rt->parent;
    }
    return false;
}

void inject_builtin_func(BlockNode* block){
    auto i32_tp = PrimType::get_int(32);
    auto i64_tp = PrimType::get_int(64);
    auto void_tp = VoidType::get();
    auto char_ptr = PointerType::get(PrimType::get_char());
    auto void_ptr = PointerType::get(void_tp);

    auto fn_void_to_i32 = FuncType::get({}, i32_tp);
    auto fn_i32_to_void = FuncType::get({i32_tp}, void_tp);
    auto fn_i32_to_i32 = FuncType::get({i32_tp}, i32_tp);
    auto fn_cptr_to_i32 = FuncType::get({char_ptr}, i32_tp); 
    auto fn_cptr_to_i32_vary = FuncType::get({char_ptr}, i32_tp, true); 
    auto fn_voidptr_to_i32 = FuncType::get({void_ptr}, i32_tp); 
    auto fn_i64_to_voidptr = FuncType::get({i64_tp}, void_ptr); 
    auto fn_void_to_i64 = FuncType::get({}, i64_tp); 

    block->set_id("read", fn_void_to_i32);
    block->set_id("write", fn_i32_to_void);

    block->set_id("exit", fn_i32_to_void);

    block->set_id("putchar", fn_i32_to_i32);
    block->set_id("getchar", fn_void_to_i32);
    block->set_id("puts", fn_cptr_to_i32);

    block->set_id("free", fn_voidptr_to_i32);
    block->set_id("malloc", fn_i64_to_voidptr);

    block->set_id("clock", fn_void_to_i64);

    block->set_id("printf", fn_cptr_to_i32_vary);
    block->set_id("scanf", fn_cptr_to_i32_vary);
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

size_t const_eval(AstNode* node){
    if(node->type == IntLiteral){
        return parse_int(node->str);
    }else{
        append_array_len_error(node->loc);
    }
    return 0;
}

var_type_ptr lookup_struct(AstNode* node, var_type_ptr assum = nullptr){
    bool err = false;
    StructType::member_list mem;
    StructType* st = nullptr;
    if(assum && assum->is_struct()){
        st = dyn_ptr_cast<StructType>(assum);
    }

    for(auto ch: node->ch){
        auto id = ch->ch[0]->str;
        auto tp = build_sym_table_with_assum(ch->ch[1], (st? st->type_of(id): nullptr));
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
        return ErrorType::get();
    
    if(st && st->match(mem))
        return st;

    append_infer_failed_error("Memeber mismatch for struct instance.", node->loc);
    return ErrorType::get();
}

var_type_ptr infer_array(AstNode* node, var_type_ptr type_assump = nullptr){
    if(node->type != ArrayInstance){
        return build_sym_table(node);
    }

    if(type_assump && type_assump->is_array()){
        ArrayType* arr = dyn_ptr_cast<ArrayType>(type_assump);
        auto res = ArrayType::get(arr->subtype, std::max(node->ch.size(), arr->len));

        for(auto ch: node->ch){
            expect_ret_type(ch, arr->subtype);
        }
        return node->ret_var_type = res;
    }else{
        var_type_ptr comm_tp = nullptr;
        for(auto ch: node->ch){
            auto ch_tp = build_sym_table(ch);
            comm_tp = comm_tp? greater_type(comm_tp, ch_tp): ch_tp;   
        }
        if(!comm_tp || comm_tp->is_void()){
            append_infer_failed_error("Failed to infer the type of array", node->loc);
            return node->ret_var_type = ErrorType::get();
        }else{
            for(auto ch: node->ch){
                expect_ret_type(ch, comm_tp);
            }
        }
        return node->ret_var_type = ArrayType::get(comm_tp, node->ch.size());
    }
}

var_type_ptr add_generic_impl(AstNode* node, std::vector<var_type_ptr>& args, std::string& sig){

    if(ast_context.generic_sub_cnt > 20){
        append_generic_error("Generic recursive replacements exceeds the limit", node->loc);
        return ErrorType::get();
    }

    ast_context.generic_sub_cnt++;
    var_type_ptr res = ErrorType::get();
    AstNode* impl;
    std::string nam;

    auto gen = Adaptor<GenericBlock>(node);
    auto rt = gen.rt;
    nam = gen.name;
    for(size_t i = 0; i < args.size(); ++i){
        rt->type_table[gen.params[i]] = args[i]; 
    }

    auto buffer = rt->type_table;

    try{
        impl = gen.proto->clone();

        impl->ch[0]->str = sig;
        impl->parent = gen.impl_list;

        build_sym_table(impl);

        res = impl->get_type(sig);
        gen.impl_list->append(impl);

    }catch(generic_exception _){
        res = ErrorType::get();
        delete impl;
    }

    rt->type_table = buffer;

    ast_context.generic_sub_cnt--;
    return res;
}

var_type_ptr ast_to_type(AstNode* node){
    // std::cout << get_node_name(node) << ": " << node->str << std::endl;
    if(node->ret_var_type)
        return node->ret_var_type;

    if(node->type == TypeDesc){
        if(node->str == "()"){
            auto params = node->ch[0];
            auto ret = node->ch[1];
            std::vector<var_type_ptr> param_list;
            var_type_ptr ret_type;
            for(auto ch: params->ch)
                param_list.push_back(ast_to_type(ch));
            ret_type = ast_to_type(ret);
            auto res = FuncType::get(param_list, ret_type);
            auto ptr = PointerType::get(res);
            return node->ret_var_type = ptr;
        }else if(node->str == "[]"){
            auto sub = node;
            while(sub->str == "[]")
                sub = sub->ch[0];
            
            auto res = ast_to_type(sub);
            if(res->is_error())
                return node->ret_var_type = ErrorType::get();

            if(res->is_void() || res->is_generic() || res->is_ref()){
                append_invalid_decl_error("Cannot declare array of type \'" + res->to_string() + "\'.", sub->loc);
                return node->ret_var_type = ErrorType::get();
            }
        
            for(auto nd = node; nd != sub; nd = nd->ch[0]){
                auto len = const_eval(nd->ch[1]);
                if(len == 0)
                    return node->ret_var_type = ErrorType::get();
                auto subtype = res;
                res = ArrayType::get(subtype, len);
            }
            return res;
        }else if(node->str == "*"){
            auto subtype = ast_to_type(node->ch[0]);
            if(subtype->is_error())
                return ErrorType::get();
            if(subtype->is_ref()){
                append_invalid_decl_error("Cannot declare pointer of type \'" + subtype->to_string() + "\'.", node->ch[0]->loc);
                return node->ret_var_type = ErrorType::get();
            }
            return node->ret_var_type = PointerType::get(subtype);
        }else if(node->str == "&"){
            auto subtype = ast_to_type(node->ch[0]);
            if(subtype->is_error())
                return ErrorType::get();
            if(subtype->is_ref() || subtype->is_void() || subtype->is_generic())
                append_invalid_decl_error("Invalid reference for type \'" + subtype->to_string() + "\'.", node->ch[0]->loc);
            return node->ret_var_type = RefType::get(subtype);
        }else if(node->str == "@") {
            auto res = build_sym_table(node->ch[0]);
            if (res->is_error()) 
                append_infer_failed_error("Failed to infer the type of expression.", node->ch[0]->loc);
            return node->ret_var_type = res->decay();
        }else if(node->str == "<>"){
            auto proto = ast_to_type(node->ch[0]);
            if(!proto->is_generic()){
                if(!proto->is_error())
                    append_generic_error("Cannot use " + proto->to_string() + " as generic type.", node->ch[0]->loc);
                return node->ret_var_type = ErrorType::get();
            }

            auto gen = proto->cast<GenericType>();

            auto args = node->ch[1];
            std::vector<var_type_ptr> arg_list;
            for(auto ch: args->ch){
                auto tp = ast_to_type(ch);
                if(tp->is_error())
                    return node->ret_var_type = ErrorType::get();
                arg_list.push_back(tp);
            }

            if(arg_list.size() != gen->param_list.size()){
                append_generic_error("Generic parameters mismatch.", node->loc);
                return node->ret_var_type = ErrorType::get();
            }

            std::string signature = TupleType(arg_list).to_string();
            signature.front() = '<';
            signature.back() = '>';
            signature = gen->name + "#" + std::to_string(gen->id) + signature;

            auto ret = gen->src->get_type(signature);

            if(!ret->is_error())
                return node->ret_var_type = ret;
            
            auto res = add_generic_impl(gen->src, arg_list, signature);
            if(res->is_error()){
                append_generic_error("failed to substitute generic type " + signature, node->loc);
            }
            return node->ret_var_type = res;

        }else if(node->str == "#auto"){
            return node->ret_var_type = AutoType::get();
        }else if(node->str == "#err"){
            return node->ret_var_type = ErrorType::get();
        }else return node->ret_var_type = ast_to_type(node->ch[0]);
    }else if(node->type == TypeList){
        if(node->ch.size() == 0){
            return node->ret_var_type = VoidType::get();
        }else if(node->ch.size() == 1){
            return node->ret_var_type = ast_to_type(node->ch[0]);
        }else{
            return node->ret_var_type = ErrorType::get();
        }
    }else if(node->type == Identifier){
        return node->ret_var_type = node->get_type(node->str);
    }else if(node->type == Err){
        return node->ret_var_type = ErrorType::get();
    }
    return nullptr;
}

var_type_ptr build_sym_table_with_assum(AstNode* node, var_type_ptr assum){
    if(assum == nullptr || assum->is_error() || assum->is_void() || assum->is_auto())
        return node->ret_var_type = build_sym_table(node);
    if(node->type == ArrayInstance){
        return node->ret_var_type = infer_array(node, assum);
    }else  if(node->is_literal()){
        switch (node->type){
            case IntLiteral: {
                if(!assum->is_prim()) return build_sym_table(node);
                auto assum_tp = dyn_ptr_cast<PrimType>(assum);
                auto literal_tp = PrimType::get_int(32);
                if(assum_tp->kind > literal_tp->kind)
                    return build_sym_table(node);
                return node->ret_var_type = assum_tp;
            }
            case DoubleLiteral:{
                if(!assum->is_prim()) return build_sym_table(node);
                auto assum_tp = dyn_ptr_cast<PrimType>(assum);
                auto literal_tp =  PrimType::get_float(64);
                if(assum_tp->kind != literal_tp->kind)
                    return build_sym_table(node);
                return node->ret_var_type = assum_tp;
            }
            case PointerLiteral:{
                return assum->is_ptr() ? node->ret_var_type = assum: build_sym_table(node);
            }
            default: return build_sym_table(node);
        }
    }else return build_sym_table(node);
}
    
void check_prim_shadow(std::string id, Locator loc){
    if(auto tp = get_type_context().get(id)){
        if(tp->is_prim())
            append_prim_shadowed_warning(id, loc);
    }
}

// void impl_mem_func(AstNode* &node) {
//     for (auto &ch : node->ch)
//         impl_mem_func(ch);
//     if (node->type == Identifier && node->str == "self") {
//         auto op_node = new OperatorNode(op_type::DeRef);
//         op_node->parent = node->parent;
//         op_node->append(node);
//         node = op_node;
//     }
// }

void build_funcbody(AstNode* node, std::vector<std::pair<std::string, var_type_ptr>> inject_var_list = {}) {
    auto func = Adaptor<FuncDecl>(node);

    for(auto ch: func.params->ch){
        auto var = Adaptor<VarDecl>(ch).check_type();
        check_prim_shadow(var.id, var.id_loc);
        if(!var.type_info->is_error())
            func.body->var_table[var.id] = std::make_shared<VarInfo>(VarInfo{var.id, var.type_info, 0, ast_context.var_id++});
    }
    if (inject_var_list.size())
        for (auto &[id, type_info] : inject_var_list) {
            func.body->var_table[id] = std::make_shared<VarInfo>(VarInfo{id, type_info, 0, ast_context.var_id++});
        }

    auto res = build_sym_table(func.body);

    bool tail_ret = false;
    if(func.body->ch.size()){
        auto tail = func.body->ch.back();
        if(tail->type == Stmt && tail->str == "return")
            tail_ret = true;
    }
    if(!tail_ret && !res->is_error() && !func.no_return()){
        require_convertable(res, func.type_info->ret_type, func.body->ch.size() ? func.body->ch.back()->loc : func.body->loc_list.back());
    }
}

var_type_ptr build_sym_table(AstNode* node){
    // std::cout << get_node_name(node) << ":" << node->str << std::endl;
    if(node->type == GenericBlock){
        auto block = static_cast<BlockNode*>(node);
        auto param = block->ch[1];
        std::vector<std::string> param_list;
        for(auto ch: param->ch){
            if(block->type_table.count(ch->str))
                append_multidef_error("Generic parameter", ch->str, ch->loc);
            else
                block->type_table[ch->str] = GenericParamType::get(ch->str);
            param_list.push_back(ch->str);
        }
        auto name = node->ch[0]->ch[0]->str;
        auto tp = GenericType::create(node, name, param_list);
        node->set_type(name, tp);
        node->append(new AstNode(GenericImpl));
        return node->ret_var_type = VoidType::get();
    }else if(node->is_block){

        auto block = static_cast<BlockNode*>(node);
        var_type_ptr res;
        for(auto ch: block->ch)
            res = build_sym_table(ch);
        
        if(node->type == StmtsRet){
            return node->ret_var_type = expect_ret_type(block->ch.back(), res->decay());
        }
        return node->ret_var_type = VoidType::get();  

    }else if(node->type == FuncDecl){

        auto func = Adaptor<FuncDecl>(node);
        check_prim_shadow(func.id, func.id_loc);
        auto flag = node->set_id(func.id, func.type_info);
        node->ret_var_type = func.type_info;

        build_funcbody(node);

        if(!flag){
            append_multidef_error("Function", func.id, func.id_loc);
            node->ret_var_type = ErrorType::get();
        }
        return node->ret_var_type;

    }else if(node->type == VarDecl){
        auto var = Adaptor<VarDecl>(node).check_type();
        var_type_ptr res_type = nullptr;

        if(var.init_val){
            res_type = build_sym_table_with_assum(var.init_val, var.type_info);
        }
        if(var.type_info->is_auto()){
            if(res_type == nullptr){
                // append_error("Failed to infer the type of \'" + var.id + "\'.", var.id_loc);
                append_infer_failed_error("Failed to infer the type of \'" + var.id + "\'.", var.id_loc);
                return node->ret_var_type = ErrorType::get();
            }

            if(res_type->decay()->is_func())
                var.type_info = PointerType::get(res_type);
            else var.type_info = res_type->decay();
        }else if(var.type_info->is_ref()){
            if(!node->get_func_parent())
                append_invalid_decl_error("Reference for global variable is not supported.", var.id_loc);
            else if(!var.init_val)
                append_invalid_decl_error("Declare reference without initial value.", var.id_loc);
        }

        if(res_type)
            expect_ret_type(var.init_val, var.type_info);
        
        check_prim_shadow(var.id, var.id_loc);
        if(!node->set_id(var.id, var.type_info)){
            // append_error("Variable \'" + var.id + "\' has been declared.", var.id_loc);
            append_multidef_error("Variable", var.id, var.id_loc);   
            return node->ret_var_type = ErrorType::get();
        };
        return node->ret_var_type = VoidType::get();

    }else if(node->type == StructDecl){

        auto st = Adaptor<StructDecl>(node);

        auto tp = st.type_info;

        if(!node->set_type(st.id, st.type_info)){
            // append_error("type " + st.id + " has been declared.", st.id_loc);
            append_multidef_error("Type", st.id, st.id_loc);
            return node->ret_var_type = ErrorType::get();
        }

        st.build_type().check_type();

        if(!st.type_info){
            node->del_type(st.id);
            if(ast_context.generic_sub_cnt > 0)
                throw generic_exception();
            return node->ret_var_type = ErrorType::get();
        }
        
        check_prim_shadow(st.id, st.id_loc);

        return node->ret_var_type = st.type_info;

    }else if (node->type == TypeDef) {
        auto tp_name = node->ch[0]->str;
        auto type_info = ast_to_type(node->ch[1]);
        if (type_info->is_error()) {
            append_invalid_decl_error("Invalid type of \'" + tp_name + "\'.", node->ch[0]->loc);
            return node->ret_var_type = ErrorType::get();
        }
        check_prim_shadow(tp_name, node->ch[0]->loc);
        if (!node->set_type(tp_name, type_info)) {
            append_multidef_error("Type", tp_name, node->ch[0]->loc);
            return node->ret_var_type = ErrorType::get();
        }
        return node->ret_var_type = VoidType::get();

    }else if(node->type == Identifier){

        // CHECK_PRIM_SHADOW(node->str, node->loc);
        auto res = node->get_id(node->str);
        if(res == nullptr){
            // append_error("variable \'" + node->str + "\' is not declared", node->loc);
            append_nodef_error("Variable", node->str, node->loc);
            res = ErrorType::get();
        }
        return node->ret_var_type = res;
    }else if(node->type == Operator){

        auto op = (OperatorNode*)node;
        std::vector<var_type_ptr> tp;

        if(op->type == op_type::Convert){
            auto ltp = ast_to_type(op->ch[1]);
            auto val = build_sym_table_with_assum(op->ch[0], ltp);
            tp.push_back(val);
            tp.push_back(ltp);
        }else if(op->type == op_type::Access){
            auto t = build_sym_table(op->ch[0]);
            auto id = op->ch[1]->str;
            if(t->is_error())
                return node->ret_var_type = t;
            bool is_ref = t->is_ref() | t->is_ptr();
            t = t->decay()->deref();
            if(t->is_struct()){
                auto st = dyn_ptr_cast<StructType>(t);
                for(auto [n, tp]: st->member){
                    if(n == id)
                        return node->ret_var_type = is_ref ? RefType::get(tp): tp;
                }
            }else if(t->is_array()){
                if(id == "length")
                    return node->ret_var_type = PrimType::get_int(32);
            }
            auto mem_func_id = t->to_string() + "$" + id;
            auto mem_func_type = op->get_id(mem_func_id);
            if (mem_func_type != nullptr) {
                return node->ret_var_type = mem_func_type;
            }
            append_invalid_access_error(t, id, node->loc_list[0]);
            return node->ret_var_type = ErrorType::get();
        }else{     
            for(auto ch: node->ch)
                tp.push_back(build_sym_table(ch));
        }
        return node->ret_var_type = op_type_eval(op, tp);

    }else if(node->type == ArrayInstance) {
        return node->ret_var_type = infer_array(node);
    }else if(node->type == StructInstance) {
        auto tp = ast_to_type(node->ch[1]);
        return node->ret_var_type = lookup_struct(node->ch[0], tp);
    }else if(node->type == IfStmt){

        expect_ret_type(node->ch[0], PrimType::get_bool());
        auto ret1 = build_sym_table(node->ch[1]);
        var_type_ptr ret2 = VoidType::get();
        if(node->ch.size() > 2)
            ret2 = build_sym_table(node->ch[2]);

        auto comm = greater_type(ret1, ret2);
        if(!comm->is_void()){
            expect_ret_type(node->ch[0], comm);
            expect_ret_type(node->ch[1], comm);
        }

        return node->ret_var_type = comm;

    }else if(node->type == WhileStmt){

        expect_ret_type(node->ch[0], PrimType::get_bool());
        build_sym_table(node->ch[1]);
        return node->ret_var_type = VoidType::get();

    }else if(node->type == ForStmt){

        build_sym_table(node->ch[0]);
        if(node->ch[1]->type != Stmt){
            expect_ret_type(node->ch[1], PrimType::get_bool());
        }
        build_sym_table(node->ch[2]);
        build_sym_table(node->ch[3]);
        return node->ret_var_type = VoidType::get();

    }else if(node->is_literal()){
        switch (node->type){
            case IntLiteral: return node->ret_var_type = PrimType::get_int(32);
            case DoubleLiteral: return node->ret_var_type = PrimType::get_float(64);
            case BoolLiteral: return node->ret_var_type = PrimType::get_bool();
            case CharLiteral: return node->ret_var_type = PrimType::get_char();
            case PointerLiteral:{
                return node->ret_var_type = PointerType::get(VoidType::get());
            }
            case StringLiteral:{
                auto len = parse_string(node->str).size() + 1;
                auto res = ArrayType::get(PrimType::get_char(), len);
                auto ret = node->ret_var_type = RefType::get(res, true);
                return ret;
            }
            default: return node->ret_var_type = ErrorType::get();
        }

    }else if(node->type == ExprList){
        var_type_ptr res;
        for(auto ch: node->ch)
            res = build_sym_table(ch);
        return node->ret_var_type = expect_ret_type(node->ch.back(), res->decay());
    }else if(node->type == Stmt){
        if(node->str == "return"){
            auto p = node->get_func_parent();
            if(!p){
                // append_error("return statement out of function body.", node->loc);
                append_misplace_error("return", "function", node->loc);
                return node->ret_var_type =VoidType::get();
            }
            auto ret_type = dyn_ptr_cast<FuncType>(p->ret_var_type)->ret_type;
            var_type_ptr res = VoidType::get();
            if(node->ch.size()){
                expect_ret_type(node->ch[0], ret_type);
            }
            return node->ret_var_type = VoidType::get();
        }else if(node->str == "break" || node->str == "continue"){
            auto p = node->get_loop_parent();
            if(!p){
                // append_error(node->str + " statement out of loop statement.", node->loc);
                append_misplace_error(node->str, "loop", node->loc);
            }
            return node->ret_var_type = VoidType::get();
        }else{
            //noop
            return node->ret_var_type = VoidType::get();
        }
    }else if(node->type == FuncArgs){
        std::vector<var_type_ptr> mem;
        for(auto ch: node->ch){
            mem.push_back(build_sym_table(ch));
        }
        auto args = TupleType::get(mem);
        return node->ret_var_type = args;
    }
    else if (node->type == StructImpl) {
        auto id_type = ast_to_type(node->ch[0]);
        auto id = id_type->to_string();
        if(id_type->is_error()){
            append_nodef_error("Variable", node->ch[0]->str, node->ch[0]->loc);
            return ErrorType::get();
        }
        for (auto &ch : node->ch[1]->ch) {
            auto func = Adaptor<FuncDecl>(ch);
            check_prim_shadow(func.id, func.id_loc);
            auto mem_func_type = MemFuncType::get(id_type, func.type_info);

            auto flag = node->set_id(id + "$" + func.id, mem_func_type);
            ch->ret_var_type = mem_func_type;

            build_funcbody(ch, {std::make_pair("self", RefType::get(id_type))});

            if(!flag){
                append_multidef_error("Member Function", id + "." + func.id, func.id_loc);
                node->ret_var_type = ErrorType::get();
            }
        }
        return node->ret_var_type = VoidType::get();
    }else if(node->type == Err){
        return node->ret_var_type = ErrorType::get();
    }else{
        assert(false && "unreachable line: build_sym_table fall through");
        for(auto ch: node->ch)
            build_sym_table(ch);
        return node->ret_var_type = VoidType::get();
    }

}