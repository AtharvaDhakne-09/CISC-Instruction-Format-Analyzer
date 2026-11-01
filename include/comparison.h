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
    std::size_t length = 0;
};

struct Comparison {
    std::string variableTitle;
    std::string variableText;
    std::string fixedTitle;
    std::string fixedText;
    std::string explanation;
    std::vector<LengthSample> variableSamples;
    std::vector<LengthSample> fixedSamples;
};

Comparison buildComparison(const InstructionCatalog& catalog);

} // namespace cisc
