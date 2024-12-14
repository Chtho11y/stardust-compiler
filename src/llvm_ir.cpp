#include "llvm_ir.h"
#include <stack>
#include <iostream>

std::unique_ptr<LLVMContext> llvm_ctx;
std::unique_ptr<Module> llvm_mod;
std::unique_ptr<IRBuilder<>> builder;
llvm::Function* init_fn = nullptr, *main_fn = nullptr;
std::string module_name;

bool ir_is_generated = false;

void init(std::string name){
    module_name = name;
    llvm_ctx = std::make_unique<LLVMContext>();
    llvm_mod = std::make_unique<Module>(module_name + ".ll", *llvm_ctx);
    builder = std::make_unique<IRBuilder<>>(*llvm_ctx);
}

void gen_module(AstNode* ast, std::string module_name){
    init(module_name);
    gen_llvm_ir(ast, *builder);
    if(!main_fn)
        throw std::runtime_error("function 'main' not found");
    if(init_fn){
        IRBuilder<> main_st(&main_fn->getEntryBlock(), main_fn->getEntryBlock().begin());
        main_st.CreateCall(init_fn);
    }
    ir_is_generated = true;
}

using llvm_val_info = std::pair<llvm::Type*, llvm::AllocaInst*>;
// std::stack<std::map<std::string, llvm_val_info>> var_stack;
std::map<size_t, llvm_val_info> var_table;
std::map<size_t, llvm::GlobalVariable*> global_var_table;
std::map<size_t, llvm::StructType*> struct_table;

void gen_llvm_struct(std::shared_ptr<StructType> st);
llvm::Value* gen_convert(llvm::Value* from_v, var_type_ptr from, var_type_ptr to, IRBuilder<>& builder);
llvm::Value* gen_llvm_ir_to_type(AstNode* ast, var_type_ptr from, var_type_ptr to, IRBuilder<>& builder);

llvm::AllocaInst* 
get_alloc_inst(llvm::Function *func, llvm::Type *type, const std::string& name, 
                llvm::Value *arr_len = nullptr){

    return IRBuilder<>(&func->getEntryBlock(), func->getEntryBlock().begin())
                        .CreateAlloca(type, arr_len, name);
}

llvm::Value* get_var_inst(size_t id){
    if(var_table.count(id))
        return var_table[id].second;
    if(global_var_table.count(id))
        return global_var_table[id];
    throw std::invalid_argument("unknown variable id " + std::to_string(id));
}

namespace type_aux{

llvm::Type* get_llvm_prim(std::shared_ptr<PrimType> type){
    using llvm::Type;

    switch (type->pr_kind){

    case PrimType::Int :{
        switch (type->siz){
        case 8: return Type::getInt8Ty(*llvm_ctx);
        case 16:return Type::getInt16Ty(*llvm_ctx);
        case 32:return Type::getInt32Ty(*llvm_ctx);
        case 64:return Type::getInt64Ty(*llvm_ctx);
        }
        break;
    }
        
    case PrimType::Char : return Type::getInt8Ty(*llvm_ctx);
    case PrimType::Bool : return Type::getInt1Ty(*llvm_ctx);
    
    case PrimType::Float:{
        if(type->siz == 32)
            return Type::getFloatTy(*llvm_ctx);
        else if(type->siz == 64)
            return Type::getDoubleTy(*llvm_ctx);
        break;
    }
    
    default:
        break;
    }
    throw std::invalid_argument("unsupport type: " + type->to_string());
}

llvm::ArrayType* get_llvm_array(std::shared_ptr<ArrayType> type){
    auto subtype = get_llvm_type(type->subtype);
    return llvm::ArrayType::get(subtype, type->len);
}

llvm::StructType* get_llvm_struct(std::shared_ptr<StructType> type){
    if(!struct_table.count(type->id))
        gen_llvm_struct(type);
    return struct_table[type->id];
}

llvm::PointerType* get_llvm_pointer(std::shared_ptr<PointerType> type){
    auto subtype = get_llvm_type(type->subtype);
    return llvm::PointerType::get(subtype, 0);
}

llvm::FunctionType* get_llvm_func(std::shared_ptr<FuncType> type){
    std::vector<llvm::Type*> args;

    auto ret = get_llvm_type(type->ret_type);

    for(auto tp: type->param_list)
        args.push_back(get_llvm_type(tp));

    return llvm::FunctionType::get(ret, args, false);
}

};// type_aux

llvm::Type* get_llvm_type(var_type_ptr type){
    using namespace type_aux;

    type = decay(type);
    switch(type->kind){

    case VarType::Prim: 
        return get_llvm_prim(std::dynamic_pointer_cast<PrimType>(type));

    case VarType::Void:
        return llvm::Type::getVoidTy(*llvm_ctx);

    case VarType::Func:
        return get_llvm_func(std::dynamic_pointer_cast<FuncType>(type));

    case VarType::Pointer:
        return get_llvm_pointer(std::dynamic_pointer_cast<PointerType>(type));

    case VarType::Array:
        return get_llvm_array(std::dynamic_pointer_cast<ArrayType>(type));

    case VarType::Struct:
        return get_llvm_struct(std::dynamic_pointer_cast<StructType>(type));
    }
    throw std::invalid_argument("unsupport type: " + type->to_string());
}

llvm_val_info add_var(var_info_ptr var, IRBuilder<>& builder){

    auto llvm_type = get_llvm_type(var->type);
    auto fn = builder.GetInsertBlock()->getParent();

    auto alloca = get_alloc_inst(fn, llvm_type, var->name);

    var_table[var->var_id] = {llvm_type, alloca};
    return {llvm_type, alloca};
}

void gen_llvm_struct(std::shared_ptr<StructType> st){
    std::vector<llvm::Type*> members;

    for (auto [name, tp] : st->member) {
        members.push_back(get_llvm_type(tp));
    }

    struct_table[st->id] = llvm::StructType::create(*llvm_ctx, members, st->name + "." + std::to_string(st->id));
}

void gen_default_ret(llvm::Type* type, IRBuilder<>& builder){
    if(type->isVoidTy())
        builder.CreateRetVoid();
    else 
        builder.CreateRet(llvm::Constant::getNullValue(type));
}

llvm::Function* gen_ext_func_def(AstNode* ast, IRBuilder<>& builder){
    using namespace llvm;

    auto func = Adaptor<FuncDecl>(ast);

    auto info = ast->get_info(func.id);
    if(!info->is_top)
        throw std::invalid_argument("inner function is not support.");
    
    auto llvm_fn_tp = (FunctionType*)get_llvm_type(func.type_info);
    auto llvm_fn = Function::Create(llvm_fn_tp, Function::ExternalLinkage, func.id, *llvm_mod);

    if(func.id == "main")
        main_fn = llvm_fn;

    auto bb = llvm::BasicBlock::Create(*llvm_ctx, func.id, llvm_fn);
    builder.SetInsertPoint(bb);

    int idx = 0;
    for (auto &arg : llvm_fn->args()) {
        auto id = func.body->get_info(func.param_name[idx++]);
        llvm::AllocaInst *alloca = get_alloc_inst(llvm_fn, arg.getType(), id->name);
        builder.CreateStore(&arg, alloca);
        var_table[id->var_id] = { arg.getType(), alloca};
    }

    auto ret = gen_llvm_ir(func.body, builder);

    if(func.body->type == StmtsRet){
        if(func.no_return())
            builder.CreateRetVoid();
        else builder.CreateRet(ret);
    }else if (builder.GetInsertBlock()->getTerminator() == nullptr) {
        llvm::Type *ret_type = llvm_fn->getReturnType();
        gen_default_ret(ret_type, builder);
    }

    return llvm_fn;
}

void gen_ext_var_decl(AstNode* ast, IRBuilder<>& builder){
    using namespace llvm;

    auto decl = Adaptor<VarDecl>(ast);
    auto info = ast->get_info(decl.id);
    auto var_tp = get_llvm_type(info->type);
    auto var = new llvm::GlobalVariable(
        *llvm_mod, var_tp, false,
        llvm::GlobalValue::ExternalLinkage,
        llvm::UndefValue::get(var_tp),
        info->name                        
    );

    global_var_table[info->var_id] = var;

    if(decl.init_val){

        auto init_val_fn_tp = llvm::FunctionType::get(var_tp, false);
        auto init_val_fn = Function::Create(
            init_val_fn_tp,
            GlobalValue::InternalLinkage,
            decl.id + ".init",
            *llvm_mod
        );
        BasicBlock *bb = BasicBlock::Create(*llvm_ctx, "entry", init_val_fn);
        IRBuilder<> init_builder(bb);

        Value *init_ret = gen_llvm_ir_to_type(decl.init_val, decl.init_val->ret_var_type, decay(decl.type_info), init_builder);
        if (!init_ret) {
            throw std::runtime_error("Failed to generate IR for init_expr.");
        }

        init_builder.CreateRet(init_ret);

        if(!init_fn){
            auto void_call = llvm::FunctionType::get(llvm::Type::getVoidTy(*llvm_ctx), false);
            init_fn = llvm::Function::Create(void_call, llvm::Function::ExternalLinkage, "global.var.init", *llvm_mod);
            auto bb = llvm::BasicBlock::Create(*llvm_ctx, "entry", init_fn);
            IRBuilder<>(bb).CreateRetVoid();
        }

        IRBuilder<> init_fn_builder(&init_fn->getEntryBlock(), init_fn->getEntryBlock().begin());

        init_ret = init_fn_builder.CreateCall(init_val_fn);
        init_fn_builder.CreateStore(init_ret, var);
    }
}

void gen_var_decl(AstNode* ast, IRBuilder<>& builder){
    auto var_node = Adaptor<VarDecl>(ast);
    auto var_info = ast->get_info(var_node.id);

    auto [var_tp, alloca] = add_var(var_info, builder);

    if(var_node.init_val){
        auto val = 
            gen_llvm_ir_to_type(var_node.init_val, var_node.init_val->ret_var_type, decay(var_info->type), builder);
        builder.CreateStore(val, alloca);
    }
}

void gen_ext_decl_list(AstNode* ast, IRBuilder<>& builder){
    for(auto ch: ast->ch){
        if(ch->type == FuncDecl){
            gen_ext_func_def(ch, builder);
        }else if(ch->type == VarDecl){
            gen_ext_var_decl(ch, builder);
        }else{
            assert(ch->type == StructDecl);
            //skip StructDecl
        }
    }
}

namespace gen_op{

// enum class op_type{
//     Add, Sub, Mul, Div, Mod, And, Or,
//     BitAnd, BitOr, Xor, Eq, Neq, Le, Ge, Lt, Gt,
//     Assign, AddEq, SubEq, MulEq, DivEq, At, Call, Comma, Access,
//     Pos, Neg, Not, Convert,
//     Ref, DeRef
// };

bool is_math_op(op_type type){
    return op_type::Add <= type && type <= op_type::Xor;
}

bool is_logic_op(op_type type){
    return op_type::And == type || type == op_type::Or;
}

bool is_uary_op(op_type type){
    return op_type::Pos <= type && type <= op_type::Not;
}

bool is_cmp_op(op_type type){
    return op_type::Eq <= type && type <= op_type::Gt;
}

bool is_assign_op(op_type type){
    return op_type::Assign <= type && type <= op_type::DivEq;
}

llvm::Value* gen_unary_op(OperatorNode* ast, IRBuilder<>& builder){
    auto val = gen_llvm_ir_to_type(ast->ch[0],ast->ch[0]->ret_var_type, ast->ret_var_type, builder);

    switch(ast->type){
    case op_type::Pos: return val;
    case op_type::Neg: return builder.CreateNeg(val);
    case op_type::Not: return builder.CreateNot(val);
    }

    assert(false && "unreachable");
    return nullptr;
}

llvm::Value* gen_math_op(OperatorNode *ast, IRBuilder<>& builder){
    
    auto ast_tp = ast->ret_var_type;
    auto res_tp = get_llvm_type(ast_tp);
    op_type op = ast->type;

    auto lhs = gen_llvm_ir_to_type(ast->ch[0], ast->ch[0]->ret_var_type, ast_tp, builder);
    auto rhs = gen_llvm_ir_to_type(ast->ch[1], ast->ch[1]->ret_var_type, ast_tp, builder);

    switch(op){
    case op_type::Add:{
        if(res_tp->isIntegerTy())
            return builder.CreateAdd(lhs, rhs);
        else if(res_tp->isFloatingPointTy())
            return builder.CreateFAdd(lhs, rhs);
        break;
    }

    case op_type::Sub:{
        if(res_tp->isIntegerTy())
            return builder.CreateSub(lhs, rhs);
        else if(res_tp->isFloatingPointTy())
            return builder.CreateFSub(lhs, rhs);
        break;
    }

    case op_type::Mul:{
        if(res_tp->isIntegerTy())
            return builder.CreateMul(lhs, rhs);
        else if(res_tp->isFloatingPointTy())
            return builder.CreateFMul(lhs, rhs);
        break;
    }

    case op_type::Div:{
        if(res_tp->isIntegerTy())
            return ast_tp->is_signed() ? builder.CreateSDiv(lhs, rhs): builder.CreateUDiv(lhs, rhs);
        else if(res_tp->isFloatingPointTy())
            return builder.CreateFDiv(lhs, rhs);
        break;
    }

    case op_type::Mod: {
        if (res_tp->isIntegerTy()) 
            return ast_tp->is_signed() ? builder.CreateSRem(lhs, rhs): builder.CreateURem(lhs, rhs);
        break;
    }

    case op_type::BitAnd:{
        if (res_tp->isIntegerTy()) 
            return builder.CreateAnd(lhs, rhs);
        break;
    }

    case op_type::BitOr:{
        if (res_tp->isIntegerTy()) 
            return builder.CreateOr(lhs, rhs);
        break;
    }

    case op_type::Xor:{
        if (res_tp->isIntegerTy()) 
            return builder.CreateXor(lhs, rhs);
        break;
    }

    case op_type::And:{
        //TODO: short
        return builder.CreateSelect(lhs, rhs, llvm::ConstantInt::getNullValue(rhs->getType()));
    }

    case op_type::Or:{
        return builder.CreateSelect(lhs, llvm::ConstantInt::getNullValue(rhs->getType()), rhs);
    }
    }
    throw std::invalid_argument("unsupport op " + ast->op_name() + " with type " + ast->ret_var_type->to_string());
}

llvm::Value* gen_cmp_op(OperatorNode *ast, IRBuilder<>& builder){
    auto comm_tp = greater_type(ast->ch[0]->ret_var_type, ast->ch[1]->ret_var_type);
    auto llvm_comm_tp = get_llvm_type(comm_tp);
    auto lhs = gen_llvm_ir_to_type(ast->ch[0], ast->ch[0]->ret_var_type, comm_tp, builder);
    auto rhs = gen_llvm_ir_to_type(ast->ch[1], ast->ch[1]->ret_var_type, comm_tp, builder);
    
    switch (ast->type)
    {
    case op_type::Eq :{
        if(llvm_comm_tp->isIntegerTy() || llvm_comm_tp->isPointerTy())
            return builder.CreateICmpEQ(lhs, rhs);
        else if(llvm_comm_tp->isFloatingPointTy())
            return builder.CreateFCmpOEQ(lhs, rhs);
    }
    case op_type::Neq :{
        if(llvm_comm_tp->isIntegerTy() || llvm_comm_tp->isPointerTy())
            return builder.CreateICmpNE(lhs, rhs);
        else if(llvm_comm_tp->isFloatingPointTy())
            return builder.CreateFCmpONE(lhs, rhs);
    }
    case op_type::Ge :{
        if(llvm_comm_tp->isIntegerTy() || llvm_comm_tp->isPointerTy())
            return comm_tp->is_signed() ? builder.CreateICmpSGE(lhs, rhs) : builder.CreateICmpUGE(lhs, rhs);
        else if(llvm_comm_tp->isFloatingPointTy())
            return builder.CreateFCmpOGE(lhs, rhs);
    }
    case op_type::Le :{
        if(llvm_comm_tp->isIntegerTy() || llvm_comm_tp->isPointerTy())
            return comm_tp->is_signed() ? builder.CreateICmpSLE(lhs, rhs) : builder.CreateICmpULE(lhs, rhs);
        else if(llvm_comm_tp->isFloatingPointTy())
            return builder.CreateFCmpOLE(lhs, rhs);
    }
    case op_type::Gt :{
        if(llvm_comm_tp->isIntegerTy() || llvm_comm_tp->isPointerTy())
            return comm_tp->is_signed() ? builder.CreateICmpSGT(lhs, rhs) : builder.CreateICmpUGT(lhs, rhs);
        else if(llvm_comm_tp->isFloatingPointTy())
            return builder.CreateFCmpOGT(lhs, rhs);
    }
    case op_type::Lt :{
        if(llvm_comm_tp->isIntegerTy() )
            return comm_tp->is_signed() ? builder.CreateICmpSLT(lhs, rhs) : builder.CreateICmpULT(lhs, rhs);
        else if(llvm_comm_tp->isFloatingPointTy())
            return builder.CreateFCmpOLT(lhs, rhs);
    }
    default:
        break;
    }

    assert(false && "reachable");
}

llvm::Value* gen_assign_op(OperatorNode* ast, IRBuilder<>& builder){
    auto ast_tp = ast->ret_var_type;
    auto res_tp = get_llvm_type(ast_tp);
    op_type op = ast->type;

    auto lvalue = gen_llvm_ir(ast->ch[0], builder);
    auto rhs = gen_llvm_ir_to_type(ast->ch[1], ast->ch[1]->ret_var_type, decay(ast_tp), builder);

    auto res = rhs;
    if(op != op_type::Assign){
        auto lhs = builder.CreateLoad(lvalue);
        switch(op){
        case op_type::AddEq:{
            if(res_tp->isIntegerTy())
                res = builder.CreateAdd(lhs, rhs);
            else if(res_tp->isFloatingPointTy())
                res = builder.CreateFAdd(lhs, rhs);
            break;
        }

        case op_type::Sub:{
            if(res_tp->isIntegerTy())
                res = builder.CreateSub(lhs, rhs);
            else if(res_tp->isFloatingPointTy())
                res = builder.CreateFSub(lhs, rhs);
            break;
        }

        case op_type::Mul:{
            if(res_tp->isIntegerTy())
                res = builder.CreateMul(lhs, rhs);
            else if(res_tp->isFloatingPointTy())
                res = builder.CreateFMul(lhs, rhs);
            break;
        }

        case op_type::Div:{
            if(res_tp->isIntegerTy())
                res = ast_tp->is_signed() ? builder.CreateSDiv(lhs, rhs): builder.CreateUDiv(lhs, rhs);
            else if(res_tp->isFloatingPointTy())
                res = builder.CreateFDiv(lhs, rhs);
            break;
        }

        }
    }
    builder.CreateStore(res, lvalue);
    return lvalue;
}

llvm::Value* gen_call_op(OperatorNode *ast, IRBuilder<>& builder){
    // TODO: support function pointer
    if(ast->ch[0]->type != Identifier)
        throw std::invalid_argument("function pointer is not support yet");

    auto func_info = ast->get_info(ast->ch[0]->str);
    auto ast_fn_tp = std::dynamic_pointer_cast<FuncType>(func_info->type);
    auto ast_args = ast->ch[1];
    auto params_tp = ast_fn_tp->param_list;

    std::vector<llvm::Value*> args;

    for(size_t i = 0; i < ast_args->ch.size(); ++i){
        args.push_back(gen_llvm_ir_to_type(ast_args->ch[i], ast_args->ch[i]->ret_var_type, params_tp[i], builder));
    }

    llvm::Function *function = builder.GetInsertBlock()->getModule()->getFunction(func_info->name);
    return builder.CreateCall(function, args, func_info->name + "_ret");
}

llvm::Value* gen_access_op(OperatorNode *ast, IRBuilder<>& builder){
    if(ast->type == op_type::At){
        auto arr = gen_llvm_ir(ast->ch[0], builder);
        auto idx = gen_llvm_ir_to_type(ast->ch[1], ast->ch[1]->ret_var_type, get_type("uint64"), builder);
        auto zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*llvm_ctx), 0);
        return builder.CreateGEP(arr, {zero, idx});

    }else if(ast->type == op_type::Access){
        
    }

    assert(false && "unreachable");
}

};

llvm::Value* gen_operator(OperatorNode* ast, IRBuilder<>& builder){
    using namespace gen_op;

    auto op = ast->type;

    if(is_math_op(op))
        return gen_math_op(ast, builder);
    
    if(is_uary_op(op))
        return gen_unary_op(ast, builder);
    
    if(is_cmp_op(op))
        return gen_cmp_op(ast, builder);

    if(is_assign_op(op))
        return gen_assign_op(ast, builder);
    
    if(op == op_type::Convert){
        return gen_llvm_ir_to_type(ast->ch[0], ast->ch[0]->ret_var_type, ast->ret_var_type, builder);
    }

    if(op == op_type::Call)
        return gen_call_op(ast, builder);
    
    if(op == op_type::Access || op == op_type::At)
        return gen_access_op(ast, builder);
    
    throw std::invalid_argument("unsupport op: " + get_op_name(op));
}

llvm::Value* gen_literal(AstNode* ast, IRBuilder<>& builder){
    using namespace llvm;
    switch (ast->type)
    {
    case IntLiteral:
        return ConstantInt::get(Type::getInt32Ty(*llvm_ctx), atoi(ast->str.c_str()));
    
    case DoubleLiteral:
        return ConstantFP::get(Type::getFloatTy(*llvm_ctx), atof(ast->str.c_str()));
    
    case BoolLiteral:
        return ConstantInt::get(Type::getInt1Ty(*llvm_ctx), ast->str == "true");
    
    default:
        throw std::invalid_argument("unsupported literal");
        break;
    }
}

llvm::Value* gen_for_stmt(AstNode* ast, IRBuilder<>& builder){
    return nullptr;
}

llvm::Value* gen_while_stmt(AstNode* ast, IRBuilder<>& builder){
    using llvm::BasicBlock;
    auto fn = builder.GetInsertBlock()->getParent();
    
    BasicBlock *b_begin = BasicBlock::Create(*llvm_ctx, "while.begin", fn);
    BasicBlock *b_cond = BasicBlock::Create(*llvm_ctx, "while.cond");
    BasicBlock *b_end = BasicBlock::Create(*llvm_ctx, "while.end");

    builder.CreateBr(b_cond);

    builder.SetInsertPoint(b_begin);

    gen_llvm_ir(ast->ch[1], builder);
    builder.CreateBr(b_cond);

    builder.SetInsertPoint(b_cond);
    fn->getBasicBlockList().push_back(b_cond);
    auto cond = gen_llvm_ir(ast->ch[0], builder);

    builder.CreateCondBr(cond, b_begin, b_end);

    fn->getBasicBlockList().push_back(b_end);
    builder.SetInsertPoint(b_end);

    return nullptr;
}

llvm::Value* gen_if_ret_stmt(AstNode* ast, IRBuilder<>& builder){
    using llvm::BasicBlock;
    auto cond = gen_llvm_ir(ast->ch[0], builder);

    auto fn = builder.GetInsertBlock()->getParent();
    BasicBlock *b_then = BasicBlock::Create(*llvm_ctx, "if.then", fn);
    BasicBlock *b_else = BasicBlock::Create(*llvm_ctx, "if.else");
    BasicBlock *b_end = BasicBlock::Create(*llvm_ctx, "if.end");

    bool no_ret = ast->ret_var_type->is_void();

    builder.CreateCondBr(cond, b_then, b_else);

    builder.SetInsertPoint(b_then);
    auto br_then = gen_llvm_ir(ast->ch[1], builder);
    if(!no_ret)
        br_then = gen_convert(br_then, ast->ch[1]->ret_var_type, ast->ret_var_type, builder);
    builder.CreateBr(b_else);
    BasicBlock *b_then_end = builder.GetInsertBlock();

    fn->getBasicBlockList().push_back(b_else);
    builder.SetInsertPoint(b_else);
    auto br_else = gen_llvm_ir(ast->ch[2], builder);
    if(!no_ret)
        br_else = gen_convert(br_else, ast->ch[2]->ret_var_type, ast->ret_var_type, builder);

    builder.CreateBr(b_end);
    BasicBlock *b_else_end = builder.GetInsertBlock();

    fn->getBasicBlockList().push_back(b_end);
    builder.SetInsertPoint(b_end);
    
    if(no_ret)
        return nullptr;
    
    llvm::PHINode *phi = builder.CreatePHI(get_llvm_type(ast->ret_var_type), 2, "if.res");
    phi->addIncoming(br_then, b_then_end);
    phi->addIncoming(br_else, b_else_end);

    return phi;
}

llvm::Value* gen_if_stmt(AstNode* ast, IRBuilder<>& builder){
    using llvm::BasicBlock;

    auto cond = gen_llvm_ir(ast->ch[0], builder);

    auto fn = builder.GetInsertBlock()->getParent();
    BasicBlock *b_then = BasicBlock::Create(*llvm_ctx, "if.then", fn);
    BasicBlock *b_end = BasicBlock::Create(*llvm_ctx, "if.end");

    builder.CreateCondBr(cond, b_then, b_end);
    builder.SetInsertPoint(b_then);
    auto br_then = gen_llvm_ir(ast->ch[1], builder);
    builder.CreateBr(b_end);

    fn->getBasicBlockList().push_back(b_end);
    builder.SetInsertPoint(b_end);

    return br_then;
}

llvm::Value* gen_llvm_ir(AstNode* ast, IRBuilder<>& builder){
    // std::cout << get_node_name(ast) << std::endl;
    switch (ast->type)
    {
    case Program:{
        gen_ext_decl_list(ast->ch[0], builder);
        return nullptr;
    }

    case Stmts: case StmtsRet:{
        llvm::Value* ret = nullptr;
        for(auto ch: ast->ch){
            ret = gen_llvm_ir(ch, builder);
        }
        if(!ast->ret_var_type->is_void())
            return gen_convert(ret, ast->ch.back()->ret_var_type, ast->ret_var_type, builder);
        return ret;
    }

    case IfStmt:{
        if(ast->ch.size() == 2){
            return gen_if_stmt(ast, builder);
        }else{
            return gen_if_ret_stmt(ast, builder);
        }
    }

    case WhileStmt:{
        return gen_while_stmt(ast, builder);
    }

    case ForStmt:{
        return nullptr;
    }

    case VarDecl:{
        gen_var_decl(ast, builder);
        return nullptr;
    }

    case Operator:{
        return gen_operator((OperatorNode*)ast, builder);
    }

    case Stmt:{
        if(ast->str == "return"){
            if(ast->ch.size()){
                auto ast_fn = Adaptor<FuncDecl>(ast->get_func_parent());
                auto ret = gen_llvm_ir_to_type(ast->ch[0], ast->ch[0]->ret_var_type, 
                                                            ast_fn.type_info->ret_type, builder);
                return builder.CreateRet(ret);
            }
            return builder.CreateRetVoid();
        }else if(ast->str == "break"){

        }else if(ast->str == "continue"){

        }else if(ast->str == ""){
            return nullptr;
        }
        break;
    }

    case StructDecl:{
        //skip
        return nullptr;
    }

    case Identifier:{
        auto info = ast->get_info(ast->str);
        // if(info->type->is_array()){
        //     auto arr = std::dynamic_pointer_cast<ArrayType>(info->type);
        //     auto zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*llvm_ctx), 0);
        //     return builder.CreateGEP(get_var_inst(info->var_id), zero);
        // }else return get_var_inst(info->var_id);
        return get_var_inst(info->var_id);
    }

    case ExprList:{
        llvm::Value* ret = nullptr;
        for(auto ch: ast->ch)
            ret = gen_llvm_ir(ch, builder);
        if(!ast->ret_var_type->is_void())
            return gen_convert(ret, ast->ch.back()->ret_var_type, ast->ret_var_type, builder);
        return ret;
    }

    default:
        if(ast->is_literal()){
            return gen_literal(ast, builder);
        }
        break;
    }
    // throw std::invalid_argument("unsupported ast node");
    return nullptr;
}

llvm::Value* gen_convert(llvm::Value* from_v, var_type_ptr ast_from, var_type_ptr ast_to, IRBuilder<>& builder){
    // std::cout << "trying covert: " << ast_from->to_string() << " " 
    //             << ast_from->is_ref() << " to " << ast_to->to_string() << " " << ast_to->is_ref() << std::endl;
    auto from = get_llvm_type(ast_from);
    auto to = get_llvm_type(ast_to);
    auto from_signed = ast_from->is_signed();
    auto to_signed = ast_to->is_signed();

    if(ast_from->is_ref() && !ast_to->is_ref()){
        // std::cout << "deref" << std::endl;
        if(ast_to->is_array()){
            auto zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*llvm_ctx), 0);
            auto arr = builder.CreateGEP(from_v, zero);
            // return from_v;
            return builder.CreateLoad(arr);
        }else{
            auto res = builder.CreateLoad(from_v);
            return gen_convert(res, decay(ast_from), decay(ast_to), builder);
        }
    }

    if(ast_from->is_same(ast_to.get()))
        return from_v;

    if (from->isIntegerTy()) {
        if(to->isIntegerTy()){
            unsigned from_width = from->getIntegerBitWidth();
            unsigned to_width = to->getIntegerBitWidth();
            if (to_width == 1) {
                // Integer to bool conversion (int -> int1)
                return builder.CreateICmpNE(from_v, llvm::ConstantInt::get(from, 0), "tobool");
            }else if (from_width < to_width) {
                // Integer extension
                return from_signed ? builder.CreateSExt(from_v, to, "sext")
                                    : builder.CreateZExt(from_v, to, "zext");
            } else if (from_width > to_width) {
                // Integer truncation
                return builder.CreateTrunc(from_v, to, "trunc");
            }
        }else if(to->isFPOrFPVectorTy()){
            // Integer to floating-point conversion
            return from_signed ? builder.CreateSIToFP(from_v, to, "sitofp")
                         : builder.CreateUIToFP(from_v, to, "uitofp");
        }
    }

    if (from->isFloatingPointTy()) {
        if(to->isFloatingPointTy()){
            if (from->getPrimitiveSizeInBits() < to->getPrimitiveSizeInBits()) {
                // Floating-point extension
                return builder.CreateFPExt(from_v, to, "fpext");
            } else if (from->getPrimitiveSizeInBits() > to->getPrimitiveSizeInBits()) {
                // Floating-point truncation
                return builder.CreateFPTrunc(from_v, to, "fptrunc");
            }
        }else if(to->isIntegerTy()){
             return to_signed ? builder.CreateFPToSI(from_v, to, "fptosi")
                         : builder.CreateFPToUI(from_v, to, "fptoui");
        }
    }
    throw std::invalid_argument("unsupported type convert");
}

llvm::Value* gen_llvm_ir_to_type(AstNode* ast, var_type_ptr from, var_type_ptr to, IRBuilder<>& builder){
    auto res = gen_llvm_ir(ast, builder);
    return gen_convert(res, from, to, builder);
}

void write_llvm_ir(std::ostream& os){
    llvm::raw_os_ostream raw_os{os};
    llvm_mod->print(raw_os, nullptr);
}