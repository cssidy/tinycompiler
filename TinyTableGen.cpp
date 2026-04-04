#include "TinyTableGenBackends.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"

using namespace llvm;

enum ActionType { PrintRecords, GenInsts };

namespace {
cl::opt<ActionType> Action(
    cl::desc("Action to perform:"),
    cl::values(
        clEnumValN(PrintRecords, "print-records", "Print all records to stdout"),
        clEnumValN(GenInsts,     "gen-insts",     "Generate instruction enum and info table")));

bool Main(raw_ostream &OS, const RecordKeeper &Records) {
    switch (Action) {
    case PrintRecords:
        OS << Records;
        break;
    case GenInsts:
        EmitInstInfo(Records, OS);
        break;
    }
    return false;
}
} // namespace

int main(int argc, char **argv) {
    sys::PrintStackTraceOnErrorSignal(argv[0]);
    PrettyStackTraceProgram X(argc, argv);
    cl::ParseCommandLineOptions(argc, argv);
    return TableGenMain(argv[0], &Main);
}
