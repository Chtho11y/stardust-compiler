#include "llvm_ir.h"
#include "literal_process.h"
#include <stack>
#include <iostream>

std::unique_ptr<LLVMContext> llvm_ctx;
std::unique_ptr<Module> llvm_mod;
std::unique_ptr<IRBuilder<>> builder;
llvm::Function* init_fn = nullptr, *main_fn = nullptr;
std::string module_name;

bool ir_is_generated = false;

using llvm_val_info = std::pair<llvm::Type*, llvm::AllocaInst*>;
// std::stack<std::map<std::string, llvm_val_info>> var_stack;
std::map<size_t, llvm_val_info> var_table;
std::map<size_t, llvm::GlobalVariable*> global_var_table;
std::map<size_t, llvm::StructType*> struct_table;
// std::map<size_t, llvm::Value*> func_ptr_table;

std::stack<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> loop_stack;

void gen_llvm_struct(std::shared_ptr<StructType> st);
llvm::Value* gen_convert(llvm::Value* from_v, var_type_ptr from, var_type_ptr to, IRBuilder<>& builder);
llvm::Value* gen_llvm_ir_to_type(AstNode* ast, var_type_ptr from, var_type_ptr to, IRBuilder<>& builder);
llvm::Value* gen_llvm_ir_decay(AstNode* ast, IRBuilder<>& builder);

struct loop_env_guard{
    loop_env_guard(llvm::BasicBlock *cond, llvm::BasicBlock *end){
        loop_stack.emplace(cond, end);
    }

    ~loop_env_guard(){
        loop_stack.pop();
    }
};

void init(std::string name){
    module_name = name;
    llvm_ctx = std::make_unique<LLVMContext>();
    llvm_mod = std::make_unique<Module>(module_name + ".ll", *llvm_ctx);
    builder = std::make_unique<IRBuilder<>>(*llvm_ctx);
}

void inject_builtin_func(){
    using namespace llvm;

    FunctionType *fn_write_tp = FunctionType::get(Type::getVoidTy(*llvm_ctx), {Type::getInt32Ty(*llvm_ctx)}, false);
    Function *fn_write = Function::Create(fn_write_tp, GlobalValue::ExternalLinkage, "write", *llvm_mod);
    Function *fn_exit = Function::Create(fn_write_tp, GlobalValue::ExternalLinkage, "exit", *llvm_mod);

    FunctionType *fn_read_tp = FunctionType::get(Type::getInt32Ty(*llvm_ctx), {}, false);
    Function *fn_read = Function::Create(fn_read_tp, GlobalValue::ExternalLinkage, "read", *llvm_mod);

    FunctionType *fn_puts_tp = FunctionType::get(Type::getInt32Ty(*llvm_ctx), {Type::getInt8PtrTy(*llvm_ctx)}, false);
    Function *fn_puts = Function::Create(fn_puts_tp, GlobalValue::ExternalLinkage, "puts", *llvm_mod);

    FunctionType *fn_putchar_tp = FunctionType::get(Type::getInt32Ty(*llvm_ctx), {Type::getInt32Ty(*llvm_ctx)}, false);
    Function *fn_putchar = Function::Create(fn_putchar_tp, GlobalValue::ExternalLinkage, "putchar", *llvm_mod);

    FunctionType *fn_getchar_tp = FunctionType::get(Type::getInt32Ty(*llvm_ctx), {}, false);
    Function *fn_getchar = Function::Create(fn_getchar_tp, GlobalValue::ExternalLinkage, "getchar", *llvm_mod);   

    FunctionType *fn_malloc_tp = FunctionType::get(Type::getInt8PtrTy(*llvm_ctx), {Type::getInt64Ty(*llvm_ctx)}, false);
    Function *fn_malloc = Function::Create(fn_malloc_tp, GlobalValue::ExternalLinkage, "malloc", *llvm_mod);

    FunctionType *fn_free_tp = FunctionType::get(Type::getInt32Ty(*llvm_ctx), {Type::getInt8PtrTy(*llvm_ctx)}, false);
    Function *fn_free = Function::Create(fn_free_tp, GlobalValue::ExternalLinkage, "free", *llvm_mod);

    FunctionType *fn_clock_tp = FunctionType::get(Type::getInt64Ty(*llvm_ctx), {}, false);
    Function *fn_clock = Function::Create(fn_clock_tp, GlobalValue::ExternalLinkage, "clock", *llvm_mod);

    FunctionType *fn_printf_tp = FunctionType::get(Type::getInt32Ty(*llvm_ctx), {Type::getInt8PtrTy(*llvm_ctx)}, true);
    Function *fn_printf = Function::Create(fn_printf_tp, GlobalValue::ExternalLinkage, "printf", *llvm_mod);    
    Function *fn_scanf = Function::Create(fn_printf_tp, GlobalValue::ExternalLinkage, "scanf", *llvm_mod);  
}

void set_module_target(){
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    std::string targetTriple = llvm::sys::getProcessTriple();

    std::string error;
    llvm::Target const *target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

    if (!target) {
        throw std::runtime_error("Error: Unable to find target: " + targetTriple);
    }

    llvm::TargetOptions targetOptions;
    
    llvm::TargetMachine *targetMachine = target->createTargetMachine(targetTriple, "generic", "", 
                                    targetOptions, llvm::Optional<llvm::Reloc::Model>());

    if (!targetMachine) {
        throw std::runtime_error("Error: Unable to create target machine");
    }

    llvm_mod->setDataLayout(targetMachine->createDataLayout());
    llvm_mod->setTargetTriple(targetTriple);
}

void gen_module(AstNode* ast, std::string module_name){
    if(ir_is_generated)
        return;
    init(module_name);
    inject_builtin_func();
    gen_llvm_ir(ast, *builder);
    if(!main_fn)
        throw std::runtime_error("function 'main' not found");
    if(init_fn){
        IRBuilder<> main_st(&main_fn->getEntryBlock(), main_fn->getEntryBlock().begin());
        main_st.CreateCall(init_fn);
    }
    set_module_target();
    ir_is_generated = true;
}

llvm::AllocaInst* 
get_alloc_inst(llvm::Function *func, llvm::Type *type, const std::string& name, 
                llvm::Value *arr_len = nullptr){

    return IRBuilder<>(&func->getEntryBlock(), func->getEntryBlock().begin())
                        .CreateAlloca(type, arr_len, name);
}

llvm::Value* get_var_inst(size_t id){
    if(var_table.count(id))
        return var_table[id].second;
    // if(func_ptr_table.count(id))
    //     return func_ptr_table[id];
    if(global_var_table.count(id))
        return global_var_table[id];
    // throw std::invalid_argument("unknown variable id " + std::to_string(id));
    return nullptr;
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
    if(subtype->isVoidTy())
        return llvm::PointerType::get(llvm::Type::getInt8Ty(*llvm_ctx), 0);
    return llvm::PointerType::get(subtype, 0);
    // return llvm::PointerType::get(llvm::Type::getInt8Ty(*llvm_ctx), 0);
}

llvm::FunctionType* get_llvm_func(std::shared_ptr<FuncType> type){
    std::vector<llvm::Type*> args;

    auto ret = get_llvm_type(type->ret_type);

    for(auto tp: type->param_list)
        args.push_back(get_llvm_type(tp));

    return llvm::FunctionType::get(ret, args, type->is_vary);
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
    std::string simple_name = "";
    for(auto ch: st->name){
        if(!isalnum(ch) && ch != '$' && ch != '_')
            break;
        simple_name.push_back(ch);
    }
    auto res = llvm::StructType::create(*llvm_ctx, simple_name + "." + std::to_string(st->id));
    struct_table[st->id] = res;

    for (auto [name, tp] : st->member) {
        members.push_back(get_llvm_type(tp));
    }

    res->setBody(members);
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

    // func_ptr_table[info->var_id] = llvm::ConstantExpr::getBitCast(llvm_fn, llvm::PointerType::get(llvm_fn_tp, 0));

    return llvm_fn;
}

llvm::Function* gen_ext_mem_func_def(AstNode* ast, IRBuilder<>& builder) {
    using namespace llvm;
    auto func_ada = Adaptor<FuncDecl>(ast);
    auto lambda_type = std::dynamic_pointer_cast<LambdaType>(ast->ret_var_type);
    auto func_type = std::make_shared<FuncType>();
    
    func_type->param_list = lambda_type->param_list;
    func_type->param_list.push_back(std::make_shared<::PointerType>(lambda_type->obj));
    func_type->ret_type = lambda_type->ret_type;

    func_ada.param_name.push_back("self");

    auto type_info = ast->get_info(lambda_type->obj->to_string() + "$" + func_ada.id);
    if (!type_info->is_top)
        throw std::invalid_argument("inner member function is not support.");
    
    auto llvm_fn_tp = (FunctionType*)get_llvm_type(func_type);
    auto llvm_fn = Function::Create(llvm_fn_tp, Function::ExternalLinkage, func_ada.id, *llvm_mod);

    auto bb = llvm::BasicBlock::Create(*llvm_ctx, func_ada.id, llvm_fn);
    builder.SetInsertPoint(bb);

    int idx = 0;
    for (auto &arg : llvm_fn->args()) {
        auto id = func_ada.body->get_info(func_ada.param_name[idx++]);
        llvm::AllocaInst *alloca = get_alloc_inst(llvm_fn, arg.getType(), id->name);
        builder.CreateStore(&arg, alloca);
        var_table[id->var_id] = { arg.getType(), alloca};
    }

    auto ret = gen_llvm_ir(func_ada.body, builder);

    if(func_ada.body->type == StmtsRet){
        if(func_ada.no_return())
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

        Value *init_ret = gen_llvm_ir_to_type(decl.init_val, decl.init_val->ret_var_type, decay(info->type), init_builder);
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

        IRBuilder<> init_fn_builder(&init_fn->getEntryBlock(), --(init_fn->getEntryBlock().end()));

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
        }
        // else if (ch->type == StructImpl) {
        //     for (auto ch : ast->ch[1]->ch)
        //         gen_ext_mem_func_def(ch, builder);
        // }
        else {
            assert(ch->type == StructDecl || ch->type == GenericBlock || ch->type == TypeDef);
            //skip Struct&Generic Decl
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

    auto lhs = gen_llvm_ir(ast->ch[0], builder);
    auto rhs = gen_llvm_ir(ast->ch[1], builder);

    bool ptr_op = decay(ast_tp)->is_ptr() || decay(ast->ch[0]->ret_var_type)->is_ptr() || decay(ast->ch[1]->ret_var_type)->is_ptr();

    if(!ptr_op){
        lhs = gen_convert(lhs, ast->ch[0]->ret_var_type, ast_tp, builder);
        rhs = gen_convert(rhs, ast->ch[1]->ret_var_type, ast_tp, builder);
    }else{
        if(!decay(ast->ch[0]->ret_var_type)->is_array())
            lhs = gen_convert(lhs, ast->ch[0]->ret_var_type, decay(ast->ch[0]->ret_var_type), builder);
        if(!decay(ast->ch[1]->ret_var_type)->is_array())
            rhs = gen_convert(rhs, ast->ch[1]->ret_var_type, decay(ast->ch[1]->ret_var_type), builder);        
    }

    switch(op){
    case op_type::Add:{
        if(res_tp->isIntegerTy())
            return builder.CreateAdd(lhs, rhs);
        else if(res_tp->isFloatingPointTy())
            return builder.CreateFAdd(lhs, rhs);
        else if (lhs->getType()->isPointerTy() && rhs->getType()->isIntegerTy()){
            // lhs->print(llvm::outs());
            lhs = gen_convert(lhs, decay(ast->ch[0]->ret_var_type), as_ptr_type(ast->ch[0]->ret_var_type), builder);
            return builder.CreateGEP(lhs, rhs);
        }
        else if (rhs->getType()->isPointerTy() && lhs->getType()->isIntegerTy()){
            rhs = gen_convert(rhs, decay(ast->ch[1]->ret_var_type), as_ptr_type(ast->ch[1]->ret_var_type), builder);
            return builder.CreateGEP(rhs, lhs);
        }
            

        break;
    }

    case op_type::Sub:{

        if(ptr_op){
            if (lhs->getType()->isPointerTy() && rhs->getType()->isIntegerTy()) {
                llvm::Value* neg_rhs = builder.CreateNeg(rhs);
                return builder.CreateGEP(lhs, neg_rhs);
            } else if (lhs->getType()->isPointerTy() && rhs->getType()->isPointerTy()) {

                auto elemSize = llvm::ConstantExpr::getSizeOf(
                    lhs->getType()->getPointerElementType());
                llvm::Value* lhsToInt = builder.CreatePtrToInt(lhs, builder.getInt64Ty());
                llvm::Value* rhsToInt = builder.CreatePtrToInt(rhs, builder.getInt64Ty());
                llvm::Value* diff = builder.CreateSub(lhsToInt, rhsToInt);
                return builder.CreateExactSDiv(diff, elemSize);
            }
            break;
        }

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
        return builder.CreateSelect(lhs, 
            llvm::ConstantInt::get(llvm::Type::getInt1Ty(*llvm_ctx), 1), rhs);
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
        if(llvm_comm_tp->isIntegerTy() || llvm_comm_tp->isPointerTy())
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

        case op_type::SubEq:{
            if(res_tp->isIntegerTy())
                res = builder.CreateSub(lhs, rhs);
            else if(res_tp->isFloatingPointTy())
                res = builder.CreateFSub(lhs, rhs);
            break;
        }

        case op_type::MulEq:{
            if(res_tp->isIntegerTy())
                res = builder.CreateMul(lhs, rhs);
            else if(res_tp->isFloatingPointTy())
                res = builder.CreateFMul(lhs, rhs);
            break;
        }

        case op_type::DivEq:{
            if(res_tp->isIntegerTy())
                res = ast_tp->is_signed() ? builder.CreateSDiv(lhs, rhs): builder.CreateUDiv(lhs, rhs);
            else if(res_tp->isFloatingPointTy())
                res = builder.CreateFDiv(lhs, rhs);
            break;
        }
        default:
            throw std::invalid_argument("op is not assign");
        }
    }
    builder.CreateStore(res, lvalue);
    return lvalue;
}

llvm::Value* gen_call_op(OperatorNode *ast, IRBuilder<>& builder){
    // TODO: support function pointer
    // if(ast->ch[0]->type != Identifier)
    //     throw std::invalid_argument("function pointer is not support yet");

    // auto func_info = ast->get_info(ast->ch[0]->str);
    std::shared_ptr<FuncType> ast_fn_tp;
    if(decay(ast->ch[0]->ret_var_type)->is_func()){
        ast_fn_tp = std::dynamic_pointer_cast<FuncType>(decay(ast->ch[0]->ret_var_type));
    }else{
        auto ptr = std::dynamic_pointer_cast<PointerType>(decay(ast->ch[0]->ret_var_type));
        ast_fn_tp = std::dynamic_pointer_cast<FuncType>(ptr->subtype);
    }

    auto function = gen_llvm_ir_decay(ast->ch[0], builder);
    auto ast_args = ast->ch[1];
    auto& params_tp = ast_fn_tp->param_list;


    std::vector<llvm::Value*> args;

    auto upper_type = [](var_type_ptr tp)-> var_type_ptr{
        tp = decay(tp);
        if(tp->is_array())
            return as_ptr_type(tp);
        if(tp->is_prim()){
            auto prim_tp = dyn_ptr_cast<PrimType>(tp);
            auto lim = (prim_tp->pr_kind == PrimType::Float ? 64: 32);
            return std::make_shared<PrimType>(std::max(prim_tp->pr_kind, PrimType::Int), std::max(lim, prim_tp->siz));
        }
        return tp;
    };

    for(size_t i = 0; i < ast_args->ch.size(); ++i){
        llvm::Value *arg;
        auto arg_tp = ast_args->ch[i]->ret_var_type;
        if(i < params_tp.size()){
            arg = gen_llvm_ir_to_type(ast_args->ch[i], arg_tp, params_tp[i], builder);
        }else{
            arg = gen_llvm_ir_to_type(ast_args->ch[i], arg_tp, upper_type(arg_tp), builder);
        }
        args.push_back(arg);
    }

    if(ast_fn_tp->ret_type->is_void())
        return builder.CreateCall(function, args);

    return builder.CreateCall(function, args, "ret");
}

llvm::Value* gen_access_op(OperatorNode *ast, IRBuilder<>& builder){
    auto zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*llvm_ctx), 0);

    if(ast->type == op_type::At){

        auto arr = gen_llvm_ir(ast->ch[0], builder);
        auto idx = gen_llvm_ir_to_type(ast->ch[1], ast->ch[1]->ret_var_type, get_type("uint64"), builder);
        

        if(decay(ast->ch[0]->ret_var_type)->is_array())
            return builder.CreateGEP(arr, {zero, idx});
        else if(decay(ast->ch[0]->ret_var_type)->is_ptr()){
            auto ptr = gen_convert(arr, ast->ch[0]->ret_var_type, decay(ast->ch[0]->ret_var_type), builder);
            return builder.CreateGEP(ptr, idx);
        }
        throw std::invalid_argument("unexpected type");

    }else if(ast->type == op_type::Access){
        auto tp = decay(ast->ch[0]->ret_var_type);

        if(tp->is_array()){
            auto len = dyn_ptr_cast<ArrayType>(tp)->len;
            return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*llvm_ctx), len);
        }

        assert(tp->is_type(VarType::Struct));
        assert(ast->ch[1]->type == Identifier);

        auto st_tp = dyn_ptr_cast<StructType>(tp);
        auto llvm_st = get_llvm_type(st_tp);
        auto st = gen_llvm_ir(ast->ch[0], builder);

        // st->print(llvm::outs());
        // st->getType()->print(llvm::outs());

        // auto ptr = gen_convert(st, ast->ch[0]->ret_var_type, tp, builder);
        auto idx = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*llvm_ctx), 
                                            st_tp->index_of(ast->ch[1]->str));
        // std::cout << " " << st_tp->index_of(ast->ch[1]->str) << std::endl;

        return builder.CreateGEP(st, {zero, idx});
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
    
    if(op == op_type::Ref){
        auto ret = gen_llvm_ir(ast->ch[0], builder);
        return ret;
    }
    
    if(op == op_type::DeRef){
        auto ptr = gen_llvm_ir_to_type(ast->ch[0], ast->ch[0]->ret_var_type, decay(ast->ch[0]->ret_var_type), builder);
        return ptr;
    }
    
    throw std::invalid_argument("unsupport op: " + get_op_name(op));
}

llvm::Value* gen_literal(AstNode* ast, IRBuilder<>& builder){
    using namespace llvm;
    switch (ast->type)
    {
    case IntLiteral:
        return ConstantInt::get(get_llvm_type(ast->ret_var_type), parse_int(ast->str));
    
    case DoubleLiteral:
        return ConstantFP::get(get_llvm_type(ast->ret_var_type), (float)parse_double(ast->str));
    
    case BoolLiteral:
        return ConstantInt::get(Type::getInt1Ty(*llvm_ctx), ast->str == "true");

    case CharLiteral:
        return ConstantInt::get(Type::getInt8Ty(*llvm_ctx), parse_char(ast->str));
    
    case PointerLiteral:
        return ConstantPointerNull::get((llvm::PointerType*)get_llvm_type(ast->ret_var_type));

    case node_type::StringLiteral:{
        Constant *str_cnst = ConstantDataArray::getString(*llvm_ctx, parse_string(ast->str), true);
        GlobalVariable *str_var = new GlobalVariable(
            *llvm_mod,
            str_cnst->getType(),
            true,
            GlobalValue::PrivateLinkage,
            str_cnst,
            ".str"
        );

        str_var->setAlignment(MaybeAlign(1));
        auto zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*llvm_ctx), 0);
        auto arr = builder.CreateGEP(str_var, zero);
        // return builder.CreateLoad(arr);
        return arr;
    }
    
    default:
        throw std::invalid_argument("unsupported literal");
        break;
    }
}

llvm::Value* gen_for_stmt(AstNode* ast, IRBuilder<>& builder){
    using llvm::BasicBlock;
    auto fn = builder.GetInsertBlock()->getParent();
    
    BasicBlock *b_begin = BasicBlock::Create(*llvm_ctx, "for.begin", fn);
    BasicBlock *b_nxt = BasicBlock::Create(*llvm_ctx, "for.nxt");
    BasicBlock *b_cond = BasicBlock::Create(*llvm_ctx, "for.cond");
    BasicBlock *b_end = BasicBlock::Create(*llvm_ctx, "for.end");

    loop_env_guard _guard(b_nxt, b_end);

    gen_llvm_ir(ast->ch[0], builder);

    builder.CreateBr(b_cond);
    builder.SetInsertPoint(b_begin);
    gen_llvm_ir(ast->ch[3], builder);
    builder.CreateBr(b_nxt);

    builder.SetInsertPoint(b_nxt);
    fn->getBasicBlockList().push_back(b_nxt);
    gen_llvm_ir(ast->ch[2], builder);
    builder.CreateBr(b_cond);

    builder.SetInsertPoint(b_cond);
    fn->getBasicBlockList().push_back(b_cond);
    if(ast->ch[1]->type == Stmt && ast->ch[1]->str == ""){
        builder.CreateBr(b_begin);
    }else{
        auto cond = gen_llvm_ir(ast->ch[1], builder);
        builder.CreateCondBr(cond, b_begin, b_end);
    }

    builder.SetInsertPoint(b_end);
    fn->getBasicBlockList().push_back(b_end);

    return nullptr;
}

llvm::Value* gen_while_stmt(AstNode* ast, IRBuilder<>& builder){
    using llvm::BasicBlock;
    auto fn = builder.GetInsertBlock()->getParent();
    
    BasicBlock *b_begin = BasicBlock::Create(*llvm_ctx, "while.begin", fn);
    BasicBlock *b_cond = BasicBlock::Create(*llvm_ctx, "while.cond");
    BasicBlock *b_end = BasicBlock::Create(*llvm_ctx, "while.end");

    loop_env_guard _guard(b_cond, b_end);

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
    builder.CreateBr(b_end);
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

llvm::Value* gen_struct_inst(AstNode* ast, IRBuilder<>& builder){
    auto st_tp = dyn_ptr_cast<StructType>(ast->ret_var_type);
    auto tp = get_llvm_type(ast->ret_var_type);
    auto st = builder.CreateAlloca(tp);
    auto mem = ast->ch[0];
    //llvm::ConstantInt::get(llvm::Type::getInt32Ty(*llvm_ctx), val);
    auto zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*llvm_ctx), 0);
    for(auto ch: mem->ch){
        auto nam = ch->ch[0]->str;
        auto mem_tp = st_tp->type_of(nam);
        auto val = gen_llvm_ir_to_type(ch->ch[1], ch->ch[1]->ret_var_type, decay(mem_tp), builder);
        auto idx = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*llvm_ctx), st_tp->index_of(nam));
        auto addr = builder.CreateGEP(st, {zero, idx});
        builder.CreateStore(val, addr);
    }
    return builder.CreateLoad(st);
}

llvm::Value* gen_array_inst(AstNode* ast, IRBuilder<>& builder){
    auto arr_tp = dyn_ptr_cast<ArrayType>(ast->ret_var_type);
    auto tp = get_llvm_type(ast->ret_var_type);
    auto arr = builder.CreateAlloca(tp);
    //llvm::ConstantInt::get(llvm::Type::getInt32Ty(*llvm_ctx), val);
    auto zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*llvm_ctx), 0);
    for(size_t i = 0; i < ast->ch.size(); ++i){
        auto ch = ast->ch[i];
        auto val = gen_llvm_ir_to_type(ch, ch->ret_var_type, decay(arr_tp->subtype), builder);
        auto idx = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*llvm_ctx), i);
        auto addr = builder.CreateGEP(arr, {zero, idx});
        builder.CreateStore(val, addr);
    }
    return builder.CreateLoad(arr);
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
        return gen_for_stmt(ast, builder);
    }

    case VarDecl:{
        gen_var_decl(ast, builder);
        return nullptr;
    }

    case TypeDef:{
        return nullptr;
    }

    case Operator:{
        return gen_operator((OperatorNode*)ast, builder);
    }

    case Stmt:{
        if(ast->str == "return"){
            llvm::Value* res;
            if(ast->ch.size()){
                auto ast_fn = Adaptor<FuncDecl>(ast->get_func_parent());
                auto ret = gen_llvm_ir_to_type(ast->ch[0], ast->ch[0]->ret_var_type, 
                                                            ast_fn.type_info->ret_type, builder);
                res = builder.CreateRet(ret);
            }else{
                res =  builder.CreateRetVoid();
            }
            
            auto fn = builder.GetInsertBlock()->getParent();
            llvm::BasicBlock *never_reach = llvm::BasicBlock::Create(*llvm_ctx, "never", fn);
            builder.SetInsertPoint(never_reach);
            return res;
        }else if(ast->str == "break"){
            auto [nxt, end] = loop_stack.top();
            builder.CreateBr(end);
            auto fn = builder.GetInsertBlock()->getParent();
            llvm::BasicBlock *never_reach = llvm::BasicBlock::Create(*llvm_ctx, "never", fn);
            builder.SetInsertPoint(never_reach);
            return nullptr;
        }else if(ast->str == "continue"){
            auto [nxt, end] = loop_stack.top();
            builder.CreateBr(nxt);
            auto fn = builder.GetInsertBlock()->getParent();
            llvm::BasicBlock *never_reach = llvm::BasicBlock::Create(*llvm_ctx, "never", fn);
            builder.SetInsertPoint(never_reach);
            return nullptr;
        }else if(ast->str == ""){
            return nullptr;
        }
        break;
    }

    case StructDecl: case GenericBlock:{
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
        if(auto res = get_var_inst(info->var_id))
            return res;

        if(auto res = llvm_mod->getFunction(info->name))
            return res;
        
        throw std::runtime_error("identifier not found: "+ info->name);
    }

    case ExprList:{
        llvm::Value* ret = nullptr;
        for(auto ch: ast->ch)
            ret = gen_llvm_ir(ch, builder);
        if(!ast->ret_var_type->is_void())
            return gen_convert(ret, ast->ch.back()->ret_var_type, ast->ret_var_type, builder);
        return ret;
    }

    case FuncDecl:{
        throw std::invalid_argument("inner function is not support");
        return nullptr;
    }

    case StructInstance:{
        return gen_struct_inst(ast, builder);
    }

    case ArrayInstance:{
        return gen_array_inst(ast, builder);
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
    // std::cout << "trying covert: " << ast_from->to_string() << " to " << ast_to->to_string() << std::endl;
    auto from = get_llvm_type(ast_from);
    auto to = get_llvm_type(ast_to);
    auto from_signed = ast_from->is_signed();
    auto to_signed = ast_to->is_signed();

    if (decay(ast_from)->is_array() && ast_to->is_ptr()) {
        // Array to Pointer Conversion
        // Convert an array type to a pointer to its element type
        // std::cout << "decay1" << std::endl;
        return builder.CreateGEP(from_v, {builder.getInt32(0), builder.getInt32(0)});
    }

    if(ast_from->is_ref() && !ast_to->is_ref()){
        // std::cout << "deref" << std::endl;
        if(ast_to->is_array() || ast_to->is_type(VarType::Struct)){
            auto zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*llvm_ctx), 0);
            auto arr = builder.CreateGEP(from_v, zero);
            // return from_v;
            return builder.CreateLoad(arr);
        }else if(decay(ast_from)->is_func()){
            return gen_convert(from_v, decay(ast_from), ast_to, builder);
        }else{
            auto res = builder.CreateLoad(from_v);
            return gen_convert(res, decay(ast_from), decay(ast_to), builder);
        }
    }

    if(ast_from->is_same(ast_to.get()))
        return from_v;

    if(decay(ast_from)->is_func() && decay(ast_to)->is_ptr()){
        return builder.CreatePointerCast(from_v, get_llvm_type(ast_to));
    }

    if(ast_from->is_ptr() && ast_to->is_ptr()){
        return builder.CreatePointerCast(from_v, to, "ptr");
    }

    if (from->isPointerTy() && to->isIntegerTy()) {
        // Pointer to Integer Conversion
        return builder.CreatePtrToInt(from_v, to);
    }

    if (from->isIntegerTy() && to->isPointerTy()) {
        // Integer to Pointer Conversion
        return builder.CreateIntToPtr(from_v, to);
    }

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

llvm::Value* gen_llvm_ir_decay(AstNode* ast, IRBuilder<>& builder){
    return gen_llvm_ir_to_type(ast, ast->ret_var_type, decay(ast->ret_var_type), builder);
}

void write_llvm_ir(std::ostream& os){
    llvm::raw_os_ostream raw_os{os};
    llvm_mod->print(raw_os, nullptr);
}