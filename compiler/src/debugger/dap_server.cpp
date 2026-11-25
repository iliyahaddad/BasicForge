// Minimal Debug Adapter Protocol (DAP) server for BasicForge.
//
// Supports initialize, setBreakpoints, configurationDone, continue, next,
// stepIn, stackTrace, disconnect. The server launches the debuggee under the
// Windows Debug API and speaks JSON-RPC over stdio with Content-Length framing.

#include "bf/dap_server.h"

#include <windows.h>
#include <dbghelp.h>

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "dbghelp.lib")

namespace bfc {
namespace {

struct Json {
    enum class Type { Null, Bool, Number, String, Array, Object };
    Type type = Type::Null;
    bool boolean = false;
    double number = 0;
    std::string string;
    std::vector<Json> array;
    std::vector<std::pair<std::string, Json>> object;

    const Json* find(const std::string& key) const {
        for (auto& kv : object) if (kv.first == key) return &kv.second;
        return nullptr;
    }
    std::string as_string(const std::string& def = "") const { return type == Type::String ? string : def; }
    int64_t as_int(int64_t def = 0) const { return type == Type::Number ? (int64_t)number : def; }
};

class JsonParser {
public:
    explicit JsonParser(const std::string& text) : s_(text) {}
    bool parse(Json& out) { skip(); return value(out); }
private:
    const std::string& s_;
    size_t pos_ = 0;
    void skip() { while (pos_ < s_.size() && std::isspace((unsigned char)s_[pos_])) pos_++; }
    bool value(Json& out) {
        skip();
        if (pos_ >= s_.size()) return false;
        char c = s_[pos_];
        if (c == '{') return object(out);
        if (c == '[') return array(out);
        if (c == '"') { out.type = Json::Type::String; return string(out.string); }
        if (c == 't' || c == 'f') return boolean(out);
        if (c == 'n') { pos_ += 4; out.type = Json::Type::Null; return true; }
        return number(out);
    }
    bool object(Json& out) {
        out.type = Json::Type::Object; pos_++;
        skip();
        if (pos_ < s_.size() && s_[pos_] == '}') { pos_++; return true; }
        while (pos_ < s_.size()) {
            skip();
            std::string key;
            if (!string(key)) return false;
            skip();
            if (pos_ >= s_.size() || s_[pos_] != ':') return false;
            pos_++;
            Json v;
            if (!value(v)) return false;
            out.object.emplace_back(std::move(key), std::move(v));
            skip();
            if (pos_ < s_.size() && s_[pos_] == ',') { pos_++; continue; }
            if (pos_ < s_.size() && s_[pos_] == '}') { pos_++; return true; }
            return false;
        }
        return false;
    }
    bool array(Json& out) {
        out.type = Json::Type::Array; pos_++;
        skip();
        if (pos_ < s_.size() && s_[pos_] == ']') { pos_++; return true; }
        while (pos_ < s_.size()) {
            Json v;
            if (!value(v)) return false;
            out.array.push_back(std::move(v));
            skip();
            if (pos_ < s_.size() && s_[pos_] == ',') { pos_++; continue; }
            if (pos_ < s_.size() && s_[pos_] == ']') { pos_++; return true; }
            return false;
        }
        return false;
    }
    bool string(std::string& out) {
        if (pos_ >= s_.size() || s_[pos_] != '"') return false;
        pos_++;
        while (pos_ < s_.size()) {
            char c = s_[pos_++];
            if (c == '"') return true;
            if (c == '\\' && pos_ < s_.size()) {
                char e = s_[pos_++];
                switch (e) {
                    case 'n': out += '\n'; break;
                    case 'r': out += '\r'; break;
                    case 't': out += '\t'; break;
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    default: out += e; break;
                }
            } else {
                out += c;
            }
        }
        return false;
    }
    bool boolean(Json& out) {
        if (s_.compare(pos_, 4, "true") == 0) { pos_ += 4; out.type = Json::Type::Bool; out.boolean = true; return true; }
        if (s_.compare(pos_, 5, "false") == 0) { pos_ += 5; out.type = Json::Type::Bool; out.boolean = false; return true; }
        return false;
    }
    bool number(Json& out) {
        size_t start = pos_;
        while (pos_ < s_.size() && (std::isdigit((unsigned char)s_[pos_]) || s_[pos_]=='-' || s_[pos_]=='+' || s_[pos_]=='.' || s_[pos_]=='e' || s_[pos_]=='E')) pos_++;
        if (start == pos_) return false;
        out.type = Json::Type::Number;
        try { out.number = std::stod(s_.substr(start, pos_-start)); } catch (...) { return false; }
        return true;
    }
};

std::string json_escape(const std::string& input) {
    std::string out;
    for (char c : input) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c;
        }
    }
    return out;
}

std::string read_message(std::istream& in) {
    std::string line;
    int64_t content_length = -1;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;
        if (line.rfind("Content-Length:", 0) == 0) {
            content_length = std::atoll(line.c_str() + 15);
        }
    }
    if (content_length < 0) return {};
    std::string body(content_length, '\0');
    in.read(body.data(), content_length);
    return body;
}

void send(std::ostream& out, const std::string& body) {
    out << "Content-Length: " << body.size() << "\r\n\r\n" << body;
    out.flush();
}

void send_response(std::ostream& out, int64_t id, const std::string& result) {
    send(out, "{\"jsonrpc\":\"2.0\",\"id\":" + std::to_string(id) + ",\"result\":" + result + "}");
}

void send_event(std::ostream& out, const std::string& event, const std::string& params) {
    send(out, "{\"jsonrpc\":\"2.0\",\"event\":\"" + event + "\",\"params\":" + params + "}");
}

struct DapState {
    HANDLE process = nullptr;
    HANDLE thread = nullptr;
    DWORD pid = 0;
    DWORD mainThreadId = 0;
    bool running = true;
    bool stopped = false;
    bool step_mode = false;
    std::string exe;
    std::vector<std::pair<std::string, int>> breakpoints;
} g;

static std::string basename_of(const std::string& path) {
    size_t slash = path.find_last_of("\\/");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

struct LineSearch {
    std::string file;
    int line = 0;
    uint64_t address = 0;
};

static BOOL CALLBACK enum_line_symbols(SYMBOL_INFO* sym, ULONG, PVOID context) {
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

void launch(const std::string& exe) {
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::string cmd = "\"" + exe + "\"";
    if (!CreateProcessA(exe.c_str(), cmd.data(), nullptr, nullptr, FALSE,
                        DEBUG_ONLY_THIS_PROCESS, nullptr, nullptr, &si, &pi)) {
        return;
    }
    g.process = pi.hProcess;
    g.thread = pi.hThread;
    g.pid = pi.dwProcessId;
    g.mainThreadId = pi.dwThreadId;
    g.stopped = true;
    SymLoadModuleEx(GetCurrentProcess(), nullptr, exe.c_str(), nullptr, 0, 0, nullptr, 0);
    send_event(std::cout, "stopped", "{\"reason\":\"entry\"}");
}

void handle(const std::string& body) {
    Json msg;
    JsonParser p(body);
    if (!p.parse(msg)) return;
    std::string method = "";
    if (const Json* m = msg.find("method")) method = m->as_string();
    int64_t id = 0;
    bool has_id = false;
    if (const Json* i = msg.find("id")) { id = i->as_int(); has_id = true; }
    const Json* args = msg.find("arguments");

    auto respond = [&](const std::string& result) {
        if (has_id) send_response(std::cout, id, result);
    };

    if (method == "initialize") {
        respond("{\"capabilities\":{\"supportsConfigurationDoneRequest\":true,\"supportsSetBreakpoints\":true}}");
    } else if (method == "setBreakpoints") {
        g.breakpoints.clear();
        if (args) {
            const Json* bps = args->find("breakpoints");
            const Json* src = args->find("source");
            std::string path = src ? src->find("path")->as_string() : "";
            if (bps) {
                for (const auto& bp : bps->array) {
                    g.breakpoints.push_back({path, (int)bp.find("line")->as_int()});
                }
            }
        }
        std::string result = "{\"breakpoints\":[";
        for (size_t i = 0; i < g.breakpoints.size(); ++i) {
            auto& bp = g.breakpoints[i];
            LineSearch search;
            search.file = basename_of(bp.first);
            search.line = bp.second;
            SymEnumSymbols(GetCurrentProcess(), 0, nullptr, enum_line_symbols, &search);
            bool verified = search.address != 0;
            if (verified) {
                CONTEXT ctx{};
                ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                if (GetThreadContext(g.thread, &ctx)) {
                    ctx.Dr0 = search.address;
                    ctx.Dr7 |= 1;
                    SetThreadContext(g.thread, &ctx);
                }
            }
            if (i) result += ",";
            result += "{\"verified\":";
            result += verified ? "true" : "false";
            result += "}";
        }
        result += "]}";
        respond(result);
    } else if (method == "launch" || method == "attach") {
        if (args) {
            if (const Json* p = args->find("program")) g.exe = p->as_string();
        }
        respond("null");
    } else if (method == "configurationDone") {
        if (!g.exe.empty()) launch(g.exe);
        respond("null");
    } else if (method == "continue") {
        g.stopped = false;
        respond("{\"allThreadsContinued\":true}");
    } else if (method == "next" || method == "stepIn") {
        CONTEXT ctx{};
        ctx.ContextFlags = CONTEXT_CONTROL;
        if (GetThreadContext(g.thread, &ctx)) {
            ctx.EFlags |= 0x100;
            SetThreadContext(g.thread, &ctx);
        }
        g.step_mode = true;
        g.stopped = false;
        respond("null");
    } else if (method == "stackTrace") {
        CONTEXT ctx{};
        ctx.ContextFlags = CONTEXT_CONTROL;
        uint64_t rip = 0;
        if (GetThreadContext(g.thread, &ctx)) rip = ctx.Rip;
        char buf[64];
        std::snprintf(buf, sizeof(buf), "0x%llx", (unsigned long long)rip);
        std::string result;
        result = std::string("{\"stackFrames\":[{\"id\":0,\"name\":\"") + buf + "\",\"line\":0,\"column\":0}],\"totalFrames\":1}";
        respond(result);
    } else if (method == "scopes") {
        respond("{\"scopes\":[{\"name\":\"Registers\",\"variablesReference\":1,\"expensive\":false}]}");
    } else if (method == "variables") {
        CONTEXT ctx{};
        ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
        uint64_t rip = 0;
        uint64_t rsp = 0;
        if (GetThreadContext(g.thread, &ctx)) { rip = ctx.Rip; rsp = ctx.Rsp; }
        char buf[64];
        std::snprintf(buf, sizeof(buf), "0x%llx", (unsigned long long)rip);
        std::string result = "{\"variables\":[{\"name\":\"rip\",\"value\":\"" + std::string(buf) + "\"}";
        std::snprintf(buf, sizeof(buf), "0x%llx", (unsigned long long)rsp);
        result += ",{\"name\":\"rsp\",\"value\":\"" + std::string(buf) + "\"}";
        result += "]}";
        respond(result);
    } else if (method == "disconnect" || method == "terminate") {
        if (g.process) TerminateProcess(g.process, 0);
        g.running = false;
        respond("null");
    } else if (has_id) {
        respond("null");
    }
}

} // namespace
int run_dap_server() {
    SymSetOptions(SymGetOptions() | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    SymInitialize(GetCurrentProcess(), nullptr, TRUE);
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    while (g.running) {
        std::string msg = read_message(std::cin);
        if (msg.empty()) break;
        handle(msg);
        while (g.stopped && g.running) {
            std::string cmd = read_message(std::cin);
            if (cmd.empty()) { g.running = false; break; }
            handle(cmd);
        }
    }

    if (g.process) CloseHandle(g.process);
    if (g.thread) CloseHandle(g.thread);
    return 0;
}

} // namespace bfc