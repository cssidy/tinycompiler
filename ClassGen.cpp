#include "ClassGen.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include <map>

using namespace llvm;
using namespace tinyjs;


// recurse to base so inherited fields come first
static void collectFields(ClassDecl *CD, std::vector<FieldDecl *> &Out) {
    if (CD->getBase())
        collectFields(CD->getBase(), Out);
    for (auto *F : CD->getFields())
        Out.push_back(F);
}

// vtable size is determined by the root class's virtual methods
static unsigned vtableSize(ClassDecl *CD) {
    ClassDecl *Root = CD;
    while (Root->getBase()) Root = Root->getBase();
    unsigned N = 0;
    for (auto *M : Root->getMethods())
        if (M->isVirtual()) ++N;
    return N;
}

static std::pair<ClassDecl *, MethodDecl *>
resolveSlot(ClassDecl *CD, unsigned Slot) {
    for (auto *M : CD->getMethods())
        if (M->getVTableIndex() == Slot)
            return {CD, M};
    if (CD->getBase())
        return resolveSlot(CD->getBase(), Slot);
    return {nullptr, nullptr};
}

void tinyjs::emitClasses(Module *M, DeclList &Decls) {
    LLVMContext &Ctx = M->getContext();
    IRBuilder<> B(Ctx);

    // collect ClassDecls
    std::vector<ClassDecl *> Classes;
    for (auto *D : Decls)
        if (auto *CD = llvm::dyn_cast<ClassDecl>(D))
            Classes.push_back(CD);
    if (Classes.empty()) return;

    // debug infrastructure
    DIBuilder DBuilder(*M);
    DIFile *DbgFile = DBuilder.createFile("test3.js", ".");
    DICompileUnit *CU = DBuilder.createCompileUnit(
        dwarf::DW_LANG_C, DbgFile, "tinycompiler", false, "", 0);
    DIBasicType *DbgPtr = DBuilder.createBasicType("ptr", 64, dwarf::DW_ATE_address);

    // build struct types
    std::map<ClassDecl *, StructType *> StructTypes;
    for (auto *CD : Classes) {
        std::vector<FieldDecl *> AllFields;
        collectFields(CD, AllFields);
        std::vector<Type *> FieldTys = {B.getPtrTy()};
        for (size_t i = 0; i < AllFields.size(); ++i)
            FieldTys.push_back(B.getInt64Ty());
        StructTypes[CD] = StructType::create(Ctx, FieldTys, CD->getName());
    }

    // forward-declare all method functions
    FunctionType *MethodFTy = FunctionType::get(B.getPtrTy(), {B.getPtrTy()}, false);
    std::map<std::pair<ClassDecl *, MethodDecl *>, Function *> MethodFns;
    for (auto *CD : Classes) {
        for (auto *MD : CD->getMethods()) {
            std::string FnName = (CD->getName() + "_" + MD->getName()).str();
            Function *Fn = Function::Create(
                MethodFTy, Function::ExternalLinkage, FnName, M);
            Fn->getArg(0)->setName("this");
            MethodFns[{CD, MD}] = Fn;
        }
    }

    // vtable globals
    std::map<ClassDecl *, GlobalVariable *> VTables;
    for (auto *CD : Classes) {
        unsigned NSlots = vtableSize(CD);
        std::vector<Type *> SlotTys(NSlots, B.getPtrTy());
        std::vector<Constant *> Slots;
        for (unsigned i = 0; i < NSlots; ++i) {
            auto [Owner, MD] = resolveSlot(CD, i);
            Slots.push_back(MethodFns[{Owner, MD}]);
        }
        StructType *VTTy = StructType::get(Ctx, ArrayRef<Type *>(SlotTys));
        VTables[CD] = new GlobalVariable(
            *M, VTTy, true, GlobalValue::ExternalLinkage,
            ConstantStruct::get(VTTy, Slots),
            (CD->getName() + "_VTable").str());
    }

    // emit method bodies
    for (auto *CD : Classes) {
        for (auto *MD : CD->getMethods()) {
            Function *Fn = MethodFns[{CD, MD}];

            // debug: DISubprogram
            DISubroutineType *DbgTy = DBuilder.createSubroutineType(
                DBuilder.getOrCreateTypeArray({DbgPtr, DbgPtr}));
            DISubprogram *SP = DBuilder.createFunction(
                CU, (CD->getName() + "_" + MD->getName()).str(),
                Fn->getName(), DbgFile, 1, DbgTy, 1,
                DISubprogram::FlagPrototyped, DISubprogram::SPFlagDefinition);
            Fn->setSubprogram(SP);

            BasicBlock *BB = BasicBlock::Create(Ctx, "entry", Fn);
            B.SetInsertPoint(BB);
            B.SetCurrentDebugLocation(DILocation::get(Ctx, 1, 1, SP));

            // debug: declare 'this' parameter
            DILocalVariable *ThisVar = DBuilder.createParameterVariable(
                SP, "this", 1, DbgFile, 1, DbgPtr);
            DBuilder.insertDeclare(Fn->getArg(0), ThisVar,
                DBuilder.createExpression(),
                DILocation::get(Ctx, 1, 1, SP), BB);

            for (auto *S : MD->getBody()) {
                if (BB->getTerminator()) break;
                if (auto *RS = llvm::dyn_cast<ReturnStmt>(S)) {
                    if (auto *SL = llvm::dyn_cast_or_null<StringLiteralExpr>(RS->getValue()))
                        B.CreateRet(B.CreateGlobalString(SL->getValue(), "", 0, M));
                    else
                        B.CreateRet(ConstantPointerNull::get(PointerType::get(Ctx, 0)));
                    break;
                }
            }
            if (!BB->getTerminator())
                B.CreateRet(ConstantPointerNull::get(PointerType::get(Ctx, 0)));

            DBuilder.finalizeSubprogram(SP);
        }
    }

    // emit constructors
    FunctionType *InitFTy = FunctionType::get(B.getVoidTy(), {B.getPtrTy()}, false);
    for (auto *CD : Classes) {
        Function *Fn = Function::Create(
            InitFTy, Function::ExternalLinkage,
            (CD->getName() + "_init").str(), M);
        Fn->getArg(0)->setName("this");

        // debug
        DISubroutineType *DbgTy = DBuilder.createSubroutineType(
            DBuilder.getOrCreateTypeArray({nullptr, DbgPtr}));
        DISubprogram *SP = DBuilder.createFunction(
            CU, (CD->getName() + "_init").str(),
            Fn->getName(), DbgFile, 1, DbgTy, 1,
            DISubprogram::FlagPrototyped, DISubprogram::SPFlagDefinition);
        Fn->setSubprogram(SP);

        BasicBlock *BB = BasicBlock::Create(Ctx, "entry", Fn);
        B.SetInsertPoint(BB);
        B.SetCurrentDebugLocation(DILocation::get(Ctx, 1, 1, SP));

        StructType *STy = StructTypes[CD];
        // store vtable pointer at field 0
        Value *VPtr = B.CreateStructGEP(STy, Fn->getArg(0), 0, "vptr");
        B.CreateStore(VTables[CD], VPtr);

        std::vector<FieldDecl *> AllFields;
        collectFields(CD, AllFields);
        for (unsigned i = 0; i < AllFields.size(); ++i) {
            Value *FPtr = B.CreateStructGEP(STy, Fn->getArg(0), i + 1,
                AllFields[i]->getName());
            B.CreateStore(B.getInt64(AllFields[i]->getDefaultValue()), FPtr);
        }
        B.CreateRetVoid();
        DBuilder.finalizeSubprogram(SP);
    }

    // exception handling runtime declarations
    FunctionType *PersFty = FunctionType::get(B.getInt32Ty(), true);
    if (!M->getFunction("__gxx_personality_v0"))
        Function::Create(PersFty, Function::ExternalLinkage, "__gxx_personality_v0", M);
    if (!M->getFunction("__cxa_allocate_exception"))
        Function::Create(FunctionType::get(B.getPtrTy(), {B.getInt64Ty()}, false),
            Function::ExternalLinkage, "__cxa_allocate_exception", M);
    if (!M->getFunction("__cxa_throw"))
        Function::Create(FunctionType::get(B.getVoidTy(), {B.getPtrTy(), B.getPtrTy(), B.getPtrTy()}, false),
            Function::ExternalLinkage, "__cxa_throw", M);
    if (!M->getFunction("__cxa_begin_catch"))
        Function::Create(FunctionType::get(B.getPtrTy(), {B.getPtrTy()}, false),
            Function::ExternalLinkage, "__cxa_begin_catch", M);
    if (!M->getFunction("__cxa_end_catch"))
        Function::Create(FunctionType::get(B.getVoidTy(), {}, false),
            Function::ExternalLinkage, "__cxa_end_catch", M);
    if (!M->getFunction("puts"))
        Function::Create(FunctionType::get(B.getInt32Ty(), {B.getPtrTy()}, false),
            Function::ExternalLinkage, "puts", M);
    if (!M->getNamedGlobal("_ZTIi"))
        new GlobalVariable(*M, B.getPtrTy(), true,
            GlobalValue::ExternalLinkage, nullptr, "_ZTIi");

    DBuilder.finalize();
}
