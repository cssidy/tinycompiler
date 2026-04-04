#pragma once
#include "llvm/ADT/StringRef.h"

namespace llvm { class Module; }

namespace tinyjs {
void emitTinyCPU(llvm::Module *M, llvm::StringRef OutPath);
} // namespace tinyjs
