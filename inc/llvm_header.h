#pragma once

#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/GlobalVariable.h"

#include "llvm/Support/raw_os_ostream.h"

using llvm::LLVMContext;
using llvm::Module;
using llvm::IRBuilder;