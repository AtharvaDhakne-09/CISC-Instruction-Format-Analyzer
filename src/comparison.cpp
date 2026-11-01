#include "comparison.h"

namespace cisc {

Comparison buildComparison(const InstructionCatalog& catalog) {
    Comparison comparison;

    comparison.variableTitle = "CISC (x86): different sizes";
    comparison.variableText =
        "These four x86 instructions are 1, 2, 3 and 5 bytes long.";
    comparison.fixedTitle = "RISC-V (RV32I): same size";
    comparison.fixedText =
        "Every example here is exactly 4 bytes long.";
    comparison.explanation =
        "CISC instructions can have different sizes. In the RV32I base "
        "instruction set, every standard instruction has the same size of "
        "4 bytes.";

    for (const auto& instruction : catalog.instructions()) {
        comparison.variableSamples.push_back(LengthSample{
            instruction.id,
            instruction.assembly,
            InstructionCatalog::bytesToHex(instruction.bytes),
            instruction.bytes.size(),
        });
    }

    comparison.fixedSamples = {
        {"riscv-add", "ADD x5, x6, x7", "B3 02 73 00", 4},
        {"riscv-addi", "ADDI x5, x6, 10", "93 02 A3 00", 4},
        {"riscv-lw", "LW x5, 8(x6)", "83 22 83 00", 4},
    };

    return comparison;
}

} // namespace cisc
