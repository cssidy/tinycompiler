#pragma once
#include "AST.h"
#include "llvm/IR/Module.h"

namespace tinyjs {

void emitClasses(llvm::Module *M, DeclList &Decls);

}
