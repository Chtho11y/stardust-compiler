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

using llvm_val_info = std::pair<llvm::Type*, llvm::Value*>;
// std::stack<std::map<std::string, llvm_val_info>> var_stack;
std::map<size_t, llvm_val_info> var_table;
std::map<size_t, llvm::GlobalVariable*> global_var_table;
std::map<size_t, llvm::StructType*> struct_table;
std::map<std::string, llvm::GlobalVariable*> impl_trait_table;
// std::map<size_t, llvm::Value*> func_ptr_table;

std::stack<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> loop_stack;

void gen_llvm_struct(sd::StructType* st);
llvm::Value* gen_convert(llvm::Value* from_v, var_type_ptr from, var_type_ptr to, IRBuilder<>& builder);
llvm::Value* gen_llvm_ir_to_type(AstNode* ast, var_type_ptr from, var_type_ptr to, IRBuilder<>& builder);

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

    if(var_table.count(id)) {
        return var_table[id].second;
    }
        
    // if(func_ptr_table.count(id))
    //     return func_ptr_table[id];
    if(global_var_table.count(id))
        return global_var_table[id];
    // throw std::invalid_argument("unknown variable id " + std::to_string(id));
    return nullptr;
}

llvm::Value* get_llvm_int(int64_t x){
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*llvm_ctx), x);
}

llvm::Value* get_llvm_int32(int64_t x){
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*llvm_ctx), x);
}

namespace type_aux{

llvm::Type* get_llvm_prim(sd::PrimType* type){
    using llvm::Type;
    using sd::PrimType;

    switch (type->kind){

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

llvm::ArrayType* get_llvm_array(sd::ArrayType* type){
    auto subtype = get_llvm_type(type->subtype);
    return llvm::ArrayType::get(subtype, type->len);
}

llvm::StructType* get_llvm_struct(sd::StructType* type){
    if(!struct_table.count(type->id))
        gen_llvm_struct(type);
    return struct_table[type->id];
}

llvm::PointerType* get_llvm_pointer(sd::PointerType* type){
    auto subtype = get_llvm_type(type->subtype);
    if(subtype->isVoidTy())
        return llvm::PointerType::get(llvm::Type::getInt8Ty(*llvm_ctx), 0);
    return llvm::PointerType::get(subtype, 0);
    // return llvm::PointerType::get(llvm::Type::getInt8Ty(*llvm_ctx), 0);
}

llvm::PointerType* get_llvm_ref(sd::RefType* type){
    auto subtype = get_llvm_type(type->subtype);
    return llvm::PointerType::get(subtype, 0);
    // return llvm::PointerType::get(llvm::Type::getInt8Ty(*llvm_ctx), 0);
}

llvm::FunctionType* get_llvm_func(sd::FuncType* type){
    std::vector<llvm::Type*> args;

    auto ret = get_llvm_type(type->ret_type);

    for(auto tp: type->param_list)
        args.push_back(get_llvm_type(tp));

    return llvm::FunctionType::get(ret, args, type->is_vary);
}

llvm::FunctionType* get_llvm_mem_func(sd::MemFuncType* type){
    std::vector<llvm::Type*> args;
    
    auto ret = get_llvm_type(type->ret_type);

    for(auto tp: type->param_list)
        args.push_back(get_llvm_type(tp));
    args.push_back(get_llvm_type(sd::PointerType::get(sd::VoidType::get())));

    return llvm::FunctionType::get(ret, args, type->is_vary);
}

};// type_aux

llvm::Type* get_trait_proto();

llvm::Type* get_llvm_type(var_type_ptr type){
    using namespace type_aux;
    using sd::type_kind;

    switch(type->type){

    case type_kind::Prim: 
        return get_llvm_prim(sd::dyn_ptr_cast<sd::PrimType>(type));

    case type_kind::Void:
        return llvm::Type::getVoidTy(*llvm_ctx);

    case type_kind::Func:
        return get_llvm_func(sd::dyn_ptr_cast<sd::FuncType>(type));
    
    case type_kind::MemFunc:
        return get_llvm_mem_func(type->cast<sd::MemFuncType>());

    case type_kind::Pointer:
        return get_llvm_pointer(sd::dyn_ptr_cast<sd::PointerType>(type));

    case type_kind::Array:
        return get_llvm_array(sd::dyn_ptr_cast<sd::ArrayType>(type));

    case type_kind::Struct:
        return get_llvm_struct(sd::dyn_ptr_cast<sd::StructType>(type));
    
    case type_kind::Ref:
        return get_llvm_ref(sd::dyn_ptr_cast<sd::RefType>(type));
    
    case type_kind::Trait:
        return get_trait_proto();
    }
    throw std::invalid_argument("unsupport type: " + type->to_string());
}

llvm::Type* get_trait_proto(){
    if(!struct_table.count(-1)){
        auto proto = llvm::StructType::create(*llvm_ctx, "trait.proto");
        std::vector<llvm::Type*> mem;
        auto void_ptr = sd::PointerType::get(sd::VoidType::get());
        mem.push_back(get_llvm_type(void_ptr));
        mem.push_back(get_llvm_type(void_ptr));
        proto->setBody(mem);
        struct_table[-1] = proto;
    }
    return struct_table[-1];
}

llvm_val_info add_var(var_info_ptr var, IRBuilder<>& builder){

    auto llvm_type = get_llvm_type(var->type);
    auto fn = builder.GetInsertBlock()->getParent();

    auto alloca = get_alloc_inst(fn, llvm_type, var->name);

    var_table[var->var_id] = {llvm_type, alloca};
    return {llvm_type, alloca};
}

void gen_llvm_struct(sd::StructType* st){
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

llvm::Function* gen_func_def(AstNode* ast, IRBuilder<>& builder){
    using namespace llvm;

    auto func = Adaptor<FuncDecl>(ast);

    auto info = ast->get_info(func.id);
    
    auto llvm_fn_tp = (FunctionType*)get_llvm_type(func.type_info);
    auto func_id = info->id_name();
    auto llvm_fn = Function::Create(llvm_fn_tp, Function::ExternalLinkage, func_id, *llvm_mod);

    if(func.id == "main")
        main_fn = llvm_fn;

    auto bb = llvm::BasicBlock::Create(*llvm_ctx, func_id, llvm_fn);
    auto n_builder = builder;
    n_builder.SetInsertPoint(bb);

    int idx = 0;
    for (auto &arg : llvm_fn->args()) {
        auto id = func.body->get_info(func.param_name[idx++]);

        if(id->type->is_ref()){
            auto zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*llvm_ctx), 0);
            auto dst = n_builder.CreateGEP(&arg, zero, id->name);
            var_table[id->var_id] = { arg.getType(), dst};
        }else{
            // llvm::AllocaInst *alloca = get_alloc_inst(llvm_fn, arg.getType(), id->name);
            llvm::AllocaInst *alloca = n_builder.CreateAlloca(arg.getType(), 0, nullptr, id->name);
            n_builder.CreateStore(&arg, alloca);
            var_table[id->var_id] = { arg.getType(), alloca};
        }
    }

    auto ret = gen_llvm_ir(func.body, n_builder);

    if(func.body->type == StmtsRet){
        if(func.no_return())
            n_builder.CreateRetVoid();
        else n_builder.CreateRet(ret);
    }else if (n_builder.GetInsertBlock()->getTerminator() == nullptr) {
        llvm::Type *ret_type = llvm_fn->getReturnType();
        gen_default_ret(ret_type, n_builder);
    }

    return llvm_fn;
}

llvm::Function* gen_ext_mem_func_def(AstNode* ast, IRBuilder<>& builder, std::string trait_name = "") {
    using namespace llvm;
    auto func_ada = Adaptor<FuncDecl>(ast);
    auto lambda_type = func_ada.type_info->cast<sd::MemFuncType>();
    func_ada.param_name.push_back("self");
    var_info_ptr type_info;
    if (trait_name == "")
        type_info = ast->get_info(lambda_type->parent_type->to_string() + "$" + func_ada.id);
    else 
        type_info = ast->get_info(lambda_type->parent_type->to_string() + "$" + trait_name + "$" + func_ada.id);
    auto parent = lambda_type->parent_type;
    if (!type_info->is_top)
        throw std::invalid_argument("inner member function is not support.");
    auto llvm_fn_tp = (FunctionType*)get_llvm_type(lambda_type);
    auto llvm_fn = Function::Create(
        llvm_fn_tp, Function::ExternalLinkage, parent->get_prefix() + func_ada.id, *llvm_mod);

    auto bb = llvm::BasicBlock::Create(*llvm_ctx, func_ada.id, llvm_fn);
    auto n_builder = builder;
    n_builder.SetInsertPoint(bb);

    int idx = 0;
    for (auto &arg : llvm_fn->args()) {
        auto id = func_ada.body->get_info(func_ada.param_name[idx++]);
        llvm::Value* val = &arg;
        if(idx == func_ada.param_name.size()){
            val = n_builder.CreatePointerCast(&arg, get_llvm_type(sd::PointerType::get(parent)));
        }
        if(id->type->is_ref()){
            auto zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*llvm_ctx), 0);
            auto dst = n_builder.CreateGEP(val, zero, id->name);
            var_table[id->var_id] = { val->getType(), dst};
        }else{
            llvm::AllocaInst *alloca = get_alloc_inst(llvm_fn, val->getType(), id->name);
            n_builder.CreateStore(val, alloca);
            var_table[id->var_id] = { val->getType(), alloca};
        }
    }

    auto ret = gen_llvm_ir(func_ada.body, n_builder);

    if(func_ada.body->type == StmtsRet){
        if(func_ada.no_return())
            n_builder.CreateRetVoid();
        else n_builder.CreateRet(ret);
    }else if (n_builder.GetInsertBlock()->getTerminator() == nullptr) {
        llvm::Type *ret_type = llvm_fn->getReturnType();
        gen_default_ret(ret_type, n_builder);
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

        Value *init_ret = gen_llvm_ir(decl.init_val, init_builder);
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

    if(var_info->type->is_ref()){
        assert(var_node.init_val);
        auto val = gen_llvm_ir(var_node.init_val, builder);
        var_table[var_info->var_id] = {val->getType(), val};
        return;
    }

    auto [var_tp, alloca] = add_var(var_info, builder);

    if(var_node.init_val){
        auto val = gen_llvm_ir(var_node.init_val, builder);
        builder.CreateStore(val, alloca);
    }
}

void gen_ext_decl_list(AstNode* ast, IRBuilder<>& builder){
    for(auto ch: ast->ch){
        if(ch->type == FuncDecl){
            gen_func_def(ch, builder);
        }else if(ch->type == VarDecl){
            gen_ext_var_decl(ch, builder);
        }
        else if (ch->type == StructImpl) {
            for (auto mem_fn : ch->ch[1]->ch){
                gen_ext_mem_func_def(mem_fn, builder);
            }
        }
        else if (ch->type == TraitDecl) {
            std::vector<llvm::Type*> members;
            std::string simple_name = "";
            auto st = ast_to_type(ch->ch[0]);
            for(auto ch: st->to_string()){
                if(!isalnum(ch) && ch != '$' && ch != '_')
                    break;
                simple_name.push_back(ch);
            }
            auto res = llvm::StructType::create(*llvm_ctx, simple_name + "." + std::to_string(st->id));
            struct_table[st->id] = res;
            auto void_ptr = sd::PointerType::get(sd::VoidType::get());
            for (int i = 0; i < ch->ch[1]->ch.size(); i++)
                members.push_back(get_llvm_type(void_ptr));
            res->setBody(members);
        }
        else if (ch->type == TraitImpl) {
            auto trait_type = sd::dyn_ptr_cast<sd::TraitType>(ast_to_type(ch->ch[0]));
            auto impl_type = ast_to_type(ch->ch[1]);
            auto trait_llvm_type = struct_table[trait_type->id];
            auto trait_impl_name = trait_type->to_string() + "$" + impl_type->to_string();
            auto var = new llvm::GlobalVariable(
                *llvm_mod, trait_llvm_type, false,
                llvm::GlobalValue::ExternalLinkage,
                llvm::UndefValue::get(trait_llvm_type),
                trait_impl_name              
            );

            impl_trait_table[trait_impl_name] = var;

            using namespace llvm;
            auto init_val_fn_tp = llvm::FunctionType::get(trait_llvm_type, false);
            auto init_val_fn = Function::Create(
                init_val_fn_tp,
                GlobalValue::InternalLinkage,
                trait_impl_name + ".init",
                *llvm_mod
            );
            BasicBlock *bb = BasicBlock::Create(*llvm_ctx, "entry", init_val_fn);
            IRBuilder<> init_builder(bb);

            // Value *init_ret = gen_llvm_ir(decl.init_val, init_builder);
            std::vector<llvm::Function*> llvm_func_list;
            llvm_func_list.resize(ch->ch[2]->ch.size());
            for (auto ch1 : ch->ch[2]->ch) {
                int pos = -1;
                auto trait_func_ada = Adaptor<FuncDecl>(ch1);
                // std::cout << trait_func_ada.id << ' ' << trait_func_ada.type_info->to_string() << '\n';
                for (int i = 0; i < trait_type->func_list.size(); i++)
                    if (trait_func_ada.id == trait_type->func_list[i].first 
                        //&& trait_func_ada.type_info == trait_type->func_list[i].second
                    )
                        pos = i;
                assert(pos != -1);
                llvm_func_list[pos] = gen_ext_mem_func_def(ch1, init_builder, ch->ch[0]->str);
            }
            auto base = init_builder.CreateAlloca(trait_llvm_type);
            for (int i = 0; i < llvm_func_list.size(); i++) {
                auto fn_ptr = init_builder.CreateGEP(base, {get_llvm_int(0), get_llvm_int32(i)});
                auto fn_addr = init_builder.CreatePointerCast(llvm_func_list[i], get_llvm_type(sd::PointerType::get(sd::VoidType::get())));
                init_builder.CreateStore(fn_addr, fn_ptr);
            }

            init_builder.CreateRet(base);

            if(!init_fn){
                auto void_call = llvm::FunctionType::get(llvm::Type::getVoidTy(*llvm_ctx), false);
                init_fn = llvm::Function::Create(void_call, llvm::Function::ExternalLinkage, "global.var.init", *llvm_mod);
                auto bb = llvm::BasicBlock::Create(*llvm_ctx, "entry", init_fn);
                IRBuilder<>(bb).CreateRetVoid();
            }

            IRBuilder<> init_fn_builder(&init_fn->getEntryBlock(), --(init_fn->getEntryBlock().end()));

            auto init_ret = init_fn_builder.CreateCall(init_val_fn);
            init_fn_builder.CreateStore(init_ret, var);

        }
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

llvm::Value* gen_unary_op(OperatorNode* ast, IRBuilder<>& builder){
    auto val = gen_llvm_ir(ast->ch[0], builder);

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

    switch(op){
    case op_type::Add:{
        if(res_tp->isIntegerTy())
            return builder.CreateAdd(lhs, rhs);
        else if(res_tp->isFloatingPointTy())
            return builder.CreateFAdd(lhs, rhs);
        else if (lhs->getType()->isPointerTy() && rhs->getType()->isIntegerTy()){
            return builder.CreateGEP(lhs, rhs);
        }
        else if (rhs->getType()->isPointerTy() && lhs->getType()->isIntegerTy()){
            return builder.CreateGEP(rhs, lhs);
        }
            
        break;
    }

    case op_type::Sub:{

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
        }else if(res_tp->isIntegerTy())
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
    auto lhs = gen_llvm_ir(ast->ch[0], builder);
    auto rhs = gen_llvm_ir(ast->ch[1], builder);

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
    auto res_tp = get_llvm_type(ast_tp->decay());
    op_type op = ast->type;

    auto lvalue = gen_llvm_ir(ast->ch[0], builder);
    auto rhs = gen_llvm_ir(ast->ch[1], builder);

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

llvm::Value* gen_trait_call(llvm::Value* trait, size_t fn_idx, sd::FuncType* fn_tp, AstNode *args, IRBuilder<>& builder){
    auto zero = get_llvm_int(0);
    auto self = builder.CreateGEP(trait, {zero, get_llvm_int32(0)});
    self = builder.CreateLoad(self, "self");
    auto fn_base = builder.CreateGEP(trait, {zero, get_llvm_int32(1)});
    auto llvm_fn_tp = get_llvm_type(sd::PointerType::get(fn_tp));

    llvm::Value* fn_addr = builder.CreateLoad(fn_base);
    fn_addr = builder.CreateGEP(fn_addr, get_llvm_int(fn_idx));
    auto fn = builder.CreatePointerCast(fn_addr, llvm_fn_tp);

    auto& params_tp = fn_tp->param_list;

    std::vector<llvm::Value*> args_list;

    for(size_t i = 0; i < args->ch.size(); ++i){
        llvm::Value *arg = gen_llvm_ir(args->ch[i], builder);
        auto arg_tp = args->ch[i]->ret_var_type;
        args_list.push_back(arg);
    }

    args_list.push_back(self);

    if(fn_tp->ret_type->is_void())
        return builder.CreateCall(fn, args_list);

    return builder.CreateCall(fn, args_list, "ret");    
}

llvm::Value* gen_call_op(OperatorNode *ast, IRBuilder<>& builder){

    if(ast->ch[0]->ret_var_type->decay()->is_mem_func()){
        auto trait = gen_llvm_ir(ast->ch[0], builder);
        return gen_trait_call(trait, 0, ast->ch[0]->ret_var_type->decay()->cast<sd::MemFuncType>(), ast->ch[1], builder);
    }

    sd::FuncType* ast_fn_tp;
    auto lhs = ast->ch[0]->ret_var_type->decay();
    if(lhs->is_func()){
        ast_fn_tp = lhs->cast<sd::FuncType>();
    }else{
        ast_fn_tp  = lhs->cast<sd::PointerType>()->subtype->cast<sd::FuncType>();
    }
    auto function = gen_llvm_ir(ast->ch[0], builder);
    auto ast_args = ast->ch[1];
    auto& params_tp = ast_fn_tp->param_list;

    std::vector<llvm::Value*> args;

    for(size_t i = 0; i < ast_args->ch.size(); ++i){
        llvm::Value *arg = gen_llvm_ir(ast_args->ch[i], builder);
        auto arg_tp = ast_args->ch[i]->ret_var_type;
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
        auto idx = gen_llvm_ir(ast->ch[1], builder);

        if(ast->ch[0]->ret_var_type->decay()->is_array())
            return builder.CreateGEP(arr, {zero, idx});
        else if(ast->ch[0]->ret_var_type->decay()->is_ptr()){
            return builder.CreateGEP(arr, idx);
        }
        throw std::invalid_argument("unexpected type");

    }else if(ast->type == op_type::Access){

        auto tp = ast->ch[0]->ret_var_type->decay();
        assert(ast->ch[1]->type == Identifier);
        
        if(ast->ret_var_type->decay()->is_mem_func()){
            auto base = builder.CreateAlloca(get_trait_proto());
            assert(ast->ch[0]->type == Identifier);
            auto obj = gen_llvm_ir(ast->ch[0], builder);

            if(!ast->ch[0]->ret_var_type->is_ref()){
                auto obj_addr = builder.CreateAlloca(get_llvm_type(tp));
                builder.CreateStore(obj, obj_addr);
                obj = obj_addr;
            }

            if(ast->ch[0]->ret_var_type->decay()->is_trait()) {
                auto func_id = ast->ch[1]->str;
                auto trait_type = sd::dyn_ptr_cast<sd::TraitType>(ast->ch[0]->ret_var_type->decay());
                int pos = -1;
                for (int i = 0; i < trait_type->func_list.size(); i++)
                    if (trait_type->func_list[i].first == func_id) {
                        pos = i;
                        break;
                    }
                assert(pos != -1);

                auto zero = get_llvm_int(0);
                auto fn_base_addr = builder.CreateGEP(obj, {zero, get_llvm_int32(1)});
                llvm::Value* fn_base = builder.CreateLoad(fn_base_addr);
                auto trait_llvm_type = struct_table[trait_type->id];
                fn_base = builder.CreatePointerCast(fn_base, llvm::PointerType::get(trait_llvm_type, 0));
                auto dest_func_addr = builder.CreateGEP(fn_base, {zero, get_llvm_int32(pos)});
                llvm::Value* dest_func = builder.CreateLoad(dest_func_addr);
                builder.CreateStore(builder.CreatePointerCast(dest_func, get_llvm_type(sd::PointerType::get(sd::VoidType::get()))), fn_base_addr);
                return base;
            }
            


            obj = builder.CreatePointerCast(obj, get_llvm_type(sd::PointerType::get(sd::VoidType::get())));

            auto fn = llvm_mod->getFunction(tp->get_prefix() + ast->ch[1]->str);
            auto zero = get_llvm_int(0);
            auto obj_ptr = builder.CreateGEP(base, {zero, get_llvm_int32(0)}, "obj");
            builder.CreateStore(obj, obj_ptr);

            auto fn_ptr = builder.CreateGEP(base, {zero, get_llvm_int32(1)}, "fn");
            auto fn_addr = builder.CreatePointerCast(fn, get_llvm_type(sd::PointerType::get(sd::VoidType::get())));
            builder.CreateStore(fn_addr, fn_ptr);

            return base;          
        }

        assert(tp->is_struct());
        
        if(tp->is_array()){
            auto len = tp->cast<sd::ArrayType>()->len;
            return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*llvm_ctx), len);
        }

        auto st_tp = tp->cast<sd::StructType>();
        auto llvm_st = get_llvm_type(st_tp);
        auto st = gen_llvm_ir(ast->ch[0], builder);

        auto idx = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*llvm_ctx), 
                                            st_tp->index_of(ast->ch[1]->str));

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
    
    if(op == op_type::Convert || op == op_type::ImpConvert){
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
        auto ptr = gen_llvm_ir(ast->ch[0], builder);
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
    builder.CreateBr(b_end);
    BasicBlock *b_then_end = builder.GetInsertBlock();

    fn->getBasicBlockList().push_back(b_else);
    builder.SetInsertPoint(b_else);
    auto br_else = gen_llvm_ir(ast->ch[2], builder);

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
    auto st_tp = sd::dyn_ptr_cast<sd::StructType>(ast->ret_var_type);
    auto tp = get_llvm_type(ast->ret_var_type);
    auto st = builder.CreateAlloca(tp);
    auto mem = ast->ch[0];
    //llvm::ConstantInt::get(llvm::Type::getInt32Ty(*llvm_ctx), val);
    auto zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*llvm_ctx), 0);
    for(auto ch: mem->ch){
        auto nam = ch->ch[0]->str;
        auto val = gen_llvm_ir(ch->ch[1], builder);
        auto idx = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*llvm_ctx), st_tp->index_of(nam));
        auto addr = builder.CreateGEP(st, {zero, idx});
        builder.CreateStore(val, addr);
    }
    return builder.CreateLoad(st);
}

llvm::Value* gen_array_inst(AstNode* ast, IRBuilder<>& builder){
    auto arr_tp = sd::dyn_ptr_cast<sd::ArrayType>(ast->ret_var_type);
    auto tp = get_llvm_type(ast->ret_var_type);
    auto arr = builder.CreateAlloca(tp);
    //llvm::ConstantInt::get(llvm::Type::getInt32Ty(*llvm_ctx), val);
    auto zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*llvm_ctx), 0);
    for(size_t i = 0; i < ast->ch.size(); ++i){
        auto ch = ast->ch[i];
        auto val = gen_llvm_ir(ch, builder);
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
                auto ret = gen_llvm_ir(ast->ch[0], builder);
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
        if(auto res = get_var_inst(info->var_id)) {
            return res;
        }
            

        if(auto res = llvm_mod->getFunction(info->id_name()))
            return res;
        
        throw std::runtime_error("identifier not found: "+ info->name);
    }

    case ExprList:{
        llvm::Value* ret = nullptr;
        for(auto ch: ast->ch)
            ret = gen_llvm_ir(ch, builder);
        return ret;
    }

    case FuncDecl:{
        // throw std::invalid_argument("inner function is not support");
        return gen_func_def(ast, builder);
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
    if (ast_to->is_trait()) {
        auto base = builder.CreateAlloca(get_trait_proto());
        auto tp = ast_from->decay();
        auto obj = from_v;

        if(!ast_from->is_ref()){
            auto obj_addr = builder.CreateAlloca(get_llvm_type(tp));
            builder.CreateStore(obj, obj_addr);
            obj = obj_addr;
        }

        obj = builder.CreatePointerCast(obj, get_llvm_type(sd::PointerType::get(sd::VoidType::get())));

        auto impl_trait_id = ast_to->to_string() + "$" + ast_from->decay()->to_string();
        assert(impl_trait_table.count(impl_trait_id) && "Error with impl_trait_id!");
        auto fn = impl_trait_table[impl_trait_id];

        // auto fn = llvm_mod->getFunction(tp->get_prefix() + ast->ch[1]->str);
        auto zero = get_llvm_int(0);
        auto obj_ptr = builder.CreateGEP(base, {zero, get_llvm_int32(0)}, "obj");
        builder.CreateStore(obj, obj_ptr);

        auto fn_ptr = builder.CreateGEP(base, {zero, get_llvm_int32(1)}, "fn");
        auto fn_addr = builder.CreatePointerCast(fn, get_llvm_type(sd::PointerType::get(sd::VoidType::get())));
        builder.CreateStore(fn_addr, fn_ptr);

        return base;         
    }
    auto from = get_llvm_type(ast_from);
    auto to = get_llvm_type(ast_to);
    auto from_signed = ast_from->is_signed();
    auto to_signed = ast_to->is_signed();

    if(ast_from == ast_to)
        return from_v;

    if (ast_from->decay()->is_array() && ast_to->is_ptr()) {
        // Array to Pointer Conversion
        // Convert an array type to a pointer to its element type
        // std::cout << "decay1" << std::endl;
        return builder.CreateGEP(from_v, {builder.getInt32(0), builder.getInt32(0)});
    }

    if(ast_from->is_ref()){
        assert(!ast_to->is_ref());
        // std::cout << "deref" << std::endl;
        if(ast_to->is_array() || ast_to->is_struct()){
            auto zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*llvm_ctx), 0);
            auto arr = builder.CreateGEP(from_v, zero);
            // return from_v;
            return builder.CreateLoad(arr);
        }else if(ast_from->decay()->is_func()){
            return gen_convert(from_v, ast_from->decay(), ast_to, builder);
        }else{
            auto res = builder.CreateLoad(from_v);
            return gen_convert(res, ast_from->decay(), ast_to, builder);
        }
    }

    if(ast_from->is_func() && ast_to->is_ptr()){
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
            }else return from_v;
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