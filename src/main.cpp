// main.cpp — Crow HTTP server for the CISC Instruction Format Analyzer.
// Serves the single-page frontend and exposes three JSON API endpoints.
//
// Endpoints:
//   GET /              -> web/index.html (HTML UI)
//   GET /api/instructions        -> full catalog (list)
//   GET /api/instruction/:id     -> single instruction with field detail
//   GET /api/comparison          -> variable-vs-fixed comparison data

// Force standalone ASIO mode — do NOT define CROW_USE_BOOST.
#ifdef CROW_USE_BOOST
#undef CROW_USE_BOOST
#endif
#define CROW_MAIN
#include "../third_party/crow/crow_all.h"

#include "api.h"
#include "comparison.h"
#include "instruction.h"

#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Read the frontend HTML from disk (next to the executable).
// We look in several candidate directories so the tool works both from the
// build directory and from the project root.
// ---------------------------------------------------------------------------
static std::string loadHtml() {
    const char* candidates[] = {
        "web/index.html",
        "../web/index.html",
        "../../web/index.html",
    };
    for (const char* path : candidates) {
        std::ifstream f(path, std::ios::binary);
        if (f) {
            std::ostringstream ss;
            ss << f.rdbuf();
            return ss.str();
        }
    }
    // Fallback — minimal page telling the user to run from the right directory.
    return "<!DOCTYPE html><html><body>"
           "<h2>Could not find web/index.html.</h2>"
           "<p>Run the server from the project root directory.</p>"
           "</body></html>";
}

int main() {
    // Build data (pure C++ — no Qt, no database).
    cisc::InstructionCatalog catalog;
    cisc::Comparison         comparison = cisc::buildComparison(catalog);

    std::string htmlPage = loadHtml();

    crow::SimpleApp app;

    // ------------------------------------------------------------------ //
    // GET /  — serve the interactive frontend
    // ------------------------------------------------------------------ //
    CROW_ROUTE(app, "/")(
        [&htmlPage]() {
            crow::response res(htmlPage);
            res.set_header("Content-Type", "text/html; charset=utf-8");
            return res;
        });

    // ------------------------------------------------------------------ //
    // GET /api/instructions  — full catalog (list of all instructions)
    // ------------------------------------------------------------------ //
    CROW_ROUTE(app, "/api/instructions")(
        [&catalog]() {
            crow::response res(cisc::catalogToJson(catalog));
            res.set_header("Content-Type", "application/json");
            res.set_header("Access-Control-Allow-Origin", "*");
            return res;
        });

    // ------------------------------------------------------------------ //
    // GET /api/instruction/<id>  — single instruction with field detail
    // ------------------------------------------------------------------ //
    CROW_ROUTE(app, "/api/instruction/<string>")(
        [&catalog](const std::string& id) {
            std::string json = cisc::instructionToJson(catalog, id);
            if (json.empty()) {
                crow::response res(404, "{\"error\":\"not found\"}");
                res.set_header("Content-Type", "application/json");
                return res;
            }
            crow::response res(json);
            res.set_header("Content-Type", "application/json");
            res.set_header("Access-Control-Allow-Origin", "*");
            return res;
        });

    // ------------------------------------------------------------------ //
    // GET /api/comparison  — variable-vs-fixed length data
    // ------------------------------------------------------------------ //
    CROW_ROUTE(app, "/api/comparison")(
        [&comparison]() {
            crow::response res(cisc::comparisonToJson(comparison));
            res.set_header("Content-Type", "application/json");
            res.set_header("Access-Control-Allow-Origin", "*");
            return res;
        });

    // ------------------------------------------------------------------ //
    // Start server on port 8080
    // ------------------------------------------------------------------ //
    std::cout << "CISC Instruction Format Analyzer\n";
    std::cout << "Server running at http://localhost:8080\n";
    std::cout << "Open that URL in Chrome or Edge to use the tool.\n";
    std::cout << "Press Ctrl+C to stop.\n\n";

    app.port(8080).multithreaded().run();
    return 0;
}
