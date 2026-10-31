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

struct LengthSample {
    std::string id;
    std::string label;
    std::string hex;
    std::string word;
    std::size_t length = 0;
    std::string format;
};

struct RiscvField {
    std::string name;
    std::string bits;
    std::string value;
    int width = 0;
};

struct ComparisonRow {
    std::string aspect;
    std::string variable;
    std::string fixed;
};

struct Comparison {
    std::string variableTitle;
    std::string variableText;
    std::string fixedTitle;
    std::string fixedText;
    std::string x86RangeNote;
    std::string scopeNote;
    std::string fixedFieldsLabel;
    std::vector<LengthSample> variableSamples;
    std::vector<LengthSample> fixedSamples;
    std::vector<RiscvField> fixedFields;
    std::vector<ComparisonRow> rows;
};

class Analyzer {
public:
    Analyzer();

    const std::string& mode() const { return mode_; }
    const std::string& notice() const { return notice_; }
    const std::vector<Instruction>& instructions() const { return instructions_; }
    const Comparison& comparison() const { return comparison_; }

    const Instruction* findInstruction(const std::string& id) const;
    std::vector<InstructionField> fieldsFor(const Instruction& instruction) const;

    static std::string fieldKey(FieldKind kind);
    static std::string fieldName(FieldKind kind);
    static std::string fieldPurpose(FieldKind kind);
    static std::string absentDetail(FieldKind kind);
    static std::string byteHex(std::uint8_t value);
    static std::string bytesToHex(const std::vector<std::uint8_t>& bytes);

private:
    void validate(const Instruction& instruction) const;
    void buildComparison();

    std::string mode_;
    std::string notice_;
    std::vector<Instruction> instructions_;
    std::map<std::string, std::size_t> index_;
    Comparison comparison_;
};

} // namespace cisc
