#pragma once

#include "comparison.h"
#include "instruction.h"

#include <string>

namespace cisc {

// Serialise the full instruction catalog to a JSON string.
std::string catalogToJson(const InstructionCatalog& catalog);

// Serialise a single instruction (with its fields) to JSON.
// Returns an empty string if the id is not found.
std::string instructionToJson(const InstructionCatalog& catalog,
                               const std::string& id);

// Serialise the variable-vs-fixed comparison to a JSON string.
std::string comparisonToJson(const Comparison& comparison);

} // namespace cisc

