#include "instruction.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace cisc {

namespace {

FieldLayout layout(FieldKind kind, std::size_t offset, std::size_t length,
                   const char* detail) {
    return FieldLayout{kind, offset, length, detail};
}

} // namespace

InstructionCatalog::InstructionCatalog()
    : mode_("32-bit x86 (IA-32), Intel syntax"),
      notice_("Predefined, verified 32-bit x86 examples only. "
              "This tool does not decode arbitrary byte sequences.") {
    instructions_ = {
        {
            "push-eax",
            "PUSH EAX",
            "In 32-bit mode, PUSH EAX subtracts 4 from ESP and stores the "
            "full 32-bit value of EAX at the new top of the stack.",
            {0x50},
            {
                layout(FieldKind::Opcode, 0, 1,
                       "50 is the one-byte PUSH opcode. The low three bits "
                       "(000) select EAX, so the register is encoded inside "
                       "the opcode itself."),
            },
        },
        {
            "mov-eax-ebx",
            "MOV EAX, EBX",
            "Both operands are registers, so the move is encoded by the "
            "opcode and a single ModR/M byte.",
            {0x89, 0xD8},
            {
                layout(FieldKind::Opcode, 0, 1,
                       "89 is the opcode for MOV r/m32, r32: move a 32-bit "
                       "register into a register or memory destination."),
                layout(FieldKind::ModRM, 1, 1,
                       "D8 = 11 011 000. mod=11 selects a register operand, "
                       "reg=011 is EBX (the source), and r/m=000 is EAX "
                       "(the destination)."),
            },
        },
        {
            "mov-eax-fs-ebx",
            "MOV EAX, DWORD PTR FS:[EBX]",
            "The FS segment override changes the segment used for the "
            "memory operand, so the effective address is FS:EBX.",
            {0x64, 0x8B, 0x03},
            {
                layout(FieldKind::Prefix, 0, 1,
                       "64 is the FS segment-override prefix. It makes the "
                       "memory operand use the FS segment instead of the "
                       "default DS segment."),
                layout(FieldKind::Opcode, 1, 1,
                       "8B is MOV r32, r/m32: load a 32-bit register from a "
                       "register or memory operand."),
                layout(FieldKind::ModRM, 2, 1,
                       "03 = 00 000 011. mod=00 selects a memory operand "
                       "with no displacement, reg=000 is EAX (the "
                       "destination), and r/m=011 is EBX (the base register)."),
            },
        },
        {
            "mov-eax-ebx-disp8",
            "MOV EAX, DWORD PTR [EBX+4]",
            "The ModR/M byte tells the CPU both the destination register "
            "and that a one-byte displacement follows.",
            {0x8B, 0x43, 0x04},
            {
                layout(FieldKind::Opcode, 0, 1,
                       "8B is MOV r32, r/m32: load a 32-bit register from a "
                       "register or memory operand."),
                layout(FieldKind::ModRM, 1, 1,
                       "43 = 01 000 011. mod=01 means a one-byte "
                       "displacement follows, reg=000 is EAX (the "
                       "destination), and r/m=011 is EBX (the base register)."),
                layout(FieldKind::Displacement, 2, 1,
                       "04 is an 8-bit displacement. It is sign-extended to "
                       "32 bits and added to EBX, giving the address EBX + 4."),
            },
        },
        {
            "add-eax-imm8",
            "ADD EAX, 5",
            "A one-byte immediate keeps this 32-bit addition short. The "
            "immediate is sign-extended to the operand size.",
            {0x83, 0xC0, 0x05},
            {
                layout(FieldKind::Opcode, 0, 1,
                       "83 is the arithmetic opcode for ADD r/m32, imm8. "
                       "The /0 digit in the ModR/M byte selects ADD."),
                layout(FieldKind::ModRM, 1, 1,
                       "C0 = 11 000 000. mod=11 selects a register "
                       "destination, reg=000 (/0 selects ADD), and r/m=000 "
                       "is EAX (the destination)."),
                layout(FieldKind::Immediate, 2, 1,
                       "05 is the immediate operand. Opcode 83 sign-extends "
                       "this 8-bit value to 32 bits, so it is added as "
                       "0x00000005."),
            },
        },
        {
            "mov-eax-sib-disp8",
            "MOV EAX, DWORD PTR [EBX+ECX*4+8]",
            "The SIB byte is needed because ModR/M r/m=100 indicates a "
            "base-plus-scaled-index addressing form.",
            {0x8B, 0x44, 0x8B, 0x08},
            {
                layout(FieldKind::Opcode, 0, 1,
                       "8B is MOV r32, r/m32: load a 32-bit register from a "
                       "register or memory operand."),
                layout(FieldKind::ModRM, 1, 1,
                       "44 = 01 000 100. mod=01 means a one-byte "
                       "displacement follows, reg=000 is EAX (the "
                       "destination), and r/m=100 signals that a SIB byte "
                       "follows."),
                layout(FieldKind::SIB, 2, 1,
                       "8B = 10 001 011. scale=10 multiplies the index by 4, "
                       "index=001 is ECX, and base=011 is EBX. The address is "
                       "EBX + ECX*4 + displacement."),
                layout(FieldKind::Displacement, 3, 1,
                       "08 is the 8-bit displacement, sign-extended and added "
                       "to the base-plus-index address: EBX + ECX*4 + 8."),
            },
        },
        {
            "add-eax-imm32",
            "ADD EAX, 0x12345678",
            "This accumulator form encodes a full 32-bit constant directly "
            "after the opcode.",
            {0x05, 0x78, 0x56, 0x34, 0x12},
            {
                layout(FieldKind::Opcode, 0, 1,
                       "05 is the short-form opcode for ADD EAX, imm32. The "
                       "accumulator EAX is implicit, so no ModR/M byte is "
                       "needed."),
                layout(FieldKind::Immediate, 1, 4,
                       "The 32-bit immediate is stored least-significant byte "
                       "first (little-endian): 78 56 34 12 represents "
                       "0x12345678."),
            },
        },
        {
            "mov-eax-sib-disp32",
            "MOV EAX, DWORD PTR [EBX+ECX*4+0x12345678]",
            "This 7-byte encoding combines every optional addressing field "
            "except a prefix and an immediate.",
            {0x8B, 0x84, 0x8B, 0x78, 0x56, 0x34, 0x12},
            {
                layout(FieldKind::Opcode, 0, 1,
                       "8B is MOV r32, r/m32: load a 32-bit register from a "
                       "register or memory operand."),
                layout(FieldKind::ModRM, 1, 1,
                       "84 = 10 000 100. mod=10 means a 32-bit displacement "
                       "follows, reg=000 is EAX (the destination), and "
                       "r/m=100 signals that a SIB byte follows."),
                layout(FieldKind::SIB, 2, 1,
                       "8B = 10 001 011. scale=10 multiplies the index by 4, "
                       "index=001 is ECX, and base=011 is EBX. The address is "
                       "EBX + ECX*4 + displacement."),
                layout(FieldKind::Displacement, 3, 4,
                       "78 56 34 12 is a 32-bit displacement in little-endian "
                       "byte order: 0x12345678. It is added to EBX + ECX*4."),
            },
        },
        {
            "lock-add-ebx-eax",
            "LOCK ADD DWORD PTR [EBX], EAX",
            "LOCK ADD DWORD PTR [EBX], EAX performs an atomic "
            "read-modify-write on the dword at [EBX].",
            {0xF0, 0x01, 0x03},
            {
                layout(FieldKind::Prefix, 0, 1,
                       "F0 is the LOCK prefix. It makes the following "
                       "read-modify-write instruction atomic with respect to "
                       "other processors."),
                layout(FieldKind::Opcode, 1, 1,
                       "01 is ADD r/m32, r32: add a 32-bit register to a "
                       "register or memory destination."),
                layout(FieldKind::ModRM, 2, 1,
                       "03 = 00 000 011. mod=00 selects a memory destination "
                       "with no displacement, reg=000 is EAX (the source), "
                       "and r/m=011 is EBX (the destination address)."),
            },
        },
    };

    for (std::size_t i = 0; i < instructions_.size(); ++i) {
        validate(instructions_[i]);
        index_[instructions_[i].id] = i;
    }
}

void InstructionCatalog::validate(const Instruction& instruction) const {
    if (instruction.bytes.empty()) {
        throw std::runtime_error("Instruction '" + instruction.id + "' has no bytes");
    }
    if (instruction.layout.empty()) {
        throw std::runtime_error("Instruction '" + instruction.id + "' has no field layout");
    }

    auto sorted = instruction.layout;
    std::sort(sorted.begin(), sorted.end(), [](const FieldLayout& a, const FieldLayout& b) {
        return a.offset < b.offset;
    });

    std::size_t expected = 0;
    for (const auto& field : sorted) {
        if (field.offset != expected) {
            throw std::runtime_error("Instruction '" + instruction.id +
                                     "' has a gap or overlap in its field layout");
        }
        if (field.length == 0 ||
            field.offset + field.length > instruction.bytes.size()) {
            throw std::runtime_error("Instruction '" + instruction.id +
                                     "' has a field outside its byte sequence");
        }
        expected += field.length;
    }

    if (expected != instruction.bytes.size()) {
        throw std::runtime_error("Instruction '" + instruction.id +
                                 "' has fields that do not cover every byte");
    }
}

const Instruction* InstructionCatalog::find(const std::string& id) const {
    const auto it = index_.find(id);
    if (it == index_.end()) {
        return nullptr;
    }
    return &instructions_[it->second];
}

std::vector<InstructionField> InstructionCatalog::fieldsFor(
    const Instruction& instruction) const {
    static const FieldKind order[] = {
        FieldKind::Prefix,
        FieldKind::Opcode,
        FieldKind::ModRM,
        FieldKind::SIB,
        FieldKind::Displacement,
        FieldKind::Immediate,
    };

    std::vector<InstructionField> result;
    result.reserve(6);

    for (const FieldKind kind : order) {
        InstructionField field{};
        field.kind = kind;
        field.key = fieldKey(kind);
        field.name = fieldName(kind);
        field.purpose = fieldPurpose(kind);
        field.offset = 0;
        field.length = 0;
        field.present = false;

        const auto it = std::find_if(
            instruction.layout.begin(), instruction.layout.end(),
            [kind](const FieldLayout& candidate) { return candidate.kind == kind; });

        if (it == instruction.layout.end()) {
            field.detail = absentDetail(kind);
        } else {
            field.present = true;
            field.offset = it->offset;
            field.length = it->length;
            field.detail = it->detail;

            std::ostringstream hex;
            for (std::size_t i = 0; i < it->length; ++i) {
                if (i > 0) {
                    hex << ' ';
                }
                hex << byteHex(instruction.bytes[it->offset + i]);
            }
            field.bytes = hex.str();
        }

        result.push_back(field);
    }

    return result;
}

std::string InstructionCatalog::fieldKey(FieldKind kind) {
    switch (kind) {
    case FieldKind::Prefix:
        return "prefix";
    case FieldKind::Opcode:
        return "opcode";
    case FieldKind::ModRM:
        return "modrm";
    case FieldKind::SIB:
        return "sib";
    case FieldKind::Displacement:
        return "displacement";
    case FieldKind::Immediate:
        return "immediate";
    }
    return "unknown";
}

std::string InstructionCatalog::fieldName(FieldKind kind) {
    switch (kind) {
    case FieldKind::Prefix:
        return "Prefix";
    case FieldKind::Opcode:
        return "Opcode";
    case FieldKind::ModRM:
        return "ModR/M byte";
    case FieldKind::SIB:
        return "SIB byte";
    case FieldKind::Displacement:
        return "Displacement";
    case FieldKind::Immediate:
        return "Immediate data";
    }
    return "Unknown field";
}

std::string InstructionCatalog::fieldPurpose(FieldKind kind) {
    switch (kind) {
    case FieldKind::Prefix:
        return "Optional bytes that modify how the instruction behaves, such "
               "as LOCK, REP or a segment override.";
    case FieldKind::Opcode:
        return "Tells the CPU which operation to perform and may also encode "
               "a register operand.";
    case FieldKind::ModRM:
        return "One byte that specifies the register operands and, for memory "
               "operands, the addressing form (mod, reg, r/m).";
    case FieldKind::SIB:
        return "One byte used with ModR/M when the address has the form "
               "[base + index x scale].";
    case FieldKind::Displacement:
        return "A signed offset added to the computed address (or to the "
               "instruction pointer for branches).";
    case FieldKind::Immediate:
        return "A constant value encoded directly inside the instruction.";
    }
    return "";
}

std::string InstructionCatalog::absentDetail(FieldKind kind) {
    switch (kind) {
    case FieldKind::Prefix:
        return "Absent. This instruction uses no legacy prefix bytes.";
    case FieldKind::Opcode:
        return "Absent. An instruction always has an opcode, so this should "
               "not occur.";
    case FieldKind::ModRM:
        return "Absent. This encoding identifies its operands without a "
               "ModR/M byte.";
    case FieldKind::SIB:
        return "Absent. The addressing mode does not use a scaled index, so "
               "no SIB byte is needed.";
    case FieldKind::Displacement:
        return "Absent. No displacement is added to an address by this "
               "instruction.";
    case FieldKind::Immediate:
        return "Absent. This instruction uses no constant operand.";
    }
    return "";
}

std::string InstructionCatalog::byteHex(std::uint8_t value) {
    std::ostringstream out;
    out << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(value);
    return out.str();
}

std::string InstructionCatalog::bytesToHex(const std::vector<std::uint8_t>& bytes) {
    std::ostringstream out;
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        if (i > 0) {
            out << ' ';
        }
        out << byteHex(bytes[i]);
    }
    return out.str();
}

} // namespace cisc
