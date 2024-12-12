#include "ir_node.h"

const std::string ir_builtin_memcpy = R"(
FUNCTION __builtin_memcpy :
PARAM dst
PARAM src
PARAM siz
_tmp_i := #0
LABEL memcpy_loop1 :
ptr_dst := dst + _tmp_i
ptr_src := src + _tmp_i
*ptr_dst := *ptr_src 
_tmp_i := _tmp_i + #4
IF _tmp_i < siz GOTO memcpy_loop1
RETURN #0
)";

const std::string ir_builtin_read = R"(
FUNCTION __builtin_read :
READ _tmp_read_v
RETURN _tmp_read_v
)";

const std::string ir_builtin_write = R"(
FUNCTION __builtin_write :
PARAM _tmp_print_v
WRITE _tmp_print_v
RETURN #0
)";

void IRContext::export_ir_code(FILE* file){
    if(!file)
        return;
    for(auto& s: code){
        fputs(s.c_str(), file);
        fputc('\n', file);
    }
    if(req_memcpy){
        fputs(ir_builtin_memcpy.c_str(), file);
    }
    if(req_read){
        fputs(ir_builtin_read.c_str(), file);
    }
    if(req_write){
        fputs(ir_builtin_write.c_str(), file);
    }
}

void IRContext::call_memcpy(std::string dst, std::string src, size_t siz){
    req_memcpy = 1;
    write_code("ARG ", "#", siz);
    write_code("ARG ", src);
    write_code("ARG ", dst);
    write_code("CALL ", "__builtin_memcpy");
}

bool math_op_support_check(op_type op){
    return op_type::Add <= op && op <= op_type::Div || op_type::AddEq <= op && op <= op_type::DivEq;
}

bool cmp_op_support_check(op_type op){
    return op_type::Eq <= op && op <= op_type::Gt;
}

bool ref_op_support_check(op_type op){
    return op_type::Ref == op || op_type::DeRef == op;
}

bool func_support_check(std::shared_ptr<FuncType> fn){
    for(auto ch: fn->param_list)
        if(!type_support_check(ch))
            return false;

    return fn->ret_type->is_void() || type_support_check(fn->ret_type);
}

bool type_support_check(var_type_ptr tp){
    tp = decay(tp);
    if(tp->is_type(VarType::Func)){
        return false;
    }else if(tp->is_prim()){
        auto sub = std::dynamic_pointer_cast<PrimType>(tp);
        return sub->pr_kind == PrimType::Int && sub->size() == 4;
    }else if(tp->is_array()){
        auto sub = std::dynamic_pointer_cast<ArrayType>(tp);
        return type_support_check(sub->subtype);
    }else if(tp->is_ptr()){
        auto sub = std::dynamic_pointer_cast<PointerType>(tp);
        return sub->is_void() || type_support_check(sub->subtype);        
    }else if(tp->is_type(VarType::Struct)){
        auto sub = std::dynamic_pointer_cast<StructType>(tp);
        bool flag = 1;
        for(auto [nam, tp]: sub->member)
            flag &= type_support_check(tp);
        return flag;
    }
    return false;
}

IrCondOpNode* cond_op_aux(IRNode* node){
    if(node->type == IRNode::CondOp){
        return dynamic_cast<IrCondOpNode*>(node);
    }else if(node->type == IRNode::Noop){
        return new IrCondOpNode(new IrImmNode(nullptr, 0), new IrImmNode(nullptr, 0), node->origin, op_type::Eq);
    }else{
        return new IrCondOpNode(node, new IrImmNode(nullptr, 0), node->origin, op_type::Neq);
    }
}

#define UNSUPPORT_ERR throw std::string("Unsupport ast node." + get_node_name(ast))

IRNode* ast_to_spl_ir(AstNode* ast){
    if(ast->type == Program){
        return ast_to_spl_ir(ast->ch[0]);
    }else if(ast->is_literal()){

        if(ast->type == IntLiteral){
            return new IrImmNode(ast->str, ast);
        }else if(ast->type == BoolLiteral){
            return new IrImmNode(ast, ast->str == "true");
        }else{
            UNSUPPORT_ERR;
        }

    }else if(ast->type == Identifier){
        auto info = ast->get_info(ast->str);
        return new IrIdNode(info, ast);
    }else if(ast->type == Operator){

        auto op = (OperatorNode*)ast;
        if(math_op_support_check(op->type) || op->type == op_type::Assign){
            auto l = ast_to_spl_ir(op->ch[0]);
            auto r = ast_to_spl_ir(op->ch[1]);
            return new IrValueOpNode(l, r, op->type, ast);
        }else if(cmp_op_support_check(op->type)){
            auto l = ast_to_spl_ir(op->ch[0]);
            auto r = ast_to_spl_ir(op->ch[1]);
            return new IrCondOpNode(l, r, ast, op->type);   
        }else if(op->type == op_type::Access){
            auto l = ast_to_spl_ir(op->ch[0]);
            auto id = op->ch[1]->str;
            if(decay(l->ret_type)->is_array()){
                auto arr = std::dynamic_pointer_cast<ArrayType>(decay(l->ret_type));
                return new IrImmNode(ast, arr->len);
            }
            auto st = std::dynamic_pointer_cast<StructType>(decay(op->ch[0]->ret_var_type));
            return new IrAccessNode(l, st->offset_of(id), ast);
        }else if(op->type == op_type::At){
            auto l = ast_to_spl_ir(op->ch[0]);
            auto r = ast_to_spl_ir(op->ch[1]);
            return new IrAccessNode(l, r, ast);            
        }else if(op->type == op_type::Call){
            if(op->ch[0]->type != Identifier){
                throw std::string("unsupport function call");
            }
            auto res = new IrCallNode(ast, op->get_info(op->ch[0]->str));
            for(auto ch: op->ch[1]->ch)
                res->args.push_back(ast_to_spl_ir(ch));
            return res;
        }else if(ref_op_support_check(op->type)){
            return new IrRefOpNode(ast_to_spl_ir(op->ch[0]), op->type, ast);
        }
        std::cout << "###" << get_op_name(op->type) << std::endl;
    }else if(ast->type == FuncDecl){

        auto func = Adaptor<FuncDecl>(ast);
        IrFuncNode* res = new IrFuncNode(ast, ast->get_info(func.id));
        res->body.push_back(ast_to_spl_ir(func.body));
        return res;

    }else if(ast->type == VarDecl){
        
        auto var = Adaptor<VarDecl>(ast);
        IRNode* ch = nullptr;
        if(var.init_val)
            ch = ast_to_spl_ir(var.init_val);
        return new IrVarDeclNode(ast->get_info(var.id), ast, ch);

    }else if(ast->type == Stmt){
        if(ast->str == "return"){
            IRNode* ch = nullptr;
            if(ast->ch.size())
                ch = ast_to_spl_ir(ast->ch[0]);
            return new IrRetNode(ast, ch);
        }else{
            return new IrNoopNode(ast);
        }
    }else if(ast->type == IfStmt){

        IRNode *cond = ast_to_spl_ir(ast->ch[0]);
        IRNode *br1 = ast_to_spl_ir(ast->ch[1]);
        IRNode *br2 = nullptr;
        if(ast->ch.size() >= 3)
            br2 = ast_to_spl_ir(ast->ch[2]);
        return new IrIfNode(cond_op_aux(cond), br1, br2, ast);
    }else if(ast->type == ForStmt){
        auto res = new IrLoopNode(ast);
        res->before_loop = ast_to_spl_ir(ast->ch[0]);
        res->after_loop = ast_to_spl_ir(ast->ch[2]);
        res->cond = cond_op_aux(ast_to_spl_ir(ast->ch[1]));
        res->body.push_back(ast_to_spl_ir(ast->ch[3]));
        return res;

    }else if(ast->type == WhileStmt){

        auto res = new IrLoopNode(ast);
        res->before_loop = res->after_loop = nullptr;
        res->cond = cond_op_aux(ast_to_spl_ir(ast->ch[0]));
        res->body.push_back(ast_to_spl_ir(ast->ch[1]));
        return res;

    }else if(ast->is_block){

        auto res = new IrBlockNode(ast);
        for(auto ch: ast->ch)
            res->body.push_back(ast_to_spl_ir(ch));
        return res;

    }else if(ast->type == StructDecl){
        return new IrNoopNode(ast);
    }
    UNSUPPORT_ERR;
}

void IrValueOpNode::gen_code(IRContext& ctx){
    lhs->gen_code(ctx);
    rhs->gen_code(ctx);

    if(op == op_type::Assign){
        if(origin->ret_var_type->is_base()){
            ctx.write_code(lhs->dst, " := ", rhs->dst);
        }else{
            ctx.call_memcpy(lhs->addr, rhs->addr, origin->ret_var_type->size());
        }
        dst = lhs->dst;
        addr = lhs->addr;
        return;
    }

    if(op_type::Add <= op && op <= op_type::Div){
        std::string op_syms[4] = {"+", "-", "*", "/"};
        dst = ctx.get_new_id("v");
        ctx.write_code(dst, " := ", lhs->dst, " ", op_syms[(int)op], " ", rhs->dst);
    }else if(op_type::AddEq <= op && op <= op_type::DivEq){
        std::string op_syms[4] = {"+", "-", "*", "/"};
        dst = ctx.get_new_id("v");
        ctx.write_code(lhs->dst, " := ", lhs->dst, " ", op_syms[(int)op - (int)op_type::AddEq], " ", rhs->dst);
    }else{
        throw std::string("unsupport op: " + get_op_name(op));
    }

    
}