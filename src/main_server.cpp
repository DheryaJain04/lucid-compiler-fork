// ============================================================================
// main_server.cpp — local playground server for Lucid (`lucid-server`).
// A thin wrapper over the compiler core using cpp-httplib. Serves the static
// web/ playground and exposes POST /compile. All compiler work stays in pure
// C++; the browser only renders results and (optionally) calls Gemini itself.
// ============================================================================
#include <iostream>
#include <string>

// A browser keeps several persistent (keep-alive) connections open at once. With
// cpp-httplib's small default thread pool, those idle connections can occupy
// every worker and the page appears to "load forever" — even though curl (one
// short connection) works fine. Give the server plenty of worker threads so
// request handling can never be starved. Must be defined before httplib.h.
#define CPPHTTPLIB_THREAD_POOL_COUNT 64

#include "httplib.h"
#include "../third_party/json.hpp"
#include "compiler.hpp"

#ifndef LUCID_WEB_DIR
#define LUCID_WEB_DIR "web"
#endif

int main(int argc, char** argv) {
    int port = 8080;
    if (argc > 1) {
        try { port = std::stoi(argv[1]); } catch (...) { port = 8080; }
    } else if (const char* env = std::getenv("PORT")) {
        try { port = std::stoi(env); } catch (...) {}
    }

    httplib::Server server;

    // Serve the playground (index.html, style.css, app.js, examples.js).
    const std::string webDir = LUCID_WEB_DIR;
    if (!server.set_mount_point("/", webDir)) {
        std::cerr << "warning: could not mount web dir '" << webDir
                  << "' — static files will not be served.\n";
    }

    // Small CORS courtesy so the page also works if opened from a file:// origin.
    server.set_default_headers({{"Access-Control-Allow-Origin", "*"},
                                {"Access-Control-Allow-Headers", "Content-Type"},
                                {"Access-Control-Allow-Methods", "POST, OPTIONS"}});
    server.Options("/compile", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });

    // POST /compile { "source": "..." } -> full compiler result JSON.
    server.Post("/compile", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json out;
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string source = body.value("source", "");
            out = lucid::compile(source);
        } catch (const std::exception& e) {
            // A compiler crash or bad request returns a clean error, never a 500 kill.
            out = {
                {"tokens", nlohmann::json::array()},
                {"ast", nullptr},
                {"symbolTable", nullptr},
                {"optimized", nullptr},
                {"output", nlohmann::json::array()},
                {"success", false},
                {"errors", nlohmann::json::array({{
                    {"phase", "server"}, {"type", "InternalError"},
                    {"message", std::string("Compiler failure: ") + e.what()},
                    {"line", 0}, {"col", 0}, {"snippet", ""}
                }})}
            };
        }
        res.set_content(out.dump(), "application/json");
    });

    // Simple health check.
    server.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    std::cout << "Lucid playground running at  http://localhost:" << port << "\n";
    std::cout << "Serving web assets from      " << webDir << "\n";
    std::cout << "Press Ctrl+C to stop.\n";

    if (!server.listen("0.0.0.0", port)) {
        std::cerr << "error: could not bind to port " << port
                  << " (is it already in use?)\n";
        return 1;
    }
    return 0;
}
