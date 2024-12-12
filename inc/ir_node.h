#pragma once
#include "ast.h"
#include <string>
#include <stack>
#include "util.h"

using util::str;

bool type_support_check(var_type_ptr tp);
bool math_op_support_check(op_type op);
bool func_support_check(std::shared_ptr<FuncType> fn);

struct IRContext{
    bool req_read = 0;
    bool req_write = 0;
    bool req_memcpy = 0;

    // std::stack<sym_table> sym_stack;
    std::vector<std::string> code;

    std::stack<std::string> loop_label;

    std::map<std::string, int> label_counter;
    
    std::string get_new_id(std::string pre){
        return pre + std::to_string(label_counter[pre]++);
    }

    template<class ...Args>
    void write_code(Args ...args){
        auto s = str(std::forward<Args>(args)...);
        // std::cout << s << std::endl;
        code.push_back(s);
    }

    void write_br(std::string cond, std::string label){
        write_code("IF ", cond, " GOTO ", label);
    }

    void write_label(std::string label){
        write_code("LABEL ", label, " :");
    }

    void export_ir_code(FILE* file);
    void call_memcpy(std::string dst, std::string src, size_t siz);

    void push_stack(){}
    void pop_stack(){}

    void push_loop(const std::string& str){
        loop_label.push(str);
    }

    void pop_loop(){
        loop_label.pop();
    }
};

struct IRNode{
    enum node_type{
        Imm, Id,
        VarDecl, 
        Block, FuncBody, 
        If, Loop, 
        ValueOp, CondOp, RefOp,
        Call, Access, Ret,
        Noop
    };

    AstNode* origin;
    var_type_ptr ret_type;
    std::string dst, addr;
    node_type type;

    IRNode(node_type type, AstNode* origin): type(type), origin(origin){
        if(origin)
            ret_type = origin->ret_var_type;
    }

    virtual void gen_code(IRContext& ir_ctx) = 0;
};

struct IrNoopNode: IRNode{
    IrNoopNode(AstNode* ast): IRNode(Noop, ast){}

    void gen_code(IRContext& ctx){
        //noop
    }
};

struct IrValueNode: IRNode{
    IrValueNode(node_type type, AstNode* ast): IRNode(type, ast){}

    virtual std::string get_value(IRContext& ctx) = 0;

    void gen_code(IRContext& ir_ctx) override{
        dst = get_value(ir_ctx);
    }
};

struct IrBlockNode: IRNode{
    std::vector<IRNode*> body;

    IrBlockNode(AstNode* ast): IRNode(Block, ast){}
    IrBlockNode(node_type type, AstNode* ast): IRNode(type, ast){}

    virtual void block_pre(IRContext& ctx){

    }

    virtual void block_post(IRContext& ctx){

    }
    
    void gen_code(IRContext& ctx) override{
        ctx.push_stack();
        block_pre(ctx);
        for(auto ch: body)
            ch->gen_code(ctx);
        block_post(ctx);
        ctx.pop_stack();
        if(origin->type == StmtsRet){
            dst = body.back()->dst;
            addr = body.back()->addr;
        }
    }
};

struct IrImmNode: IrValueNode{
    int val;

    IrImmNode(AstNode* ast, int val = 0): IrValueNode(Imm, ast), val(val){
        ret_type = get_type("int");
    };
    IrImmNode(const std::string& str, AstNode* ast): IrValueNode(Imm, ast), val(util::str_to_int(str)){
        ret_type = get_type("int");
    }

    std::string get_value(IRContext& ctx) override{
        return str('#', val);
    }
};

struct IrIdNode: IrValueNode{
    var_info_ptr var_info;

    IrIdNode(var_info_ptr var, AstNode* ast): IrValueNode(Id, ast), var_info(var){}

    std::string get_value(IRContext& ctx) override{
        if(var_info->is_base()){
            addr = "&" + var_info->id_name();
            return var_info->id_name();
        }else{
            addr = var_info->ptr_name();
            return "*" + addr;
        }
    }
};

struct IrFuncNode: public IrBlockNode{
    var_info_ptr func_info;

    IrFuncNode(AstNode* ast, var_info_ptr info): IrBlockNode(FuncBody, ast), func_info(info){}

    void block_pre(IRContext& ctx) override {
        ctx.write_code("FUNCTION ", func_info->id_name(), " :");
        auto func = Adaptor<FuncDecl>(origin);
        auto body = func.body;
        auto param = func.param_name;

        if(!func_support_check(func.type_info)){
            throw std::string("Unsupport func :" + func.type_info->to_string());
        }
        
        for(auto &s: param){
            auto info = body->get_info(s);
            if(info->is_base()){
                ctx.write_code("PARAM ", info->id_name());
            }else{
                ctx.write_code("PARAM ", info->ptr_name());
            }
        }
    }

    void block_post(IRContext& ctx) override {
        auto func = Adaptor<FuncDecl>(origin);
        if(func.body->type == StmtsRet){
            ctx.write_code("RETURN ", body.back()->dst);
        }else{
            ctx.write_code("RETURN #0");
        }
        ctx.write_code(""); //add an empty line
    }
};

struct IrVarDeclNode: IRNode{
    var_info_ptr info;
    IRNode* init;

    IrVarDeclNode(var_info_ptr info, AstNode* ast, IRNode* init = nullptr): 
        IRNode(VarDecl, ast), info(info), init(init){}

    void gen_code(IRContext& ctx) override{
        if(!type_support_check(info->type)){
            throw std::string("Unsupport type");
        }
        
        if(init)
            init->gen_code(ctx);

        if(info->is_base()){
            ctx.write_code("DEC ", info->id_name(), " ", info->size());
        }else{
            ctx.write_code("DEC ", info->id_name(), " ", info->size());
            ctx.write_code(info->ptr_name(), " := &", info->id_name());
        }

        if(init){
            if(info->is_base()){
                ctx.write_code(info->id_name(), " := ", init->dst);
            }else{
                ctx.call_memcpy(info->ptr_name(), init->addr, info->size());
            }
        }
    }
};

struct IrCallNode: IRNode{
    std::vector<IRNode*> args;
    var_info_ptr func_info;

    IrCallNode(AstNode* ast, var_info_ptr info): IRNode(Call, ast), func_info(info){}

    void gen_code(IRContext& ctx) override{
        std::string name = func_info->id_name();
        if(func_info->is_top && func_info->name == "read"){
            ctx.req_read = 1;
            name = "__builtin_read";
        }
        if(func_info->is_top && func_info->name == "write"){
            ctx.req_write = 1;
            name = "__builtin_write";
        }
        for(auto ch: args){
            ch->gen_code(ctx);
            if(ch->ret_type->is_base())
                ctx.write_code("ARG ", ch->dst);
            else{
                auto v = ctx.get_new_id("v");
                ctx.write_code("DEC ", v, " ", ch->ret_type->size());
                ctx.call_memcpy("&" + v, ch->addr, ch->ret_type->size());
                ctx.write_code("ARG ", "&", v);
            }
        }
        if(ret_type->is_void()){
            ctx.write_code("CALL ", name);
        }else{
            auto ret = ctx.get_new_id("ret");
            if(ret_type->is_base()){
                ctx.write_code(ret, " := CALL ", name);
                addr = "&" + ret;
                dst = ret;
            }else{
                ctx.write_code(ret, " := CALL ", name);
                addr = ret;
                dst = "*" + addr;
            }
        }
    }
};

struct IrRetNode: IRNode{
    IRNode* ch;

    IrRetNode(AstNode* ast, IRNode *ch = nullptr): IRNode(Ret, ast), ch(ch){};

    void gen_code(IRContext& ctx) override{
        if(ch){
            ch->gen_code(ctx);
            ctx.write_code("RETURN ", ch->dst);
        }else{
            ctx.write_code("RETURN #0");
        }
    }
};

struct IrValueOpNode: IRNode{
    IRNode *lhs, *rhs;
    op_type op;
    IrValueOpNode(IRNode* lhs, IRNode* rhs, op_type op, AstNode* ast):
        IRNode(ValueOp, ast), lhs(lhs), rhs(rhs), op(op){}

    void gen_code(IRContext& ctx) override; 
};

struct IrRefOpNode: IRNode{
    IRNode *arg;
    op_type op;

    IrRefOpNode(IRNode *arg, op_type op, AstNode* ast):
        IRNode(RefOp, ast), arg(arg), op(op){}

    void gen_code(IRContext& ctx) override{
        arg->gen_code(ctx);

        if(op == op_type::Ref){//&a

            dst = ctx.get_new_id("v");
            ctx.write_code(dst, " := ", arg->addr);
            addr = "&" + dst;
            return;

        }else if(op == op_type::DeRef){//*a
            addr = ctx.get_new_id("v");
            ctx.write_code(addr, " := ", arg->dst);
            dst = "*" + addr;
            return;
        }
        throw std::string("unreachable line @IR_RefOpNode::gen_code");
    }
};

struct IrAccessNode: IRNode{
    IRNode *lhs, *rhs;
    size_t subtype_width;

    IrAccessNode(IRNode* lhs, IRNode* rhs, AstNode* ast):
        IRNode(ValueOp, ast), lhs(lhs), rhs(rhs){
            auto tp = decay(lhs->ret_type);
            if(tp->is_array()){
                auto arr = std::dynamic_pointer_cast<ArrayType>(tp);
                subtype_width = arr->subtype->size();
            }else if(tp->is_ptr()){
                auto ptr = std::dynamic_pointer_cast<PointerType>(tp);
                subtype_width = ptr->subtype->size();
            }
        }
    
    IrAccessNode(IRNode* lhs, size_t offset, AstNode* ast):
        IRNode(ValueOp, ast), lhs(lhs){
            rhs = new IrImmNode(nullptr, offset);
            rhs->ret_type = get_type("int");
            subtype_width = 1;
        }

    void gen_code(IRContext& ctx) override{
        lhs->gen_code(ctx);
        rhs->gen_code(ctx);
        addr = ctx.get_new_id("v");
        ctx.write_code(addr, " := ", rhs->dst, " * #", subtype_width);
        ctx.write_code(addr, " := ", lhs->addr, " + ", addr);
        dst = "*" + addr;
    }
};

struct IrCondOpNode: IRNode{
    IRNode *lhs, *rhs;
    op_type op;

    IrCondOpNode(IRNode* lhs, IRNode* rhs, AstNode* ast, op_type op): 
        IRNode(CondOp, ast), lhs(lhs), rhs(rhs), op(op){}

    std::string get_cond(IRContext& ctx){
        static std::string s[] = {"==", "!=", "<=", ">=", "<", ">"};
        lhs->gen_code(ctx);
        rhs->gen_code(ctx);
        return str(lhs->dst, " ", s[(int)op - (int)op_type::Eq], " ", rhs->dst);
    }

    std::string get_cond_inv(IRContext& ctx){
        static std::string s[] = {"!=", "==", ">", "<", ">=", "<="};
        lhs->gen_code(ctx);
        rhs->gen_code(ctx);
        return str(lhs->dst, " ", s[(int)op - (int)op_type::Eq], " ", rhs->dst);
    }    

    void gen_code(IRContext& ctx) override{
        lhs->gen_code(ctx);
        rhs->gen_code(ctx);
        dst = ctx.get_new_id("cmp_res");
        auto label = ctx.get_new_id("cmp_end");
        ctx.write_code(dst, " := #1");
        ctx.write_br(get_cond(ctx), label);
        ctx.write_code(dst, " := #0");
        ctx.write_label(label);
    }
};

struct IrIfNode: IRNode{
    IrCondOpNode *cond;
    IRNode *if_true, *if_false;

    IrIfNode(IrCondOpNode* cond, IRNode *if_true, IRNode *if_false, AstNode* ast):
        IRNode(If, ast), cond(cond), if_true(if_true), if_false(if_false){}
    
    void gen_code(IRContext& ctx) override{
        auto cmp = cond->get_cond(ctx);
        auto label_true = ctx.get_new_id("if_true");
        auto label_end = ctx.get_new_id("if_end");
        if(!ret_type->is_void()){
            dst = ctx.get_new_id("res");
            bool base = ret_type->is_base();
            ctx.write_br(cmp, label_true);
            if_false->gen_code(ctx);

            ctx.write_code(dst, " := ", base ? if_false->dst: if_false->addr);

            ctx.write_code("GOTO ", label_end);
            ctx.write_label(label_true);
            if_true->gen_code(ctx);
            ctx.write_code(dst, " := ", base? if_true->dst: if_true->addr);
            ctx.write_label(label_end);
        }else{
            ctx.write_br(cmp, label_true);
            if(if_false){
                if_false->gen_code(ctx);
            }
            ctx.write_code("GOTO ", label_end);
            ctx.write_label(label_true);
            if_true->gen_code(ctx);
            ctx.write_label(label_end);
        }
    }
};

struct IrLoopNode: IrBlockNode{
    IrCondOpNode* cond;
    IRNode* before_loop;
    IRNode* after_loop;
    std::string label_begin, label_end;

    IrLoopNode(AstNode* ast): IrBlockNode(Loop, ast){}

    void block_pre(IRContext& ctx) override{
        // std::cout << "@@@1" << std::endl;
        if(before_loop)
            before_loop->gen_code(ctx);
        // std::cout << "@@@2" << std::endl;
        label_begin = ctx.get_new_id("loop_begin");
        label_end = ctx.get_new_id("loop_end");
        ctx.write_label(label_begin);
        auto cmp = cond->get_cond_inv(ctx);
        ctx.write_br(cmp, label_end);
        ctx.push_loop(label_end);
    }

    void block_post(IRContext& ctx) override{
        if(after_loop)
            after_loop->gen_code(ctx);
        ctx.write_code("GOTO ", label_begin);
        ctx.write_label(label_end);
        ctx.pop_loop();
    }    
};

IRNode* ast_to_spl_ir(AstNode* ast);