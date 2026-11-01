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
    : mode_("32-bit x86, Intel syntax"),
      notice_("Four simple, verified 32-bit x86 examples.") {
    instructions_ = {
        {
            "push-eax",
            "PUSH EAX",
            "Put the value stored in EAX onto the stack.",
            {0x50},
            {
                layout(FieldKind::Opcode, 0, 1,
                       "50 is the opcode for PUSH EAX. The register EAX is "
                       "built into this byte, so no extra bytes are needed."),
            },
        },
        {
            "mov-eax-ebx",
            "MOV EAX, EBX",
            "Copy the value stored in EBX into EAX.",
            {0x89, 0xD8},
            {
                layout(FieldKind::Opcode, 0, 1,
                       "89 is the opcode for MOV. It tells the CPU to copy a "
                       "value."),
                layout(FieldKind::ModRM, 1, 1,
                       "D8 tells the CPU to use EBX as the source and EAX as "
                       "the destination."),
            },
        },
        {
            "mov-eax-ebx-disp8",
            "MOV EAX, [EBX+4]",
            "Find the memory address EBX + 4, read its value and copy it "
            "into EAX.",
            {0x8B, 0x43, 0x04},
            {
                layout(FieldKind::Opcode, 0, 1,
                       "8B is the opcode for MOV from memory into a register."),
                layout(FieldKind::ModRM, 1, 1,
                       "43 tells the CPU to use EBX as the address and EAX as "
                       "the destination. It also says that one displacement "
                       "byte follows."),
                layout(FieldKind::Displacement, 2, 1,
                       "04 is added to EBX, so the memory address is "
                       "EBX + 4."),
            },
        },
        {
            "add-eax-imm32",
            "ADD EAX, 0x12345678",
            "Add the given number to the value stored in EAX.",
            {0x05, 0x78, 0x56, 0x34, 0x12},
            {
                layout(FieldKind::Opcode, 0, 1,
                       "05 is the opcode for ADD with EAX. It tells the CPU "
                       "to add a number to EAX."),
                layout(FieldKind::Immediate, 1, 4,
                       "This is the number 0x12345678. It is stored smallest "
                       "byte first, so the bytes are 78 56 34 12."),
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
        FieldKind::Opcode,
        FieldKind::ModRM,
        FieldKind::Displacement,
        FieldKind::Immediate,
    };

    std::vector<InstructionField> result;
    result.reserve(instruction.layout.size());

    for (const FieldKind kind : order) {
        const auto it = std::find_if(
            instruction.layout.begin(), instruction.layout.end(),
            [kind](const FieldLayout& candidate) { return candidate.kind == kind; });

        if (it == instruction.layout.end()) {
            continue;
        }

        InstructionField field{};
        field.kind = kind;
        field.key = fieldKey(kind);
        field.name = fieldName(kind);
        field.purpose = fieldPurpose(kind);
        field.offset = it->offset;
        field.length = it->length;
        field.present = true;
        field.detail = it->detail;

        std::ostringstream hex;
        for (std::size_t i = 0; i < it->length; ++i) {
            if (i > 0) {
                hex << ' ';
            }
            hex << byteHex(instruction.bytes[it->offset + i]);
        }
        field.bytes = hex.str();

        result.push_back(field);
    }

    return result;
}

std::string InstructionCatalog::fieldKey(FieldKind kind) {
    switch (kind) {
    case FieldKind::Opcode:
        return "opcode";
    case FieldKind::ModRM:
        return "modrm";
    case FieldKind::Displacement:
        return "displacement";
    case FieldKind::Immediate:
        return "immediate";
    }
    return "unknown";
}

std::string InstructionCatalog::fieldName(FieldKind kind) {
    switch (kind) {
    case FieldKind::Opcode:
        return "Opcode";
    case FieldKind::ModRM:
        return "ModR/M";
    case FieldKind::Displacement:
        return "Displacement";
    case FieldKind::Immediate:
        return "Immediate";
    }
    return "Unknown field";
}

std::string InstructionCatalog::fieldPurpose(FieldKind kind) {
    switch (kind) {
    case FieldKind::Opcode:
        return "Tells the CPU what operation to perform.";
    case FieldKind::ModRM:
        return "Helps identify the registers or memory to use.";
    case FieldKind::Displacement:
        return "An extra number used to find a memory location.";
    case FieldKind::Immediate:
        return "A number given directly inside the instruction.";
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
