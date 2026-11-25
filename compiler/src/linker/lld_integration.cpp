#include "bf/lld_linker.h"
#include "bf/diagnostic.h"
#include "bf/toolchain.h"
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cwchar>
#include <cstdio>

#ifdef BFC_LLVM_HEADERS_AVAILABLE
#include <lld/Common/Driver.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/WithColor.h>
#include <llvm/Support/raw_ostream.h>
#endif

namespace bfc {

LldLinker::LldLinker(DiagnosticSink& sink) : sink_(sink) {}

bool LldLinker::link_executable(const std::vector<std::string>& object_files,
                                const std::string& output_path,
                                const std::string& entry_point,
                                const std::string& subsystem,
                                bool dll) {
#ifdef BFC_LLVM_HEADERS_AVAILABLE
    // Prepare arguments for LLD COFF linker
    std::vector<const char*> args;
    args.push_back(bfc::find_llvm_tool("lld-link.exe"));
    args.push_back("/nologo");
    args.push_back("/debug");
    args.push_back(("/out:" + output_path).c_str());
    args.push_back(("/entry:" + entry_point).c_str());
    args.push_back("/subsystem:console");
    if (subsystem == "windows") {
        args.push_back("/subsystem:windows");
    }
    if (dll) {
        args.push_back("/DLL");
    }
    
    // Library paths
    args.push_back("/libpath:C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207\\lib\\x64");
    args.push_back("/libpath:C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207\\lib\\onecore\\x64");
    args.push_back("/libpath:C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.26100.0\\ucrt\\x64");
        args.push_back("/libpath:C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.26100.0\\um\\x64");
    
    // Required libraries
    args.push_back("legacy_stdio_definitions.lib");
    args.push_back("libcmt.lib");
        if (dll) { args.push_back("msvcrt.lib"); args.push_back("libcmt.lib"); }
    args.push_back("libvcruntime.lib");
    args.push_back("kernel32.lib");
        if (dll) args.push_back("ucrt.lib");
        if (dll) { args.push_back("msvcrt.lib"); args.push_back("libcmt.lib"); }
    args.push_back("user32.lib");
    args.push_back("gdi32.lib");
        args.push_back("shell32.lib");
        args.push_back("comctl32.lib");
        args.push_back("ole32.lib");

    // Object files
    std::vector<std::string> obj_strings;
    for (const auto& obj : object_files) {
        obj_strings.push_back(obj);
    }
    for (const auto& obj : obj_strings) {
        args.push_back(obj.c_str());
    }
    args.push_back("libvcruntime.lib");
    args.push_back("kernel32.lib");
        if (dll) args.push_back("ucrt.lib");
        if (dll) { args.push_back("msvcrt.lib"); args.push_back("libcmt.lib"); }
    args.push_back("user32.lib");
    args.push_back("gdi32.lib");
        args.push_back("shell32.lib");
        args.push_back("comctl32.lib");
        args.push_back("ole32.lib");

    // Add null terminator
    args.push_back(nullptr);

    // Redirect LLD diagnostics to our sink
    std::string stdout_str;
    std::string stderr_str;
    llvm::raw_string_ostream stdout_stream(stdout_str);
    llvm::raw_string_ostream stderr_stream(stderr_str);

    bool success = lld::coff::link(llvm::ArrayRef<const char*>(args.data(), args.size() - 1), stdout_stream, stderr_stream);
    
    stdout_stream.flush();
    stderr_stream.flush();

    if (!success) {
        Diagnostic d;
        d.severity = DiagnosticSeverity::Error;
        d.message = "LLD linking failed: " + stderr_str;
        d.location = SourceLocation{};
        sink_.emit(d);
        return false;
    }

    Diagnostic d;
    d.severity = DiagnosticSeverity::Note;
    d.message = "[bfc] Linked successfully: " + output_path;
    d.location = SourceLocation{};
    sink_.emit(d);
    return true;
#else
    // Fallback: write command to .bat file and execute (avoids cmd.exe quoting issues)
    std::string cmd;
    std::vector<std::string> args;
    args.push_back(bfc::find_llvm_tool("lld-link.exe"));
    args.push_back("/nologo");
    args.push_back("/debug");
    args.push_back("/out:" + output_path);
    args.push_back("/entry:" + entry_point);
    args.push_back("/subsystem:console");
    if (subsystem == "windows") {
        args.push_back("/subsystem:windows");
    }
    if (dll) {
        args.push_back("/DLL");
        const char* aliases[] = {
            "strlen=_strlen", "malloc=_malloc", "free=_free",
            "memcpy=_memcpy", "memset=_memset", "strcmp=_strcmp",
            "strcpy=_strcpy", "strncpy=_strncpy", "toupper=_toupper",
            "tolower=_tolower", "fopen=_fopen", "fclose=_fclose",
            "fread=_fread", "fwrite=_fwrite", "fseek=_fseek",
            "ftell=_ftell", "fgets=_fgets", "fputs=_fputs",
            "printf=_printf", "fprintf=_fprintf", "sprintf=_sprintf",
            "snprintf=_snprintf", "atoi=_atoi", "atof=_atof",
            "strtod=_strtod", "strtoll=_strtoll", "abort=_abort",
            "exit=_exit", "system=_system", "getenv=_getenv",
            "time=_time", "localtime=_localtime", "sqrt=_sqrt",
            "sin=_sin", "cos=_cos", "tan=_tan", "pow=_pow",
            "floor=_floor", "ceil=_ceil", "fmod=_fmod", "fabs=_fabs",
            "log=_log", "log10=_log10", "exp=_exp",
        };
        for (const char* alias : aliases) {
            args.push_back(std::string("/ALTERNATENAME:") + alias);
        }
    }
    args.push_back("/libpath:C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207\\lib\\x64");
    args.push_back("/libpath:C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207\\lib\\onecore\\x64");
        args.push_back("/libpath:C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.26100.0\\ucrt\\x64");
        args.push_back("/libpath:C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.26100.0\\um\\x64");
        args.push_back("legacy_stdio_definitions.lib");
        args.push_back("libcmt.lib");
        if (dll) { args.push_back("msvcrt.lib"); args.push_back("libcmt.lib"); }
        args.push_back("libvcruntime.lib");
        args.push_back("kernel32.lib");
        if (dll) args.push_back("ucrt.lib");
        if (dll) { args.push_back("msvcrt.lib"); args.push_back("libcmt.lib"); }
        args.push_back("user32.lib");
        args.push_back("gdi32.lib");
        args.push_back("shell32.lib");
        args.push_back("comctl32.lib");
        args.push_back("ole32.lib");

        for (const auto& obj : object_files) {
            args.push_back(obj);
        }
        args.push_back("libvcruntime.lib");
        args.push_back("kernel32.lib");
        if (dll) args.push_back("ucrt.lib");
        if (dll) { args.push_back("msvcrt.lib"); args.push_back("libcmt.lib"); }
        args.push_back("user32.lib");
        args.push_back("gdi32.lib");
        args.push_back("shell32.lib");
        args.push_back("comctl32.lib");
        args.push_back("ole32.lib");

    for (size_t i = 0; i < args.size(); ++i) {
        if (i != 0) cmd += " ";
        const std::string& a = args[i];
        if (a.find(' ') != std::string::npos || a.find('\t') != std::string::npos) {
            cmd += "\"" + a + "\"";
        } else {
            cmd += a;
        }
    }

    // Write command to a temporary .bat file
    std::string bat_path = "_bfc_link_cmd.bat";
    {
        if (std::getenv("BFC_DEBUG_LINK")) {
            std::fprintf(stderr, "[lld bat] %s\n", cmd.c_str());
        }
        std::ofstream bat_file(bat_path);
        if (!bat_file) {
            Diagnostic d;
            d.severity = DiagnosticSeverity::Error;
            d.message = "Failed to create batch file for linking";
            d.location = SourceLocation{};
            sink_.emit(d);
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

    if (rc != 0) {
        Diagnostic d;
        d.severity = DiagnosticSeverity::Error;
        d.message = "LLD linking failed (exit code " + std::to_string(rc) + ")";
        d.location = SourceLocation{};
        sink_.emit(d);
        return false;
    }

    Diagnostic d;
    d.severity = DiagnosticSeverity::Note;
    d.message = "[bfc] Linked successfully: " + output_path;
    d.location = SourceLocation{};
    sink_.emit(d);
    return true;
#endif
}

} // namespace bfc
