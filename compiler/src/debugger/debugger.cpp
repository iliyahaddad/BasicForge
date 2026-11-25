// Native Win32 debugger for BasicForge.
//
// Uses INT3 breakpoints (process-wide, thread-independent), single-step,
// continue, backtrace, register inspection and global reads. Symbols/line info
// come from the PDB produced by lld-link /debug.

#include "bf/debugger.h"
#include "bf/toolchain.h"

#ifndef WINAPI_FAMILY_PARTITION
#define WINAPI_FAMILY_PARTITION(x) 1
#endif
#include <windows.h>
#include <dbghelp.h>

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <tlhelp32.h>

#pragma comment(lib, "dbghelp.lib")

namespace bfc {
namespace {

struct Breakpoint {
    uint64_t address = 0;
    std::string name;
    std::string condition;
    uint8_t saved_byte = 0;
    bool armed = false;
    bool enabled = true;
};

struct DebuggerState {
    HANDLE process = nullptr;
    HANDLE thread = nullptr;
    DWORD pid = 0;
    DWORD mainThreadId = 0;
    bool running = true;
    bool stopped = false;
    bool step_mode = false;
    int pending_breakpoint = -1;
    uint64_t moduleBase = 0;
    std::string imagePath;
    std::vector<Breakpoint> breakpoints;
    std::vector<std::string> watches;
} g;

std::string g_script;

static bool iequals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

std::string basename_of(const std::string& path) {
    size_t slash = path.find_last_of("\\/");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

bool read_memory(uint64_t address, void* buffer, size_t size) {
    SIZE_T read = 0;
    return ReadProcessMemory(g.process, reinterpret_cast<LPCVOID>(address), buffer,
                             size, &read) &&
           read == size;
}

bool write_memory(uint64_t address, const void* buffer, size_t size) {
    SIZE_T written = 0;
    return WriteProcessMemory(g.process, reinterpret_cast<LPVOID>(address),
                              buffer, size, &written) &&
           written == size;
}

bool get_thread_context(CONTEXT& ctx, DWORD flags) {
    ctx.ContextFlags = flags;
    return GetThreadContext(g.thread, &ctx) != FALSE;
}

bool set_thread_context(CONTEXT& ctx, DWORD flags) {
    ctx.ContextFlags = flags;
    return SetThreadContext(g.thread, &ctx) != FALSE;
}

void arm(Breakpoint& bp) {
    uint8_t cc = 0xCC;
    if (write_memory(bp.address, &cc, 1)) {
        FlushInstructionCache(g.process, reinterpret_cast<LPCVOID>(bp.address), 1);
        bp.armed = true;
    }
}

void disarm(Breakpoint& bp) {
    if (!bp.armed) return;
    write_memory(bp.address, &bp.saved_byte, 1);
    FlushInstructionCache(g.process, reinterpret_cast<LPCVOID>(bp.address), 1);
    bp.armed = false;
}

void rearm_all() {
    CONTEXT ctx{};
    if (!get_thread_context(ctx, CONTEXT_DEBUG_REGISTERS)) return;
    ctx.Dr0 = ctx.Dr1 = ctx.Dr2 = ctx.Dr3 = 0;
    ctx.Dr7 = 0;
    set_thread_context(ctx, CONTEXT_DEBUG_REGISTERS);

    int slot = 0;
    for (auto& bp : g.breakpoints) {
        bp.armed = false;
        if (!bp.enabled) continue;
        CONTEXT c{};
        if (!get_thread_context(c, CONTEXT_DEBUG_REGISTERS)) break;
        switch (slot) {
            case 0: c.Dr0 = bp.address; break;
            case 1: c.Dr1 = bp.address; break;
            case 2: c.Dr2 = bp.address; break;
            case 3: c.Dr3 = bp.address; break;
            default: break;
        }
        c.Dr7 |= (1ULL << (slot * 2));
        set_thread_context(c, CONTEXT_DEBUG_REGISTERS);
        bp.armed = true;
        slot++;
    }
}

void list_threads() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateToolhelp32Snapshot failed\n";
        return;
    }
    THREADENTRY32 te{};
    te.dwSize = sizeof(te);
    if (Thread32First(snap, &te)) {
        do {
            if (te.th32OwnerProcessID == g.pid) {
                std::cout << "Thread " << te.th32ThreadID
                          << " (base priority " << te.tpBasePri << ")\n";
            }
        } while (Thread32Next(snap, &te));
    }
    CloseHandle(snap);
}

bool set_breakpoint(const std::string& name, uint64_t address,
                    const std::string& condition = "") {
    uint8_t original = 0;
    if (!read_memory(address, &original, 1)) {
        std::cerr << "Could not read address 0x" << std::hex << address << std::dec << "\n";
        return false;
    }
    Breakpoint bp;
    bp.address = address;
    bp.name = name;
    bp.condition = condition;
    bp.saved_byte = original;
    g.breakpoints.push_back(bp);
    arm(g.breakpoints.back());
    std::cout << "Breakpoint " << g.breakpoints.size() << " at " << name
              << " (0x" << std::hex << address << std::dec << ")\n";
    return true;
}

uint64_t find_symbol(const std::string& name) {
    alignas(SYMBOL_INFO) unsigned char buffer[sizeof(SYMBOL_INFO) + 512];
    auto* sym = reinterpret_cast<SYMBOL_INFO*>(buffer);
    std::memset(buffer, 0, sizeof(buffer));
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen = 511;
    DWORD64 displacement = 0;
    if (SymFromName(GetCurrentProcess(), name.c_str(), sym)) {
        return static_cast<uint64_t>(sym->Address);
    }
    return 0;
}

struct LineSearch {
    std::string file;
    int line = 0;
    uint64_t address = 0;
};

BOOL CALLBACK enum_symbols_callback(SYMBOL_INFO* sym, ULONG, PVOID context) {
    auto* search = static_cast<LineSearch*>(context);
    IMAGEHLP_LINE64 line{};
    line.SizeOfStruct = sizeof(line);
    DWORD displacement = 0;
    if (SymGetLineFromAddr64(GetCurrentProcess(), sym->Address, &displacement, &line)) {
        if (line.LineNumber == static_cast<DWORD>(search->line) &&
            basename_of(line.FileName) == basename_of(search->file)) {
            search->address = static_cast<uint64_t>(sym->Address);
            return FALSE;
        }
    }
    return TRUE;
}

bool find_line(const std::string& file, int line, uint64_t& address) {
    LineSearch search{file, line, 0};
    SymEnumSymbols(GetCurrentProcess(), 0, nullptr, enum_symbols_callback, &search);
    address = search.address;
    return address != 0;
}

bool parse_break_spec(const std::string& spec, std::string& name, uint64_t& address) {
    size_t colon = spec.find_last_of(':');
    if (colon != std::string::npos && colon + 1 < spec.size()) {
        std::string file = spec.substr(0, colon);
        std::string line_text = spec.substr(colon + 1);
        bool all_digits = !line_text.empty();
        for (char c : line_text) {
            if (c < '0' || c > '9') { all_digits = false; break; }
        }
        if (all_digits) {
            int line = std::atoi(line_text.c_str());
            if (find_line(file, line, address)) {
                name = spec;
                return true;
            }
            std::cerr << "Could not resolve " << spec << "\n";
            return false;
        }
    }
    name = spec;
    address = find_symbol(spec);
    if (address == 0) {
        std::cerr << "Symbol not found: " << spec << "\n";
        return false;
    }
    return true;
}

void print_symbol(const std::string& name) {
    uint64_t address = 0;
    if (name.rfind("0x", 0) == 0 || name.rfind("0X", 0) == 0) {
        address = std::strtoull(name.c_str(), nullptr, 16);
    } else {
        address = find_symbol(name);
    }
    if (address == 0) {
        std::cerr << "Symbol not found: " << name << "\n";
        return;
    }
    uint64_t value = 0;
    if (!read_memory(address, &value, sizeof(value))) {
        std::cerr << "Could not read memory at 0x" << std::hex << address << std::dec << "\n";
        return;
    }
    std::cout << name << " @ 0x" << std::hex << address << std::dec
              << " = " << static_cast<int64_t>(value)
              << " (0x" << std::hex << value << std::dec << ")\n";
}

void print_registers() {
    CONTEXT ctx{};
    if (!get_thread_context(ctx, CONTEXT_CONTROL | CONTEXT_INTEGER)) {
        std::cerr << "Could not read registers.\n";
        return;
    }
    std::cout << "Rip=0x" << std::hex << ctx.Rip
              << " Rsp=0x" << ctx.Rsp
              << " Rbp=0x" << ctx.Rbp << std::dec << "\n";
    std::cout << "Rax=" << static_cast<int64_t>(ctx.Rax)
              << " Rcx=" << static_cast<int64_t>(ctx.Rcx)
              << " Rdx=" << static_cast<int64_t>(ctx.Rdx)
              << " R8=" << static_cast<int64_t>(ctx.R8)
              << " R9=" << static_cast<int64_t>(ctx.R9) << "\n";
}

void print_backtrace() {
    CONTEXT ctx{};
    if (!get_thread_context(ctx, CONTEXT_CONTROL | CONTEXT_INTEGER)) {
        std::cerr << "Could not read context.\n";
        return;
    }
    STACKFRAME64 frame{};
    frame.AddrPC.Offset = ctx.Rip;
    frame.AddrFrame.Offset = ctx.Rbp;
    frame.AddrStack.Offset = ctx.Rsp;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;

    int depth = 0;
    while (depth < 32 &&
           StackWalk64(IMAGE_FILE_MACHINE_AMD64, GetCurrentProcess(), g.process,
                       &frame, &ctx, nullptr, SymFunctionTableAccess64,
                       SymGetModuleBase64, nullptr)) {
        alignas(SYMBOL_INFO) unsigned char buffer[sizeof(SYMBOL_INFO) + 512];
        auto* sym = reinterpret_cast<SYMBOL_INFO*>(buffer);
        std::memset(buffer, 0, sizeof(buffer));
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 511;
        DWORD64 displacement = 0;
        std::cout << "  #" << depth << " 0x" << std::hex << frame.AddrPC.Offset
                  << std::dec;
        if (SymFromAddr(GetCurrentProcess(), frame.AddrPC.Offset, &displacement, sym)) {
            std::cout << " " << sym->Name;
        }
        std::cout << "\n";
        if (frame.AddrPC.Offset == 0) break;
        ++depth;
    }
}

void print_help() {
    std::cout
        << "Commands:\n"
        << "  break <function> | break <file>:<line>  set a breakpoint\n"
        << "  delete <n> (d)                        remove a breakpoint\n"
        << "  disable <n> / enable <n>              toggle a breakpoint\n"
        << "  continue (c)                            resume execution\n"
        << "  step (s)                                single-step one instruction\n"
        << "  print <symbol|0xADDR> (p)               read a global/address\n"
        << "  watch <symbol|0xADDR>                   alias of print\n"
        << "  backtrace (bt, callstack)               stack trace\n"
        << "  registers (r)                           show registers\n"
        << "  threads                                 list threads\n"
        << "  help (h)                                this help\n"
        << "  quit (q)                                terminate the debuggee\n";
}

void do_continue() {
    if (g.pending_breakpoint >= 0) {
        // Single-step over the restored instruction, then re-arm.
        CONTEXT ctx{};
        if (get_thread_context(ctx, CONTEXT_CONTROL)) {
            ctx.EFlags |= 0x100;
            set_thread_context(ctx, CONTEXT_CONTROL);
        }
        g.step_mode = false;
    }
    g.stopped = false;
}

void do_step() {
    if (g.pending_breakpoint >= 0) {
        CONTEXT ctx{};
        if (get_thread_context(ctx, CONTEXT_CONTROL)) {
            ctx.EFlags |= 0x100;
            set_thread_context(ctx, CONTEXT_CONTROL);
        }
    }
    g.step_mode = true;
    g.stopped = false;
}

uint64_t get_reg_value(const std::string& name, CONTEXT& ctx) {
    if (iequals(name, "rip")) return ctx.Rip;
    if (iequals(name, "rsp")) return ctx.Rsp;
    if (iequals(name, "rbp")) return ctx.Rbp;
    if (iequals(name, "rax")) return ctx.Rax;
    if (iequals(name, "rbx")) return ctx.Rbx;
    if (iequals(name, "rcx")) return ctx.Rcx;
    if (iequals(name, "rdx")) return ctx.Rdx;
    if (iequals(name, "rsi")) return ctx.Rsi;
    if (iequals(name, "rdi")) return ctx.Rdi;
    if (iequals(name, "r8")) return ctx.R8;
    if (iequals(name, "r9")) return ctx.R9;
    if (iequals(name, "r10")) return ctx.R10;
    if (iequals(name, "r11")) return ctx.R11;
    if (iequals(name, "r12")) return ctx.R12;
    if (iequals(name, "r13")) return ctx.R13;
    if (iequals(name, "r14")) return ctx.R14;
    if (iequals(name, "r15")) return ctx.R15;
    return 0;
}

static std::string trim_copy(const std::string& s) {
    size_t a = s.find_first_not_of(" \t");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t");
    return s.substr(a, b - a + 1);
}

struct CondEval {
    const std::string& s;
    size_t pos = 0;
    CONTEXT& ctx;
    bool ok = true;

    CondEval(const std::string& text, CONTEXT& context) : s(text), ctx(context) {}

    void skip() {
        while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) ++pos;
    }
    bool match(const char* op) {
        skip();
        size_t n = std::strlen(op);
        if (s.compare(pos, n, op) == 0) { pos += n; return true; }
        return false;
    }
    int64_t parse_or() {
        int64_t l = parse_and();
        while (match("||")) { int64_t r = parse_and(); l = (l != 0 || r != 0); }
        return l;
    }
    int64_t parse_and() {
        int64_t l = parse_cmp();
        while (match("&&")) { int64_t r = parse_cmp(); l = (l != 0 && r != 0); }
        return l;
    }
    int64_t parse_cmp() {
        int64_t l = parse_add();
        for (;;) {
            if (match("==")) l = (l == parse_add());
            else if (match("!=")) l = (l != parse_add());
            else if (match("<=")) l = (l <= parse_add());
            else if (match(">=")) l = (l >= parse_add());
            else if (match("<")) l = (l < parse_add());
            else if (match(">")) l = (l > parse_add());
            else break;
        }
        return l;
    }
    int64_t parse_add() {
        int64_t l = parse_mul();
        for (;;) {
            if (match("+")) l += parse_mul();
            else if (match("-")) l -= parse_mul();
            else break;
        }
        return l;
    }
    int64_t parse_mul() {
        int64_t l = parse_unary();
        for (;;) {
            if (match("*")) l *= parse_unary();
            else if (match("/")) { int64_t r = parse_unary(); l = r ? l / r : 0; }
            else if (match("%")) { int64_t r = parse_unary(); l = r ? l % r : 0; }
            else break;
        }
        return l;
    }
    int64_t parse_unary() {
        skip();
        if (match("-")) return -parse_unary();
        if (match("!")) return parse_unary() == 0;
        if (match("+")) return parse_unary();
        return parse_primary();
    }
    int64_t parse_primary() {
        skip();
        if (match("(")) { int64_t v = parse_or(); match(")"); return v; }
        if (pos < s.size() && s[pos] == '[') {
            size_t end = s.find(']', pos);
            if (end == std::string::npos) { ok = false; return 0; }
            std::string inside = s.substr(pos + 1, end - pos - 1);
            pos = end + 1;
            CondEval sub(inside, ctx);
            int64_t addr = sub.parse_or();
            uint64_t value = 0;
            if (!read_memory(static_cast<uint64_t>(addr), &value, sizeof(value))) {
                ok = false;
                return 0;
            }
            return static_cast<int64_t>(value);
        }
        size_t start = pos;
        while (pos < s.size() &&
               (std::isalnum(static_cast<unsigned char>(s[pos])) || s[pos] == '_')) {
            ++pos;
        }
        std::string token = s.substr(start, pos - start);
        if (token.empty()) { ok = false; return 0; }
        char* endp = nullptr;
        long long num = std::strtoll(token.c_str(), &endp, 0);
        if (endp && *endp == '\0') return static_cast<int64_t>(num);
        return static_cast<int64_t>(get_reg_value(token, ctx));
    }
};

bool eval_condition(const std::string& condition, CONTEXT& ctx) {
    std::string s = trim_copy(condition);
    if (s.empty()) return true;
    CondEval e(s, ctx);
    int64_t v = e.parse_or();
    return e.ok && v != 0;
}

void print_locals() {
    CONTEXT ctx{};
    if (!get_thread_context(ctx, CONTEXT_CONTROL | CONTEXT_INTEGER)) {
        std::cerr << "Could not read context.\n";
        return;
    }
    alignas(SYMBOL_INFO) unsigned char buffer[sizeof(SYMBOL_INFO) + 512];
    auto* sym = reinterpret_cast<SYMBOL_INFO*>(buffer);
    std::memset(buffer, 0, sizeof(buffer));
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen = 511;
    DWORD64 displacement = 0;
    std::cout << "Function: ";
    if (SymFromAddr(GetCurrentProcess(), ctx.Rip, &displacement, sym)) {
        std::cout << sym->Name << "\n";
    } else {
        std::cout << "?\n";
    }
    std::cout << "Stack (RSP=0x" << std::hex << ctx.Rsp << std::dec << "):\n";
    for (int i = 0; i < 8; ++i) {
        uint64_t value = 0;
        if (read_memory(ctx.Rsp + static_cast<uint64_t>(i) * 8, &value, sizeof(value))) {
            std::cout << "  [rsp+" << (i * 8) << "] = 0x" << std::hex << value
                      << std::dec << "\n";
        }
    }

    // Source-level local names from the PDB (via llvm-pdbutil). Values are
    // approximate stack reads; a full PDB local-offset decoder is not used.
    std::string pdb_func_name;
    {
        alignas(SYMBOL_INFO) unsigned char buffer[sizeof(SYMBOL_INFO) + 512];
        auto* sym = reinterpret_cast<SYMBOL_INFO*>(buffer);
        std::memset(buffer, 0, sizeof(buffer));
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 511;
        DWORD64 displacement = 0;
        if (SymFromAddr(GetCurrentProcess(), ctx.Rip, &displacement, sym)) {
            pdb_func_name = sym->Name;
        }
    }

    std::string tool = bfc::find_llvm_tool("llvm-pdbutil.exe");
    std::string pdb_path = g.imagePath;
    size_t dot = pdb_path.find_last_of('.');
    if (dot != std::string::npos) pdb_path = pdb_path.substr(0, dot) + ".pdb";
    else pdb_path += ".pdb";
    std::string bat_path = "_bfc_pdb.bat";
    {
        std::ofstream bat(bat_path);
        bat << "@echo off\n\"" << tool << "\" dump -symbols \"" << pdb_path
            << "\"\n";
    }
    FILE* pipe = _popen(bat_path.c_str(), "r");
    if (pipe) {
        std::cout << "PDB locals:\n";
        char line[2048];
        int index = 0;
        bool in_target = false;
        while (index < 20 && std::fgets(line, sizeof(line), pipe)) {
            if (std::strstr(line, "S_GPROC32") ||
                std::strstr(line, "S_LPROC32")) {
                in_target = !pdb_func_name.empty() &&
                            std::strstr(line, pdb_func_name.c_str()) != nullptr;
                continue;
            }
            if (!in_target) continue;
            if (!std::strstr(line, "S_LOCAL")) continue;
            char* p = std::strchr(line, '`');
            if (!p) continue;
            char* q = std::strchr(p + 1, '`');
            if (!q) continue;
            *q = '\0';
            std::cout << "  " << (p + 1);
            uint64_t value = 0;
            if (read_memory(ctx.Rsp + static_cast<uint64_t>(index) * 8,
                            &value, sizeof(value))) {
                std::cout << " ~ 0x" << std::hex << value << std::dec;
            }
            std::cout << "\n";
            ++index;
        }
        _pclose(pipe);
    }
    std::remove(bat_path.c_str());
}

void print_watches() {
    if (g.watches.empty()) return;
    CONTEXT ctx{};
    if (!get_thread_context(ctx, CONTEXT_CONTROL | CONTEXT_INTEGER)) return;
    for (size_t i = 0; i < g.watches.size(); ++i) {
        CondEval e(g.watches[i], ctx);
        int64_t value = e.parse_or();
        std::cout << "  watch[" << i << "] " << g.watches[i] << " = " << value
                  << "\n";
    }
}

void repl() {
    std::string line;
    static std::istringstream script_stream(g_script);
    std::istream& input = g_script.empty() ? std::cin : script_stream;
    while (g.running && g.stopped) {
        std::cout << "(bfdbg) " << std::flush;
        if (!std::getline(input, line)) {
            g.running = false;
            g.stopped = false;
            return;
        }
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        if (command.empty()) continue;

        if (command == "continue" || command == "c") {
            do_continue();
        } else if (command == "step" || command == "s") {
            do_step();
        } else if (command == "break" || command == "b") {
            std::string rest;
            std::getline(iss, rest);
            std::istringstream bs(rest);
            std::string spec;
            bs >> spec;
            if (spec.empty()) {
                std::cerr << "Usage: break <function|file:line> [if <cond>]\n";
                continue;
            }
            std::string condition;
            std::string word;
            if (bs >> word && iequals(word, "if")) {
                std::getline(bs, condition);
                condition = trim_copy(condition);
            }
            std::string name;
            uint64_t address = 0;
            if (parse_break_spec(spec, name, address)) {
                set_breakpoint(name, address, condition);
            }
        } else if (command == "locals" || command == "stack") {
            print_locals();
        } else if (command == "print" || command == "p") {
            std::string name;
            iss >> name;
            if (name.empty()) {
                std::cerr << "Usage: print <symbol|0xADDR>\n";
            } else {
                print_symbol(name);
            }
        } else if (command == "backtrace" || command == "bt" || command == "callstack") {
            print_backtrace();
        } else if (command == "registers" || command == "r") {
            print_registers();
        } else if (command == "threads") {
            list_threads();
        } else if (command == "delete" || command == "d") {
            int n = 0;
            if (iss >> n) {
                if (n >= 1 && n <= static_cast<int>(g.breakpoints.size())) {
                    g.breakpoints.erase(g.breakpoints.begin() + (n - 1));
                    rearm_all();
                    std::cout << "Breakpoint " << n << " deleted.\n";
                } else {
                    std::cerr << "No such breakpoint: " << n << "\n";
                }
            } else {
                std::cerr << "Usage: delete <n>\n";
            }
        } else if (command == "disable") {
            int n = 0;
            if (iss >> n && n >= 1 && n <= static_cast<int>(g.breakpoints.size())) {
                g.breakpoints[n - 1].enabled = false;
                rearm_all();
            }
        } else if (command == "enable") {
            int n = 0;
            if (iss >> n && n >= 1 && n <= static_cast<int>(g.breakpoints.size())) {
                g.breakpoints[n - 1].enabled = true;
                rearm_all();
            }
        } else if (command == "watch") {
            std::string name;
            iss >> name;
            if (name.empty()) std::cerr << "Usage: watch <address>\n";
            else print_symbol(name);
        } else if (command == "setreg") {
            std::string name;
            std::string value;
            iss >> name >> value;
            if (name.empty() || value.empty()) {
                std::cerr << "Usage: setreg <name> <value>\n";
            } else {
                CONTEXT ctx{};
                if (get_thread_context(ctx, CONTEXT_CONTROL | CONTEXT_INTEGER)) {
                    uint64_t v = std::strtoull(value.c_str(), nullptr, 0);
                    bool ok = true;
                    if (iequals(name, "rip")) ctx.Rip = v;
                    else if (iequals(name, "rsp")) ctx.Rsp = v;
                    else if (iequals(name, "rbp")) ctx.Rbp = v;
                    else if (iequals(name, "rax")) ctx.Rax = v;
                    else if (iequals(name, "rbx")) ctx.Rbx = v;
                    else if (iequals(name, "rcx")) ctx.Rcx = v;
                    else if (iequals(name, "rdx")) ctx.Rdx = v;
                    else if (iequals(name, "rsi")) ctx.Rsi = v;
                    else if (iequals(name, "rdi")) ctx.Rdi = v;
                    else if (iequals(name, "r8")) ctx.R8 = v;
                    else if (iequals(name, "r9")) ctx.R9 = v;
                    else if (iequals(name, "r10")) ctx.R10 = v;
                    else if (iequals(name, "r11")) ctx.R11 = v;
                    else if (iequals(name, "r12")) ctx.R12 = v;
                    else if (iequals(name, "r13")) ctx.R13 = v;
                    else if (iequals(name, "r14")) ctx.R14 = v;
                    else if (iequals(name, "r15")) ctx.R15 = v;
                    else ok = false;
                    if (ok) {
                        set_thread_context(ctx, CONTEXT_CONTROL | CONTEXT_INTEGER);
                        std::cout << name << " = 0x" << std::hex << v << std::dec << "\n";
                    } else {
                        std::cerr << "Unknown register: " << name << "\n";
                    }
                }
            }
        } else if (command == "disasm") {
            std::string tool = bfc::find_llvm_tool("llvm-objdump.exe");
            std::string cmd = "\"" + tool + "\" -d --no-show-raw-insn \"" +
                              g.imagePath + "\"";
            FILE* pipe = _popen(cmd.c_str(), "r");
            if (pipe) {
                char buffer[1024];
                while (std::fgets(buffer, sizeof(buffer), pipe)) {
                    std::cout << buffer;
                }
                _pclose(pipe);
            } else {
                std::cerr << "Failed to run llvm-objdump\n";
            }
        } else if (command == "watch") {
            std::string expr;
            std::getline(iss, expr);
            if (!expr.empty() && expr[0] == ' ') expr.erase(0, 1);
            if (expr.empty()) {
                std::cerr << "Usage: watch <expr>\n";
            } else {
                g.watches.push_back(expr);
                std::cout << "Watch " << (g.watches.size() - 1) << ": " << expr
                          << "\n";
            }
        } else if (command == "unwatch") {
            int n = 0;
            if (iss >> n && n >= 0 && n < static_cast<int>(g.watches.size())) {
                g.watches.erase(g.watches.begin() + n);
                std::cout << "Watch " << n << " removed.\n";
            } else {
                std::cerr << "Usage: unwatch <n>\n";
            }
        } else if (command == "help" || command == "h") {
            print_help();
        } else if (command == "quit" || command == "q") {
            g.running = false;
            g.stopped = false;
            TerminateProcess(g.process, 0);
            return;
        } else {
            std::cerr << "Unknown command: " << command << " (try 'help')\n";
        }
    }
}

void handle_exception(DEBUG_EVENT& event, DWORD& continue_status) {
    const DWORD code = event.u.Exception.ExceptionRecord.ExceptionCode;
    continue_status = DBG_CONTINUE;

    if (code == EXCEPTION_BREAKPOINT) {
        if (event.dwThreadId != g.mainThreadId) return;
        CONTEXT ctx{};
        if (!get_thread_context(ctx, CONTEXT_CONTROL)) return;
        const uint64_t rip = ctx.Rip;
        int hit = -1;
        for (size_t i = 0; i < g.breakpoints.size(); ++i) {
            if (g.breakpoints[i].address == rip - 1) { hit = static_cast<int>(i); break; }
        }
        if (hit < 0) return; // system breakpoint
        auto& bp = g.breakpoints[hit];
        CONTEXT full{};
        if (get_thread_context(full, CONTEXT_CONTROL | CONTEXT_INTEGER) &&
            !bp.condition.empty() && !eval_condition(bp.condition, full)) {
            // Conditional breakpoint not taken: step over the instruction and
            // re-arm automatically.
            full.Rip = bp.address;
            full.EFlags |= 0x100;
            set_thread_context(full, CONTEXT_CONTROL);
            g.pending_breakpoint = hit;
            g.step_mode = false;
            continue_status = DBG_CONTINUE;
            return;
        }
        disarm(bp);
        ctx.Rip = bp.address;
        set_thread_context(ctx, CONTEXT_CONTROL);
        g.pending_breakpoint = hit;
        std::cout << "\nBreakpoint " << (hit + 1) << ": " << bp.name << "\n";
        g.stopped = true;
        print_watches();
        return;
    }

    if (code == EXCEPTION_SINGLE_STEP) {
        if (event.dwThreadId != g.mainThreadId) return;
        CONTEXT ctx{};
        if (get_thread_context(ctx, CONTEXT_CONTROL)) {
            ctx.EFlags &= ~0x100ULL;
            set_thread_context(ctx, CONTEXT_CONTROL);
        }
        if (g.pending_breakpoint >= 0) {
            arm(g.breakpoints[g.pending_breakpoint]);
            g.pending_breakpoint = -1;
        }
        if (g.step_mode) {
            g.step_mode = false;
            std::cout << "\nStep complete.\n";
            print_watches();
            g.stopped = true;
        }
        return;
    }
}

} // namespace

int run_debugger(const std::string& executable,
                 const std::vector<std::string>& arguments,
                 const std::string& script_path) {
    g = DebuggerState{};
    g_script.clear();
    if (!script_path.empty()) {
        std::ifstream script(script_path);
        if (!script) {
            std::cerr << "Cannot read debugger script: " << script_path << "\n";
            return 1;
        }
        std::stringstream buffer;
        buffer << script.rdbuf();
        g_script = buffer.str();
    }
    g.imagePath = executable;

    if (!SymInitialize(GetCurrentProcess(), nullptr, TRUE)) {
        std::cerr << "SymInitialize failed: " << GetLastError() << "\n";
        return 1;
    }
    SymSetOptions(SymGetOptions() | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

    std::string command_line = "\"" + executable + "\"";
    for (const auto& arg : arguments) command_line += " \"" + arg + "\"";
    std::vector<char> mutable_command(command_line.begin(), command_line.end());
    mutable_command.push_back('\0');
    std::string mutable_exe = executable;

    STARTUPINFOA startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION info{};
    if (!CreateProcessA(mutable_exe.data(), mutable_command.data(), nullptr, nullptr,
                        FALSE, DEBUG_ONLY_THIS_PROCESS, nullptr, nullptr, &startup,
                        &info)) {
        std::cerr << "CreateProcess failed: " << GetLastError() << " exe=\""
                  << mutable_exe << "\"\n";
        SymCleanup(GetCurrentProcess());
        return 1;
    }

    g.process = info.hProcess;
    g.thread = info.hThread;
    g.pid = info.dwProcessId;
    g.mainThreadId = info.dwThreadId;

    std::cout << "BasicForge Debugger\n"
              << "Debugging: " << executable << "\n"
              << "Type 'help' for commands.\n";

    bool symbols_loaded = false;
    while (g.running) {
        DEBUG_EVENT event{};
        if (!WaitForDebugEvent(&event, INFINITE)) break;

        DWORD continue_status = DBG_CONTINUE;
        bool resume = true;

        switch (event.dwDebugEventCode) {
            case CREATE_PROCESS_DEBUG_EVENT:
                g.process = event.u.CreateProcessInfo.hProcess;
                g.thread = event.u.CreateProcessInfo.hThread;
                g.pid = event.dwProcessId;
                g.mainThreadId = event.dwThreadId;
                g.moduleBase = reinterpret_cast<uint64_t>(
                    event.u.CreateProcessInfo.lpBaseOfImage);
                if (!symbols_loaded) {
                    SymLoadModuleEx(GetCurrentProcess(), g.process,
                                    g.imagePath.c_str(), nullptr, g.moduleBase, 0,
                                    nullptr, 0);
                    symbols_loaded = true;
                }
                std::cout << "Process created. Set breakpoints with 'break'.\n";
                g.stopped = true;
                break;
            case EXCEPTION_DEBUG_EVENT:
                handle_exception(event, continue_status);
                break;
            case EXIT_PROCESS_DEBUG_EVENT:
                std::cout << "\nDebuggee exited with code "
                          << event.u.ExitProcess.dwExitCode << "\n";
                g.running = false;
                resume = false;
                break;
            default:
                break;
        }

        if (g.stopped && g.running) repl();
        if (resume) ContinueDebugEvent(g.pid, event.dwThreadId, continue_status);
    }

    for (auto& bp : g.breakpoints) disarm(bp);
    if (!g_script.empty() && g.process) TerminateProcess(g.process, 0);
    if (g.process) CloseHandle(g.process);
    if (g.thread) CloseHandle(g.thread);
    SymCleanup(GetCurrentProcess());
    return 0;
}

} // namespace bfc