#include "crow_all.h"

#include "analyzer.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

crow::response serveFile(const fs::path& path, const std::string& contentType) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return crow::response(404, "File not found");
    }

    std::ostringstream contents;
    contents << input.rdbuf();

    crow::response response(200, contents.str());
    response.set_header("Content-Type", contentType);
    response.set_header("Cache-Control", "no-store");
    return response;
}

crow::response jsonResponse(const crow::json::wvalue& value, int status = 200) {
    crow::response response(status);
    response.set_header("Content-Type", "application/json; charset=utf-8");
    response.set_header("Cache-Control", "no-store");
    response.body = value.dump();
    return response;
}

crow::json::wvalue instructionListJson(const cisc::Analyzer& analyzer) {
    crow::json::wvalue out;
    out["mode"] = analyzer.mode();
    out["notice"] = analyzer.notice();
    out["count"] = static_cast<int>(analyzer.instructions().size());
    out["instructions"] = crow::json::wvalue::list();

    std::size_t index = 0;
    for (const auto& instruction : analyzer.instructions()) {
        auto& item = out["instructions"][index];
        item["id"] = instruction.id;
        item["assembly"] = instruction.assembly;
        item["hex"] = cisc::Analyzer::bytesToHex(instruction.bytes);
        item["length"] = static_cast<int>(instruction.bytes.size());
        ++index;
    }

    return out;
}

crow::json::wvalue instructionDetailJson(const cisc::Analyzer& analyzer,
                                         const cisc::Instruction& instruction) {
    crow::json::wvalue out;
    out["id"] = instruction.id;
    out["assembly"] = instruction.assembly;
    out["mode"] = analyzer.mode();
    out["hex"] = cisc::Analyzer::bytesToHex(instruction.bytes);
    out["length"] = static_cast<int>(instruction.bytes.size());
    out["note"] = instruction.note;

    out["bytes"] = crow::json::wvalue::list();
    for (std::size_t i = 0; i < instruction.bytes.size(); ++i) {
        out["bytes"][i] = cisc::Analyzer::byteHex(instruction.bytes[i]);
    }

    const auto fields = analyzer.fieldsFor(instruction);
    out["fields"] = crow::json::wvalue::list();
    for (std::size_t i = 0; i < fields.size(); ++i) {
        const auto& field = fields[i];
        auto& item = out["fields"][i];
        item["key"] = field.key;
        item["name"] = field.name;
        item["present"] = field.present;
        item["bytes"] = field.bytes;
        item["offset"] = field.present ? static_cast<int>(field.offset) : -1;
        item["length"] = static_cast<int>(field.length);
        item["purpose"] = field.purpose;
        item["detail"] = field.detail;
    }

    return out;
}

crow::json::wvalue comparisonJson(const cisc::Analyzer& analyzer) {
    const auto& comparison = analyzer.comparison();

    crow::json::wvalue out;
    out["variableTitle"] = comparison.variableTitle;
    out["variableText"] = comparison.variableText;
    out["fixedTitle"] = comparison.fixedTitle;
    out["fixedText"] = comparison.fixedText;
    out["x86RangeNote"] = comparison.x86RangeNote;
    out["scopeNote"] = comparison.scopeNote;
    out["fixedFieldsLabel"] = comparison.fixedFieldsLabel;

    out["variableSamples"] = crow::json::wvalue::list();
    for (std::size_t i = 0; i < comparison.variableSamples.size(); ++i) {
        const auto& sample = comparison.variableSamples[i];
        auto& item = out["variableSamples"][i];
        item["id"] = sample.id;
        item["label"] = sample.label;
        item["hex"] = sample.hex;
        item["word"] = sample.word;
        item["length"] = static_cast<int>(sample.length);
        item["format"] = sample.format;
    }

    out["fixedSamples"] = crow::json::wvalue::list();
    for (std::size_t i = 0; i < comparison.fixedSamples.size(); ++i) {
        const auto& sample = comparison.fixedSamples[i];
        auto& item = out["fixedSamples"][i];
        item["id"] = sample.id;
        item["label"] = sample.label;
        item["hex"] = sample.hex;
        item["word"] = sample.word;
        item["length"] = static_cast<int>(sample.length);
        item["format"] = sample.format;
    }

    out["fixedFields"] = crow::json::wvalue::list();
    for (std::size_t i = 0; i < comparison.fixedFields.size(); ++i) {
        const auto& field = comparison.fixedFields[i];
        auto& item = out["fixedFields"][i];
        item["name"] = field.name;
        item["bits"] = field.bits;
        item["value"] = field.value;
        item["width"] = field.width;
    }

    out["rows"] = crow::json::wvalue::list();
    for (std::size_t i = 0; i < comparison.rows.size(); ++i) {
        const auto& row = comparison.rows[i];
        auto& item = out["rows"][i];
        item["aspect"] = row.aspect;
        item["variable"] = row.variable;
        item["fixed"] = row.fixed;
    }

    return out;
}

fs::path findFrontendDir(const char* argv0) {
    std::vector<fs::path> candidates;

    if (const char* fromEnv = std::getenv("CISC_FRONTEND_DIR")) {
        candidates.emplace_back(fromEnv);
    }
#ifdef CISC_FRONTEND_DIR
    candidates.emplace_back(CISC_FRONTEND_DIR);
#endif
    candidates.emplace_back(fs::absolute(fs::path(__FILE__)).parent_path().parent_path() /
                            "frontend");
    candidates.emplace_back(fs::current_path() / "frontend");
    candidates.emplace_back(fs::current_path().parent_path() / "frontend");

    const fs::path executableDir = fs::absolute(fs::path(argv0)).parent_path();
    candidates.emplace_back(executableDir / "frontend");
    candidates.emplace_back(executableDir.parent_path() / "frontend");

    for (const auto& candidate : candidates) {
        std::error_code error;
        if (fs::exists(candidate / "index.html", error)) {
            return fs::weakly_canonical(candidate, error);
        }
    }

    return {};
}

} // namespace

int main(int argc, char** argv) {
    const fs::path frontendDir = findFrontendDir(argc > 0 ? argv[0] : "cisc_analyzer.exe");
    if (frontendDir.empty()) {
        std::cerr << "Could not locate the frontend directory.\n"
                  << "Run the program from the project root, place the 'frontend' "
                     "folder next to the executable, or set CISC_FRONTEND_DIR.\n";
        return 1;
    }

    const cisc::Analyzer analyzer;

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([frontendDir]() {
        return serveFile(frontendDir / "index.html", "text/html; charset=utf-8");
    });
    CROW_ROUTE(app, "/style.css")([frontendDir]() {
        return serveFile(frontendDir / "style.css", "text/css; charset=utf-8");
    });
    CROW_ROUTE(app, "/script.js")([frontendDir]() {
        return serveFile(frontendDir / "script.js", "application/javascript; charset=utf-8");
    });

    CROW_ROUTE(app, "/api/instructions")([&analyzer]() {
        return jsonResponse(instructionListJson(analyzer));
    });

    CROW_ROUTE(app, "/api/instructions/<string>")([&analyzer](const std::string& id) {
        const cisc::Instruction* instruction = analyzer.findInstruction(id);
        if (instruction == nullptr) {
            crow::json::wvalue error;
            error["error"] = "Instruction not found";
            error["id"] = id;
            return jsonResponse(error, 404);
        }
        return jsonResponse(instructionDetailJson(analyzer, *instruction));
    });

    CROW_ROUTE(app, "/api/comparison")([&analyzer]() {
        return jsonResponse(comparisonJson(analyzer));
    });

    app.loglevel(crow::LogLevel::Warning);
    app.bindaddr("127.0.0.1").port(8080).multithreaded();

    std::cout << "CISC Instruction Format Analyzer\n"
              << "Frontend directory: " << frontendDir.string() << "\n"
              << "Open http://localhost:8080 in your browser.\n"
              << "Press Ctrl+C to stop the server.\n";

    app.run();

    return 0;
}
