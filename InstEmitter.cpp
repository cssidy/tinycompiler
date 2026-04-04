#include "TinyTableGenBackends.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

using namespace llvm;

namespace {
class InstInfoEmitter {
    const RecordKeeper &Records;
public:
    explicit InstInfoEmitter(const RecordKeeper &R) : Records(R) {}
    void run(raw_ostream &OS);
private:
    void emitEnum(raw_ostream &OS);
    void emitTable(raw_ostream &OS);
};
} // namespace

void InstInfoEmitter::emitEnum(raw_ostream &OS) {
    OS << "#ifdef GET_INST_ENUM\n#undef GET_INST_ENUM\n";
    OS << "enum TinyCPUOpcode {\n";
    unsigned Idx = 0;
    for (const Record *R : Records.getAllDerivedDefinitions("Inst"))
        OS << "  " << R->getName() << " = " << Idx++ << ",\n";
    OS << "  NUM_TINYCPU_INSTS\n};\n";
    OS << "#endif\n\n";
}

void InstInfoEmitter::emitTable(raw_ostream &OS) {
    OS << "#ifdef GET_INST_INFO\n#undef GET_INST_INFO\n";
    for (const Record *R : Records.getAllDerivedDefinitions("Inst")) {
        StringRef Mnemonic = R->getValueAsString("Mnemonic");
        int64_t   Opcode   = R->getValueAsInt("Opcode");
        int64_t   NumOps   = R->getValueAsInt("NumOps");
        OS << "  { \"" << Mnemonic << "\", " << Opcode << ", " << NumOps << " },\n";
    }
    OS << "#endif\n";
}

void InstInfoEmitter::run(raw_ostream &OS) {
    emitSourceFileHeader("TinyCPU Instruction Info", OS);
    emitEnum(OS);
    emitTable(OS);
}

void EmitInstInfo(const RecordKeeper &RK, raw_ostream &OS) {
    InstInfoEmitter(RK).run(OS);
}
