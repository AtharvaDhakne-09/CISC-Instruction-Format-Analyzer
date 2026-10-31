#pragma once

#include "instruction.h"

#include <cstddef>
#include <string>
#include <vector>

namespace cisc {

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

Comparison buildComparison(const InstructionCatalog& catalog);

} // namespace cisc
