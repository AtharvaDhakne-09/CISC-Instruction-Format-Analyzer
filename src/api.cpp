#include "api.h"

#include <sstream>

namespace cisc {

// ---------------------------------------------------------------------------
// Minimal JSON helpers
// ---------------------------------------------------------------------------

namespace {

std::string jsonStr(const std::string& s) {
    std::ostringstream o;
    o << '"';
    for (char c : s) {
        switch (c) {
        case '"':  o << "\\\""; break;
        case '\\': o << "\\\\"; break;
        case '\n': o << "\\n";  break;
        case '\r': o << "\\r";  break;
        case '\t': o << "\\t";  break;
        default:   o << c;      break;
        }
    }
    o << '"';
    return o.str();
}

std::string jsonNum(std::size_t n) {
    return std::to_string(n);
}

std::string jsonBool(bool b) {
    return b ? "true" : "false";
}

} // namespace

// ---------------------------------------------------------------------------
// catalogToJson
// ---------------------------------------------------------------------------

std::string catalogToJson(const InstructionCatalog& catalog) {
    std::ostringstream o;
    o << "{";
    o << "\"mode\":" << jsonStr(catalog.mode()) << ",";
    o << "\"notice\":" << jsonStr(catalog.notice()) << ",";
    o << "\"instructions\":[";

    const auto& instructions = catalog.instructions();
    for (std::size_t i = 0; i < instructions.size(); ++i) {
        if (i > 0) o << ",";
        const auto& instr = instructions[i];
        o << "{";
        o << "\"id\":" << jsonStr(instr.id) << ",";
        o << "\"assembly\":" << jsonStr(instr.assembly) << ",";
        o << "\"note\":" << jsonStr(instr.note) << ",";
        o << "\"hex\":" << jsonStr(InstructionCatalog::bytesToHex(instr.bytes)) << ",";
        o << "\"length\":" << jsonNum(instr.bytes.size());
        o << "}";
    }

    o << "]}";
    return o.str();
}

// ---------------------------------------------------------------------------
// instructionToJson
// ---------------------------------------------------------------------------

std::string instructionToJson(const InstructionCatalog& catalog,
                               const std::string& id) {
    const Instruction* instr = catalog.find(id);
    if (!instr) return "";

    std::ostringstream o;
    o << "{";
    o << "\"id\":" << jsonStr(instr->id) << ",";
    o << "\"assembly\":" << jsonStr(instr->assembly) << ",";
    o << "\"note\":" << jsonStr(instr->note) << ",";
    o << "\"hex\":" << jsonStr(InstructionCatalog::bytesToHex(instr->bytes)) << ",";
    o << "\"length\":" << jsonNum(instr->bytes.size()) << ",";

    // Individual bytes array
    o << "\"bytes\":[";
    for (std::size_t i = 0; i < instr->bytes.size(); ++i) {
        if (i > 0) o << ",";
        o << jsonStr(InstructionCatalog::byteHex(instr->bytes[i]));
    }
    o << "],";

    // Fields array
    const auto fields = catalog.fieldsFor(*instr);
    o << "\"fields\":[";
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) o << ",";
        const auto& f = fields[i];
        o << "{";
        o << "\"key\":" << jsonStr(f.key) << ",";
        o << "\"name\":" << jsonStr(f.name) << ",";
        o << "\"present\":" << jsonBool(f.present) << ",";
        o << "\"bytes\":" << jsonStr(f.bytes) << ",";
        o << "\"offset\":" << jsonNum(f.offset) << ",";
        o << "\"length\":" << jsonNum(f.length) << ",";
        o << "\"purpose\":" << jsonStr(f.purpose) << ",";
        o << "\"detail\":" << jsonStr(f.detail);
        o << "}";
    }
    o << "]";

    o << "}";
    return o.str();
}

// ---------------------------------------------------------------------------
// comparisonToJson
// ---------------------------------------------------------------------------

std::string comparisonToJson(const Comparison& cmp) {
    std::ostringstream o;
    o << "{";
    o << "\"variableTitle\":" << jsonStr(cmp.variableTitle) << ",";
    o << "\"variableText\":" << jsonStr(cmp.variableText) << ",";
    o << "\"fixedTitle\":" << jsonStr(cmp.fixedTitle) << ",";
    o << "\"fixedText\":" << jsonStr(cmp.fixedText) << ",";
    o << "\"explanation\":" << jsonStr(cmp.explanation) << ",";

    // Variable samples
    o << "\"variableSamples\":[";
    for (std::size_t i = 0; i < cmp.variableSamples.size(); ++i) {
        if (i > 0) o << ",";
        const auto& s = cmp.variableSamples[i];
        o << "{";
        o << "\"id\":" << jsonStr(s.id) << ",";
        o << "\"label\":" << jsonStr(s.label) << ",";
        o << "\"hex\":" << jsonStr(s.hex) << ",";
        o << "\"length\":" << jsonNum(s.length);
        o << "}";
    }
    o << "],";

    // Fixed samples
    o << "\"fixedSamples\":[";
    for (std::size_t i = 0; i < cmp.fixedSamples.size(); ++i) {
        if (i > 0) o << ",";
        const auto& s = cmp.fixedSamples[i];
        o << "{";
        o << "\"id\":" << jsonStr(s.id) << ",";
        o << "\"label\":" << jsonStr(s.label) << ",";
        o << "\"hex\":" << jsonStr(s.hex) << ",";
        o << "\"length\":" << jsonNum(s.length);
        o << "}";
    }
    o << "]";

    o << "}";
    return o.str();
}

} // namespace cisc

