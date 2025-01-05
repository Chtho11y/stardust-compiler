#pragma once

#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/LegacyPassManager.h"

#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include "llvm/Support/raw_os_ostream.h"
#include <llvm/Support/Host.h>
#include "llvm/Support/FileSystem.h"
#include <llvm/Analysis/TargetFolder.h>
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"



using llvm::LLVMContext;
using llvm::Module;
using llvm::IRBuilder;