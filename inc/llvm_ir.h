#pragma once

#include "llvm_header.h"
#include "ast.h"

llvm::Type* get_llvm_type(var_type_ptr type);
llvm::Value* gen_llvm_ir(AstNode* ast, IRBuilder<>& builder);
void gen_module(AstNode* ast, std::string module_name);
void write_llvm_ir(std::ostream& os);
