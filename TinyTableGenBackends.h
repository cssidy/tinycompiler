#pragma once
namespace llvm {
class raw_ostream;
class RecordKeeper;
}
void EmitInstInfo(const llvm::RecordKeeper &RK, llvm::raw_ostream &OS);
