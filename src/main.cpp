#include "checker.hpp"
#include "codegen.hpp"
#include "evaluator.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#ifndef XELANG_RUNTIME_DIR
#define XELANG_RUNTIME_DIR "src"
#endif
#ifndef XELANG_CXX_COMPILER
#define XELANG_CXX_COMPILER "c++"
#endif

namespace {
struct SourceUnit {
    std::filesystem::path path;
    std::string text;
    xe::Program parsed;
};

std::string shellQuote(const std::string& value) {
#if defined(_WIN32)
    std::string quoted = "\"";
    std::size_t backslashes = 0;
    for (char character : value) {
        if (character == '\\') { ++backslashes; continue; }
        if (character == '"') {
            quoted.append(backslashes * 2 + 1, '\\');
            quoted += '"';
        } else {
            quoted.append(backslashes, '\\');
            quoted += character;
        }
        backslashes = 0;
    }
    quoted.append(backslashes * 2, '\\');
    return quoted + '"';
#else
    std::string quoted = "'";
    for (char c : value) { if (c == '\'') quoted += "'\\''"; else quoted += c; }
    return quoted + "'";
#endif
}
void help(std::ostream& output) {
    output << "Xe compiler\n\n"
              "Usage:\n"
              "  xe --run <file.xe> [--library-path <directory>] [-- arguments...]\n"
              "  xe --build <file.xe> [file.xe ...] [--library-path <directory>] -o <program.xee>\n\n"
              "Options:\n"
              "  -h, --help       Show this help\n"
              "  -V, --version    Show compiler version\n";
}

void usage() {
    help(std::cerr);
}

std::filesystem::path resolveLibrary(const std::string& name,
                                     const std::filesystem::path& importer,
                                     const std::vector<std::filesystem::path>& libraryPaths) {
    std::filesystem::path modulePath;
    for (char character : name) modulePath += character == '.' ? '/' : character;
    auto filename = modulePath;
    filename += ".xel";
    auto shortName = std::filesystem::path(name.substr(name.find_last_of('.') == std::string::npos ? 0 : name.find_last_of('.') + 1));
    shortName += ".xel";
    std::vector<std::filesystem::path> candidates = {
        importer.parent_path() / filename,
        importer.parent_path() / shortName,
    };
    for (const auto& directory : libraryPaths) {
        candidates.push_back(directory / filename);
        candidates.push_back(directory / shortName);
    }
    for (const auto& candidate : candidates) {
        std::error_code error;
        if (std::filesystem::is_regular_file(candidate, error) && !error) return std::filesystem::weakly_canonical(candidate);
    }
    throw std::runtime_error("cannot resolve library unit '" + name + "' imported by " + importer.string());
}

void loadSourceGraph(const std::filesystem::path& inputPath,
                     const std::vector<std::filesystem::path>& libraryPaths,
                     std::vector<SourceUnit>& units,
                     std::unordered_map<std::string, std::size_t>& loaded) {
    std::error_code error;
    auto path = std::filesystem::weakly_canonical(inputPath, error);
    if (error) path = std::filesystem::absolute(inputPath);
    const std::string key = path.string();
    if (loaded.contains(key)) return;

    std::ifstream source(path, std::ios::binary);
    if (!source) throw std::runtime_error("cannot open source file: " + path.string());
    std::ostringstream contents;
    contents << source.rdbuf();
    std::string text = contents.str();
    xe::Lexer lexer(text);
    xe::Parser parser(lexer.scan());
    auto parsed = parser.parse();
    loaded.emplace(key, units.size());
    units.push_back({path, std::move(text), std::move(parsed)});

    const auto imports = units.back().parsed.imports;
    for (const auto& import : imports)
        loadSourceGraph(resolveLibrary(import, path, libraryPaths), libraryPaths, units, loaded);
}
}
int main(int argc, char** argv) {
    try {
        if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
            help(std::cout);
            return 0;
        }
        if (argc == 2 && (std::string(argv[1]) == "--version" || std::string(argv[1]) == "-V")) {
            std::cout << "Xe 1.0.0\n";
            return 0;
        }
        if (argc < 3) { usage(); return 2; }
        const std::string mode = argv[1];
        std::string outputPath = "a.out";
        std::vector<std::string> sourcePaths, programArguments;
        std::vector<std::filesystem::path> libraryPaths;
        if (mode != "--run" && mode != "--build") { usage(); return 2; }
        bool passProgramArguments = false;
        for (int i = 2; i < argc; ++i) {
            const std::string argument = argv[i];
            if (mode == "--run" && argument == "--") { passProgramArguments = true; continue; }
            if (mode == "--run" && passProgramArguments) { programArguments.push_back(argument); continue; }
            if (argument == "--library-path" && i + 1 < argc) {
                libraryPaths.emplace_back(argv[++i]);
            } else if (mode == "--build" && argument == "-o" && i + 1 < argc) {
                outputPath = argv[++i];
            } else if (argument.starts_with('-')) {
                if (mode == "--run" && sourcePaths.size() == 1) programArguments.push_back(argument);
                else { usage(); return 2; }
            } else if (mode == "--run" && sourcePaths.size() == 1) {
                programArguments.push_back(argument);
            } else {
                sourcePaths.push_back(argument);
            }
        }
        if (sourcePaths.empty()) { usage(); return 2; }
        std::vector<SourceUnit> units;
        std::unordered_map<std::string, std::size_t> loaded;
        for (const auto& sourcePath : sourcePaths) loadSourceGraph(sourcePath, libraryPaths, units, loaded);

        // Collect module type aliases first so declarations may refer to aliases
        // declared in another source file or later in the input list.
        std::unordered_map<std::string, std::string> sharedAliases;
        for (const auto& source : units) {
            for (const auto& alias : source.parsed.aliases) sharedAliases[alias.name] = alias.target;
        }

        xe::Program program;
        for (const auto& source : units) {
            xe::Lexer lexer(source.text);
            xe::Parser parser(lexer.scan(), sharedAliases);
            auto unit = parser.parse();
            program.functions.insert(program.functions.end(), std::make_move_iterator(unit.functions.begin()), std::make_move_iterator(unit.functions.end()));
            program.aliases.insert(program.aliases.end(), std::make_move_iterator(unit.aliases.begin()), std::make_move_iterator(unit.aliases.end()));
            program.records.insert(program.records.end(), std::make_move_iterator(unit.records.begin()), std::make_move_iterator(unit.records.end()));
        }
        xe::Checker{}.check(program);
        if (mode == "--run") return xe::Evaluator(std::cin, std::cout, std::move(programArguments)).run(program);
        std::string generated = xe::Codegen{}.generate(program);
        auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        auto temporary = std::filesystem::temp_directory_path() / ("xelang-" + std::to_string(stamp) + ".cpp");
        { std::ofstream file(temporary, std::ios::binary); if (!file) throw std::runtime_error("cannot create temporary C++ file"); file << generated; }
        auto temporaryObject = temporary; temporaryObject.replace_extension(".obj");
#if defined(_WIN32)
        std::string command = shellQuote(XELANG_CXX_COMPILER) + " /nologo /std:c++20 /O2 /I" + shellQuote(XELANG_RUNTIME_DIR) + " /Fo" + shellQuote(temporaryObject.string()) + " /Fe:" + shellQuote(outputPath) + " " + shellQuote(temporary.string());
#else
        std::string command = shellQuote(XELANG_CXX_COMPILER) + " -std=c++20 -O2 -pthread -I " + shellQuote(XELANG_RUNTIME_DIR) + " " + shellQuote(temporary.string()) + " -o " + shellQuote(outputPath);
#endif
        int status = std::system(command.c_str()); std::error_code ignored;
        std::filesystem::remove(temporary, ignored); std::filesystem::remove(temporaryObject, ignored);
        return status == 0 ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "xe: " << error.what() << '\n'; return 1;
    }
}
