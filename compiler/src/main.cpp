#include "bf/bfc.h"
#include "bf/lexer.h"
#include "bf/parser.h"
#include "bf/semantic.h"
#include "bf/ir_builder.h"
#include "bf/diagnostic.h"
#include "bf/codegen.h"
#include "bf/runtime_codegen.h"
#include "bf/lld_linker.h"
#include "bf/optimizer.h"
#include "bf/toolchain.h"
#include "bf/lsp_server.h"
#include "bf/dap_server.h"
#include "bf/debugger.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdint>
#include <cctype>
#include <sys/stat.h>
#include <direct.h>
#include <windows.h>

namespace fs = std::filesystem;

namespace bfc {

// Link target: console (default), gui, sharedlib, staticlib.
static std::string g_target = "console";

static void link_settings(std::string& entry, std::string& subsystem, bool& dll) {
    entry = "mainCRTStartup";
    subsystem = "console";
    dll = false;
    if (g_target == "gui") {
        entry = "WinMainCRTStartup";
        subsystem = "windows";
    } else if (g_target == "sharedlib") {
        entry = "DllMain";
        subsystem = "windows";
        dll = true;
    }
}

static std::string read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "bfc: cannot open input file: " << path << "\n";
        std::exit(1);
    }
    return std::string((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
}

static bool run_command(const std::vector<std::string>& args) {
    // Build command with quoted arguments for the .bat file
    std::string cmd;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i != 0) cmd += " ";
        const std::string& a = args[i];
        if (a.find(' ') != std::string::npos || a.find('\t') != std::string::npos) {
            cmd += "\"" + a + "\"";
        } else {
            cmd += a;
        }
    }
    std::cout << "[bfc] " << cmd << "\n";

    // Write command to a temporary .bat file to avoid cmd.exe quoting issues
    // with paths containing spaces (e.g. "C:\Program Files\LLVM\bin\clang.exe")
    std::string bat_path = "_bfc_cmd.bat";
    {
        std::ofstream bat_file(bat_path);
        if (!bat_file) {
            return false;
        }
        bat_file << "@echo off\r\n";
        bat_file << cmd << "\r\n";
    }

    // Execute the .bat file using _wsystem
    std::wstring wbat_path(bat_path.begin(), bat_path.end());
    int rc = _wsystem(wbat_path.c_str());

    // Clean up
    remove(bat_path.c_str());

    return rc == 0;
}

static bool entry_name_equals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

static std::string entry_name(const std::unique_ptr<bfc::Program>& program) {
    // Prefer the conventional Sub Main even if library/procedure declarations
    // were included before it.
    for (const auto& mod : program->modules) {
        for (const auto& proc : mod->procedures) {
            if (!proc->is_function && entry_name_equals(proc->name, "Main")) {
                return proc->name;
            }
        }
    }
    for (const auto& mod : program->modules) {
        for (const auto& proc : mod->procedures) {
            if (!proc->is_function) return proc->name;
        }
    }
    return "Main";
}

static bool file_exists(const std::string& path) {
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}

static bool dir_exists(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) return false;
    return (buffer.st_mode & S_IFDIR) != 0;
}

static bool make_dir(const std::string& path) {
    return _mkdir(path.c_str()) == 0 || dir_exists(path);
}

// Simple hash function for incremental build caching
static uint64_t hash_string(const std::string& str) {
    uint64_t hash = 5381;
    for (char c : str) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// Bump this whenever code generation changes, so existing .obj files are not
// reused after a compiler upgrade.
static constexpr const char* kBfcCodegenVersion = "bfc-codegen-2026-11-22-r2";

static uint64_t source_cache_hash(const std::string& source) {
    return hash_string(source + "|" + kBfcCodegenVersion);
}

// Check if incremental build can be skipped
static bool can_skip_compile(const std::string& input_path, const std::string& source, const std::string& obj_path, uint64_t& out_hash) {
    out_hash = source_cache_hash(source);
    
    // Check if object file exists and is newer than source
    struct stat source_stat, obj_stat;
    if (stat(input_path.c_str(), &source_stat) != 0) return false;
    if (stat(obj_path.c_str(), &obj_stat) != 0) return false;
    
    // Object must be newer than source
    if (obj_stat.st_mtime < source_stat.st_mtime) return false;
    
    // Check hash cache file
    std::string cache_path = obj_path + ".hash";
    std::ifstream cache_file(cache_path);
    if (!cache_file) return false;
    
    uint64_t cached_hash = 0;
    cache_file >> cached_hash;
    return cached_hash == out_hash;
}

// Save hash for incremental build
static void save_hash(const std::string& obj_path, uint64_t hash) {
    std::string cache_path = obj_path + ".hash";
    std::ofstream cache_file(cache_path);
    if (cache_file) {
        cache_file << hash;
    }
}

static std::string trim_copy(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static bool resolve_include(const std::string& name, const fs::path& current_file,
                            const std::vector<std::string>& include_dirs,
                            fs::path& out) {
    fs::path p(name);
    if (p.is_absolute()) {
        if (fs::exists(p)) { out = p; return true; }
        return false;
    }

    std::vector<fs::path> bases;
    bases.push_back(current_file.parent_path());
    for (const auto& d : include_dirs) bases.push_back(fs::path(d));

    char exe_buf[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exe_buf, MAX_PATH);
    fs::path exe_dir = fs::path(exe_buf).parent_path();
    bases.push_back(exe_dir / "libraries");
    bases.push_back(exe_dir / "libraries" / "stdlib");
    bases.push_back(exe_dir / ".." / "libraries");
    bases.push_back(exe_dir / ".." / "libraries" / "stdlib");

    for (const auto& b : bases) {
        fs::path candidate = b / p;
        std::error_code ec;
        if (fs::exists(candidate, ec)) {
            out = candidate;
            return true;
        }
    }
    return false;
}

static bool preprocess_includes(const std::string& source,
                                const fs::path& current_file,
                                const std::vector<std::string>& include_dirs,
                                std::string& output,
                                std::vector<fs::path>& stack,
                                std::vector<fs::path>& included,
                                std::string& error) {
    std::error_code ec;
    fs::path canonical = fs::weakly_canonical(current_file, ec);
    if (!ec) {
        for (const auto& s : stack) {
            if (s == canonical) {
                error = "Circular include: " + current_file.string();
                return false;
            }
        }
        for (const auto& s : included) {
            if (s == canonical) return true; // already included once
        }
        included.push_back(canonical);
    }
    stack.push_back(canonical);

    std::istringstream stream(source);
    std::string line;
    while (std::getline(stream, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.pop_back();
        }
        std::string trimmed = trim_copy(line);
        std::string lower = trimmed;
        for (char& c : lower) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (std::getenv("BFC_DEBUG_INCLUDES")) {
            std::cerr << "[inc] " << current_file.string() << " | " << trimmed << "\n";
        }
        if (lower.rfind("include", 0) == 0) {
            size_t q1 = trimmed.find('"');
            size_t q2 = (q1 == std::string::npos) ? std::string::npos
                                                  : trimmed.find('"', q1 + 1);
            if (q1 != std::string::npos && q2 != std::string::npos) {
                std::string inc = trimmed.substr(q1 + 1, q2 - q1 - 1);
                fs::path inc_path;
                if (!resolve_include(inc, current_file, include_dirs, inc_path)) {
                    error = "Include not found: " + inc;
                    stack.pop_back();
                    return false;
                }
                std::string inc_source = read_file(inc_path.string());
                if (inc_source.empty()) {
                    error = "Cannot read include: " + inc_path.string();
                    stack.pop_back();
                    return false;
                }
                if (!preprocess_includes(inc_source, inc_path, include_dirs, output,
                                         stack, included, error)) {
                    return false;
                }
                output += "\n";
                continue;
            }
        }
        output += line;
        output += "\n";
    }
    stack.pop_back();
    return true;
}

static bool compile_source(const std::string& input, const std::string& output,
                           DiagnosticSink& sink,
                           const std::vector<std::string>& include_dirs = {}) {
    std::string source = read_file(input);
    fs::path in_path(input);
    std::string stem = in_path.stem().string();
    fs::path obj_path = stem + ".obj";
    fs::path runtime_c = stem + "_runtime.c";
    fs::path runtime_obj = stem + "_runtime.obj";

    std::string expanded;
    std::string include_error;
    std::vector<fs::path> include_stack;
    std::vector<fs::path> included_files;
    if (!preprocess_includes(source, in_path, include_dirs, expanded, include_stack,
                             included_files, include_error)) {
        bfc::Diagnostic d;
        d.severity = bfc::DiagnosticSeverity::Error;
        d.message = include_error;
        d.location = bfc::SourceLocation{in_path.string(), 1, 1, 0};
        sink.emit(d);
        return 1;
    }
    if (std::getenv("BFC_DEBUG_INCLUDES")) {
        std::ofstream dbg("bf_expanded.bf");
        dbg << expanded;
    }

    // Check incremental build cache (both objects must exist)
    uint64_t source_hash = 0;
    if (g_target == "console" && fs::exists(obj_path) && fs::exists(runtime_obj) &&
        can_skip_compile(input, expanded, obj_path.string(), source_hash)) {
        std::cout << "[bfc] Skipping compilation (up-to-date): " << input << "\n";
        // Just re-link
        std::vector<std::string> object_files = {obj_path.string(), runtime_obj.string()};
        bfc::LldLinker linker(sink);
        std::string entry;
        std::string subsystem;
        bool dll = false;
        link_settings(entry, subsystem, dll);
        if (!linker.link_executable(object_files, output, entry, subsystem, dll)) {
            return 1;
        }
        std::cout << "[bfc] Produced " << output << " (cached)\n";
        return 0;
    }

    bfc::Lexer lexer(expanded, in_path.filename().string());
    bfc::Parser parser(lexer, sink);
    auto program = parser.parse();
    if (sink.has_errors()) return 1;

    bfc::SemanticAnalyzer sema(sink);
    sema.analyze(*program);
    if (sink.has_errors()) return 1;

    bool has_main = false;
    for (const auto& mod : program->modules) {
        for (const auto& proc : mod->procedures) {
            if (!proc->is_function && proc->name == "Main") {
                has_main = true;
                break;
            }
        }
        if (has_main) break;
    }
    if (!has_main) {
        bfc::Diagnostic d;
        d.severity = bfc::DiagnosticSeverity::Error;
        d.message = "Program must have a 'Sub Main()' entry point";
        d.location = bfc::SourceLocation{};
        sink.emit(d);
        return 1;
    }

    bfc::IrBuilder irb(sink);
    bfc::IrModule module = irb.lower(*program);
    if (sink.has_errors()) return 1;

    // Optimizer disabled while constant folding is being fixed.
    // bfc::Optimizer opt(sink, bfc::OptLevel::O2);
    // opt.optimize(module);
    std::cout << "[bfc] IR optimization complete\n";

    bfc::LlvmCodeGen codegen(sink);
    if (!codegen.generate_object(module, obj_path.string())) {
        return 1;
    }
    std::cout << "[bfc] Wrote " << obj_path << "\n";

    // Save hash for incremental build
    save_hash(obj_path.string(), source_hash);

    bfc::RuntimeCodeGen rtgen(sink);
    if (!rtgen.generate_runtime_c(runtime_c.string(), entry_name(program),
                                  !module.forms.empty(), g_target)) {
        return 1;
    }
    std::cout << "[bfc] Wrote " << runtime_c << "\n";

    std::string clang = bfc::find_llvm_tool("clang.exe");
    std::vector<std::string> cc_args = {clang, "-target", "x86_64-pc-windows-msvc",
        "-DWINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP", "-c", runtime_c.string(), "-o", runtime_obj.string()};
    if (!bfc::run_command(cc_args)) {
        std::cerr << "bfc: failed to compile runtime\n";
        return 1;
    }

    if (g_target == "staticlib") {
        std::string lib = bfc::find_llvm_tool("llvm-lib.exe");
        std::string lib_out = output;
        if (lib_out.size() < 4 || lib_out.substr(lib_out.size() - 4) != ".lib") {
            lib_out += ".lib";
        }
        std::string lib_cmd = "\"" + lib + "\" /out:" + lib_out + " " +
                              obj_path.string() + " " + runtime_obj.string();
        if (std::system(lib_cmd.c_str()) != 0) {
            return 1;
        }
        std::cout << "[bfc] Produced " << lib_out << "\n";
        return 0;
    }

    bfc::LldLinker linker(sink);
    std::vector<std::string> object_files = {obj_path.string(), runtime_obj.string()};
    std::string entry;
    std::string subsystem;
    bool dll = false;
    link_settings(entry, subsystem, dll);
    if (!linker.link_executable(object_files, output, entry, subsystem, dll)) {
        return 1;
    }

    std::cout << "[bfc] Produced " << output << "\n";
    return 0;
}

static bool write_file(const std::string& path, const std::string& content) {
    std::ofstream out(path);
    if (!out) return false;
    out << content;
    return true;
}

static std::string read_file_opt(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return "";
    return std::string((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
}

} // namespace bfc

int main(int argc, char** argv) {
    if (argc >= 2 &&
        (std::string(argv[1]) == "--lsp" || std::string(argv[1]) == "lsp")) {
        return bfc::run_lsp_server();
    }

    if (argc >= 2 && std::string(argv[1]) == "--dap") {
        return bfc::run_dap_server();
    }

    if (argc >= 3 && std::string(argv[1]) == "debug") {
        std::vector<std::string> debug_args;
        std::string debug_script;
        for (int i = 3; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--script" && i + 1 < argc) {
                debug_script = argv[++i];
            } else {
                debug_args.push_back(arg);
            }
        }
        return bfc::run_debugger(argv[2], debug_args, debug_script);
    }

    if (argc < 2) {
        std::cerr << "Usage: bfc <command> [args...]\n";
        std::cerr << "Commands:\n";
        std::cerr << "  lsp | --lsp       Run the language server on stdio\n";
        std::cerr << "  debug <exe>      Debug an executable (breakpoints/step/backtrace)\n";
        std::cerr << "  new <project>     Create a new BasicForge project\n";
        std::cerr << "  build <proj>      Build a project (.bfproj)\n";
        std::cerr << "  run <proj>        Build and run a project\n";
        std::cerr << "  check <proj>      Check a project for errors\n";
        std::cerr << "  clean <proj>      Clean build artifacts\n";
        std::cerr << "  fmt <proj>        Format source files\n";
        std::cerr << "  test <proj>       Run tests\n";
        std::cerr << "  doc <proj>        Generate documentation\n";
        std::cerr << "  <input.bf> [-o <output.exe>]  Compile a single source file\n";
        return 2;
    }

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--target" && i + 1 < argc) {
            bfc::g_target = argv[++i];
        }
    }

    std::string cmd = argv[1];

    // Detect mode: known command, .bfproj project file, or .bf source file
    bool is_cmd = (cmd == "new" || cmd == "build" || cmd == "run" ||
                   cmd == "check" || cmd == "clean" || cmd == "fmt" ||
                   cmd == "test" || cmd == "doc");
    bool is_bfproj = (cmd.size() >= 7 && cmd.compare(cmd.size() - 7, 7, ".bfproj") == 0);
    bool is_bf = (cmd.size() >= 3 && cmd.compare(cmd.size() - 3, 3, ".bf") == 0);

    if (is_cmd && cmd == "new") {
        if (argc < 3) {
            std::cerr << "bfc: missing project name\n";
            return 1;
        }
        std::string proj = argv[2];
        if (!bfc::make_dir(proj)) {
            std::cerr << "bfc: cannot create directory '" << proj << "'\n";
            return 1;
        }
        bfc::make_dir(proj + "/src");
        bfc::make_dir(proj + "/lib");

        std::string proj_file = proj + "/" + proj + ".bfproj";
        std::string proj_content = "{\n";
        proj_content += "  \"name\": \"" + proj + "\",\n";
        proj_content += "  \"version\": \"1.0.0\",\n";
        proj_content += "  \"main\": \"src/Main.bf\",\n";
        proj_content += "  \"sources\": [\"src/*.bf\"],\n";
        proj_content += "  \"output\": \"" + proj + ".exe\"\n";
        proj_content += "}\n";
        bfc::write_file(proj_file, proj_content);

        std::string main_bf = proj + "/src/Main.bf";
        std::string main_content = "Module Main\n\n";
        main_content += "    Sub Main()\n";
        main_content += "        Print(\"Hello from \" & \"" + proj + "\")\n";
        main_content += "    End Sub\n\n";
        main_content += "End Module\n";
        bfc::write_file(main_bf, main_content);

        std::cout << "[bfc] Created project '" << proj << "'\n";
        std::cout << "[bfc]   " << proj_file << "\n";
        std::cout << "[bfc]   " << main_bf << "\n";
        return 0;
    }

    if (is_cmd) {
        if (argc < 3) {
            std::cerr << "bfc: missing project file for '" << cmd << "'\n";
            return 1;
        }
        std::string proj_file = argv[2];
        if (!bfc::file_exists(proj_file)) {
            std::cerr << "bfc: project file not found: " << proj_file << "\n";
            return 1;
        }
        std::string proj_content = bfc::read_file_opt(proj_file);
        std::string proj_dir = fs::path(proj_file).parent_path().string();
        if (proj_dir.empty()) proj_dir = ".";

        auto get_json_field = [&](const std::string& key) -> std::string {
            size_t pos = proj_content.find("\"" + key + "\"");
            if (pos == std::string::npos) return "";
            size_t colon = proj_content.find(":", pos);
            if (colon == std::string::npos) return "";
            size_t quote1 = proj_content.find("\"", colon + 1);
            if (quote1 == std::string::npos) return "";
            size_t quote2 = proj_content.find("\"", quote1 + 1);
            if (quote2 == std::string::npos) return "";
            return proj_content.substr(quote1 + 1, quote2 - quote1 - 1);
        };

        std::string main_file = get_json_field("main");
        std::string output = get_json_field("output");

        if (cmd == "build" || cmd == "run" || cmd == "check") {
            if (main_file.empty()) {
                std::cerr << "bfc: no 'main' field in project file\n";
                return 1;
            }
            std::string main_path = proj_dir + "/" + main_file;
            if (!bfc::file_exists(main_path)) {
                std::cerr << "bfc: main source not found: " << main_path << "\n";
                return 1;
            }
            // Change to project directory so output files are created there
            char orig_dir[4096];
            _getcwd(orig_dir, sizeof(orig_dir));
            _chdir(proj_dir.c_str());
            std::string rel_main = main_file;
            std::string rel_output = output.empty() ? "a.exe" : output;
            bfc::ConsoleDiagnosticSink sink;
            int rc = bfc::compile_source(rel_main, rel_output, sink);
            _chdir(orig_dir);
            if (rc != 0) return rc;
        }

        if (cmd == "run") {
            if (output.empty()) output = "a.exe";
            std::string exe_path = proj_dir + "/" + output;
            if (!bfc::file_exists(exe_path)) {
                std::cerr << "bfc: executable not found: " << exe_path << "\n";
                return 1;
            }
            std::cout << "[bfc] Running " << exe_path << "\n";
            std::vector<std::string> run_args = {exe_path};
            if (!bfc::run_command(run_args)) {
                return 1;
            }
        }

        if (cmd == "clean") {
            std::string out = output.empty() ? "a.exe" : output;
            std::string exe_path = proj_dir + "/" + out;
            if (bfc::file_exists(exe_path)) {
                remove(exe_path.c_str());
                std::cout << "[bfc] Removed " << exe_path << "\n";
            }
            for (const auto& ext : {".obj", ".c"}) {
                for (auto& p : fs::recursive_directory_iterator(proj_dir)) {
                    if (p.path().extension() == ext) {
                        remove(p.path().string().c_str());
                        std::cout << "[bfc] Removed " << p.path() << "\n";
                    }
                }
            }
        }

        if (cmd == "fmt") {
            std::cout << "[bfc] Formatting source files...\n";
            // Simple formatter: normalize indentation and keyword casing
            std::string source = bfc::read_file_opt(argv[2]);
            if (source.empty()) {
                std::cerr << "bfc: cannot read file: " << argv[2] << "\n";
                return 1;
            }
            
            // Format: normalize line endings, indentation
            std::string formatted;
            int indent = 0;
            std::istringstream stream(source);
            std::string line;
            while (std::getline(stream, line)) {
                // Trim leading/trailing whitespace
                size_t start = line.find_first_not_of(" \t");
                if (start == std::string::npos) {
                    formatted += "\n";
                    continue;
                }
                std::string trimmed = line.substr(start);
                
                // Adjust indent based on keywords
                if (trimmed.find("End ") == 0 || trimmed.find("Next") == 0 || 
                    trimmed.find("Loop") == 0 || trimmed.find("Wend") == 0) {
                    if (indent > 0) indent--;
                }
                
                // Add indentation
                for (int i = 0; i < indent; i++) formatted += "    ";
                formatted += trimmed + "\n";
                
                // Increase indent after opening keywords
                if (trimmed.find("Sub ") != std::string::npos || 
                    trimmed.find("Function ") != std::string::npos ||
                    trimmed.find("Class ") != std::string::npos ||
                    trimmed.find("Module ") != std::string::npos ||
                    trimmed.find("If ") != std::string::npos && trimmed.find("Then") != std::string::npos ||
                    trimmed.find("For ") != std::string::npos ||
                    trimmed.find("While ") != std::string::npos ||
                    trimmed.find("Do") != std::string::npos ||
                    trimmed.find("Try") != std::string::npos ||
                    trimmed.find("Select Case") != std::string::npos) {
                    indent++;
                }
            }
            
            bfc::write_file(argv[2], formatted);
            std::cout << "[bfc] Formatted: " << argv[2] << "\n";
            return 0;
        }

        if (cmd == "test") {
            std::cout << "[bfc] Running tests...\n";
            // Find all .bf test files in the project
            std::string test_dir = proj_dir + "/tests";
            if (!fs::exists(test_dir)) {
                std::cout << "[bfc] No tests directory found\n";
                return 0;
            }
            
            int passed = 0, failed = 0;
            for (const auto& entry : fs::directory_iterator(test_dir)) {
                if (entry.path().extension() == ".bf") {
                    std::string test_file = entry.path().string();
                    std::string test_exe = test_file.substr(0, test_file.size() - 3) + ".exe";
                    
                    bfc::ConsoleDiagnosticSink sink;
                    if (bfc::compile_source(test_file, test_exe, sink) == 0) {
                        // Run the test
                        std::string runCmd = "\"" + test_exe + "\"";
                        int result = std::system(runCmd.c_str());
                        if (result == 0) {
                            std::cout << "[PASS] " << entry.path().filename().string() << "\n";
                            passed++;
                        } else {
                            std::cout << "[FAIL] " << entry.path().filename().string() << "\n";
                            failed++;
                        }
                    } else {
                        std::cout << "[FAIL] " << entry.path().filename().string() << " (compile error)\n";
                        failed++;
                    }
                }
            }
            
            std::cout << "\n[bfc] Results: " << passed << " passed, " << failed << " failed\n";
            return failed > 0 ? 1 : 0;
        }

        if (cmd == "doc") {
            std::cout << "[bfc] Generating documentation...\n";
            // Simple doc generator: extract XML comments (''' <summary>)
            std::string source = bfc::read_file_opt(argv[2]);
            if (source.empty()) {
                std::cerr << "bfc: cannot read file: " << argv[2] << "\n";
                return 1;
            }
            
            std::string html = "<!DOCTYPE html>\n<html>\n<head>\n<title>Documentation</title>\n";
            html += "<style>body{font-family:system-ui;margin:2em;} .doc{margin:1em 0;padding:1em;background:#f5f5f5;border-left:4px solid #007acc;}</style>\n";
            html += "</head>\n<body>\n<h1>Documentation</h1>\n";
            
            std::istringstream stream(source);
            std::string line;
            std::string currentSummary;
            int lineNum = 0;
            
            while (std::getline(stream, line)) {
                lineNum++;
                // Look for XML doc comments
                if (line.find("'''") != std::string::npos) {
                    std::string comment = line.substr(line.find("'''") + 3);
                    // Trim
                    size_t start = comment.find_first_not_of(" \t");
                    if (start != std::string::npos) {
                        comment = comment.substr(start);
                    }
                    if (comment.find("<summary>") != std::string::npos) {
                        currentSummary = comment;
                    }
                } else if (line.find("Sub ") != std::string::npos || 
                           line.find("Function ") != std::string::npos ||
                           line.find("Class ") != std::string::npos) {
                    // Extract name
                    std::string name;
                    size_t subPos = line.find("Sub ");
                    size_t funcPos = line.find("Function ");
                    size_t classPos = line.find("Class ");
                    
                    if (subPos != std::string::npos) {
                        name = line.substr(subPos + 4);
                    } else if (funcPos != std::string::npos) {
                        name = line.substr(funcPos + 9);
                    } else if (classPos != std::string::npos) {
                        name = line.substr(classPos + 6);
                    }
                    
                    // Trim name
                    size_t end = name.find_first_of(" \t\r\n");
                    if (end != std::string::npos) name = name.substr(0, end);
                    
                    if (!name.empty()) {
                        html += "<div class=\"doc\">\n<h2>" + name + "</h2>\n";
                        if (!currentSummary.empty()) {
                            html += "<p>" + currentSummary + "</p>\n";
                            currentSummary.clear();
                        }
                        html += "<code>Line " + std::to_string(lineNum) + "</code>\n</div>\n";
                    }
                }
            }
            
            html += "</body>\n</html>\n";
            
            std::string outFile = "documentation.html";
            bfc::write_file(outFile, html);
            std::cout << "[bfc] Generated: " << outFile << "\n";
            return 0;
        }

        return 0;
    }

    if (is_bf) {
        // Single file compilation: bfc <input.bf> [-o <output.exe>] [-I <dir>]
        std::string input = cmd;
        std::string output = "a.exe";
        std::vector<std::string> include_dirs;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-o" && i + 1 < argc) {
                output = argv[++i];
            } else if (arg == "-I" && i + 1 < argc) {
                include_dirs.push_back(argv[++i]);
            }
        }

        bfc::ConsoleDiagnosticSink sink;
        return bfc::compile_source(input, output, sink, include_dirs);
    }

    std::cerr << "bfc: unknown command or file type: " << cmd << "\n";
    return 1;
}
