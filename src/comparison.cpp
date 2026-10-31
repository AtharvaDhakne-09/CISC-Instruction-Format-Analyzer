#include "comparison.h"

namespace cisc {

Comparison buildComparison(const InstructionCatalog& catalog) {
    Comparison comparison;

    comparison.variableTitle = "Variable-length format (x86 CISC)";
    comparison.variableText =
        "In a variable-length instruction set such as x86, different "
        "instructions can occupy different numbers of bytes. In 32-bit mode "
        "an x86 instruction may be as short as 1 byte and as long as 15 "
        "bytes. The decoder reads prefixes, the opcode and any addressing or "
        "immediate fields one after another to discover where the instruction "
        "ends.";
    comparison.fixedTitle = "Fixed-length format (32-bit RISC-V RV32I)";
    comparison.fixedText =
        "In a fixed-length instruction set, every instruction in the base "
        "instruction set occupies the same number of bytes. The 32-bit "
        "RISC-V base integer ISA (RV32I) uses exactly one 32-bit word, or "
        "4 bytes, per instruction. The next instruction therefore always "
        "starts 4 bytes after the current one, which keeps fetch and decode "
        "simple and regular.";
    comparison.x86RangeNote =
        "The x86 instruction length limit in 32-bit mode is 15 bytes. The "
        "examples in this tool range from 1 to 7 bytes.";
    comparison.scopeNote =
        "Fixed length applies to the RV32I base integer ISA shown here. It "
        "does not mean that every RISC architecture is fixed-length: the "
        "optional RISC-V C extension adds 16-bit compressed instructions, "
        "and some other RISC ISAs use variable-length encodings.";
    comparison.fixedFieldsLabel =
        "32-bit R-type word for ADD x5, x6, x7 (bits 31 down to 0)";

    for (const auto& instruction : catalog.instructions()) {
        comparison.variableSamples.push_back(LengthSample{
            instruction.id,
            instruction.assembly,
            InstructionCatalog::bytesToHex(instruction.bytes),
            "",
            instruction.bytes.size(),
            "",
        });
    }

    comparison.fixedSamples = {
        {"riscv-add", "ADD x5, x6, x7", "B3 02 73 00", "0x007302B3", 4, "R-type"},
        {"riscv-addi", "ADDI x5, x6, 10", "93 02 A3 00", "0x00A30293", 4, "I-type"},
        {"riscv-lw", "LW x5, 8(x6)", "83 22 83 00", "0x00832283", 4, "I-type"},
        {"riscv-sub", "SUB x5, x6, x7", "B3 02 73 40", "0x407302B3", 4, "R-type"},
    };

    comparison.fixedFields = {
        {"funct7", "31:25", "0000000 (ADD)", 7},
        {"rs2", "24:20", "00111 (x7)", 5},
        {"rs1", "19:15", "00110 (x6)", 5},
        {"funct3", "14:12", "000 (ADD)", 3},
        {"rd", "11:7", "00101 (x5)", 5},
        {"opcode", "6:0", "0110011 (OP)", 7},
    };

    comparison.rows = {
        {"Instruction length",
         "1 to 15 bytes; the samples here are 1 to 7 bytes",
         "Always 4 bytes (32 bits) in RV32I"},
        {"Finding the next instruction",
         "Parse prefixes, opcode, ModR/M, SIB, displacement and immediate "
         "to learn the length",
         "Add 4 to the current address"},
        {"Encoding layout",
         "Optional prefixes followed by an opcode and optional addressing "
         "or operand fields",
         "One 32-bit word with fixed field positions (opcode, rd, funct3, "
         "rs1, rs2/funct7, immediate)"},
        {"Memory operands",
         "The addressing mode is encoded inside the instruction (ModR/M, "
         "SIB, displacement)",
         "Separate load/store instructions encode a base register and "
         "immediate offset; ALU instructions operate on registers"},
        {"Instruction alignment",
         "No alignment requirement; an instruction may start at any byte "
         "address",
         "Every base instruction starts on a 4-byte boundary"},
        {"Decode complexity",
         "Higher: the decoder examines fields sequentially and must handle "
         "many instruction forms",
         "Lower: fields sit at fixed positions and every instruction has "
         "the same stride"},
        {"Code density",
         "Shorter encodings are possible for common simple operations",
         "Every instruction costs the same 4 bytes"},
    };

    return comparison;
}

} // namespace cisc
