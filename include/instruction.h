#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace cisc {

enum class FieldKind {
    Prefix,
    Opcode,
    ModRM,
    SIB,
    Displacement,
    Immediate
};

struct FieldLayout {
    FieldKind kind = FieldKind::Opcode;
    std::size_t offset = 0;
    std::size_t length = 0;
    std::string detail;
};

struct InstructionField {
    FieldKind kind = FieldKind::Opcode;
    bool present = false;
    std::string key;
    std::string name;
    std::string bytes;
    std::size_t offset = 0;
    std::size_t length = 0;
    std::string purpose;
    std::string detail;
};

struct Instruction {
    std::string id;
    std::string assembly;
    std::string note;
    std::vector<std::uint8_t> bytes;
    std::vector<FieldLayout> layout;
};

class InstructionCatalog {
public:
    InstructionCatalog();

    const std::string& mode() const { return mode_; }
    const std::string& notice() const { return notice_; }
    const std::vector<Instruction>& instructions() const { return instructions_; }

    const Instruction* find(const std::string& id) const;
    std::vector<InstructionField> fieldsFor(const Instruction& instruction) const;

    static std::string fieldKey(FieldKind kind);
    static std::string fieldName(FieldKind kind);
    static std::string fieldPurpose(FieldKind kind);
    static std::string absentDetail(FieldKind kind);
    static std::string byteHex(std::uint8_t value);
    static std::string bytesToHex(const std::vector<std::uint8_t>& bytes);

private:
    void validate(const Instruction& instruction) const;

    std::string mode_;
    std::string notice_;
    std::vector<Instruction> instructions_;
    std::map<std::string, std::size_t> index_;
};

} // namespace cisc
