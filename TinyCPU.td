// TinyCPU ISA definition.
// Each instruction has a mnemonic, binary opcode, and operand count.

class Inst<string mnemonic, int opcode, int numops> {
    string Mnemonic = mnemonic;
    int    Opcode   = opcode;
    int    NumOps   = numops;
}

// generates a register variant and an immediate variant (suffix "i", opcode+1)
multiclass InstWithImm<string mnemonic, int opcode, int numops> {
    def "":  Inst<mnemonic,                  opcode,           numops>;
    def "I": Inst<!strconcat(mnemonic, "i"), !add(opcode, 1),  numops>;
}

defm ADD : InstWithImm<"add", 0xA0, 3>;
defm SUB : InstWithImm<"sub", 0xB0, 3>;
defm MUL : InstWithImm<"mul", 0xC0, 3>;

def LD  : Inst<"ld",  0xD0, 2>;
def ST  : Inst<"st",  0xD1, 2>;
def BR  : Inst<"br",  0xE0, 1>;
def RET : Inst<"ret", 0xFF, 0>;
