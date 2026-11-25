// Minimal Language Server Protocol server for BasicForge.
//
// Supported: initialize/shutdown/exit, didOpen/didChange/didSave/didClose with
// full-text sync and diagnostics, completion (keywords), hover/definition
// stubs. The protocol is JSON-RPC 2.0 over stdio with Content-Length framing.

#include "bf/lsp_server.h"

#include "bf/bfc.h"
#include "bf/diagnostic.h"
#include "bf/lexer.h"
#include "bf/parser.h"
#include "bf/semantic.h"

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace bfc {
namespace {

// ---------------------------------------------------------------------------
// Minimal JSON value + parser (only what LSP needs)
// ---------------------------------------------------------------------------

struct Json {
    enum class Type { Null, Bool, Number, String, Array, Object };

    Type type = Type::Null;
    bool boolean = false;
    double number = 0.0;
    std::string string;
    std::vector<Json> array;
    std::vector<std::pair<std::string, Json>> object;

    const Json* find(const std::string& key) const {
        for (const auto& kv : object) {
            if (kv.first == key) return &kv.second;
        }
        return nullptr;
    }

    std::string as_string(const std::string& fallback = "") const {
        return type == Type::String ? string : fallback;
    }

    int64_t as_int(int64_t fallback = 0) const {
        if (type == Type::Number) return static_cast<int64_t>(number);
        return fallback;
    }
};

class JsonParser {
public:
    explicit JsonParser(const std::string& text) : text_(text) {}

    bool parse(Json& out) {
        skip_ws();
        if (!parse_value(out)) return false;
        skip_ws();
        return pos_ == text_.size();
    }

private:
    const std::string& text_;
    size_t pos_ = 0;

    void skip_ws() {
        while (pos_ < text_.size() &&
               std::isspace(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }

    bool parse_value(Json& out) {
        skip_ws();
        if (pos_ >= text_.size()) return false;
        const char c = text_[pos_];
        if (c == '{') return parse_object(out);
        if (c == '[') return parse_array(out);
        if (c == '"') {
            out.type = Json::Type::String;
            return parse_string(out.string);
        }
        if (c == 't' || c == 'f') return parse_bool(out);
        if (c == 'n') return parse_null(out);
        return parse_number(out);
    }

    bool parse_object(Json& out) {
        out.type = Json::Type::Object;
        ++pos_; // {
        skip_ws();
        if (pos_ < text_.size() && text_[pos_] == '}') {
            ++pos_;
            return true;
        }
        while (pos_ < text_.size()) {
            skip_ws();
            std::string key;
            if (!parse_string(key)) return false;
            skip_ws();
            if (pos_ >= text_.size() || text_[pos_] != ':') return false;
            ++pos_;
            Json value;
            if (!parse_value(value)) return false;
            out.object.emplace_back(std::move(key), std::move(value));
            skip_ws();
            if (pos_ < text_.size() && text_[pos_] == ',') {
                ++pos_;
                continue;
            }
            if (pos_ < text_.size() && text_[pos_] == '}') {
                ++pos_;
                return true;
            }
            return false;
        }
        return false;
    }

    bool parse_array(Json& out) {
        out.type = Json::Type::Array;
        ++pos_; // [
        skip_ws();
        if (pos_ < text_.size() && text_[pos_] == ']') {
            ++pos_;
            return true;
        }
        while (pos_ < text_.size()) {
            Json value;
            if (!parse_value(value)) return false;
            out.array.push_back(std::move(value));
            skip_ws();
            if (pos_ < text_.size() && text_[pos_] == ',') {
                ++pos_;
                continue;
            }
            if (pos_ < text_.size() && text_[pos_] == ']') {
                ++pos_;
                return true;
            }
            return false;
        }
        return false;
    }

    bool parse_string(std::string& out) {
        if (pos_ >= text_.size() || text_[pos_] != '"') return false;
        ++pos_;
        while (pos_ < text_.size()) {
            const char c = text_[pos_++];
            if (c == '"') return true;
            if (c != '\\') {
                out += c;
                continue;
            }
            if (pos_ >= text_.size()) return false;
            const char esc = text_[pos_++];
            switch (esc) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    if (pos_ + 4 > text_.size()) return false;
                    unsigned code = 0;
                    for (int i = 0; i < 4; ++i) {
                        const char h = text_[pos_++];
                        code <<= 4;
                        if (h >= '0' && h <= '9') code |= static_cast<unsigned>(h - '0');
                        else if (h >= 'a' && h <= 'f') code |= static_cast<unsigned>(h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') code |= static_cast<unsigned>(h - 'A' + 10);
                        else return false;
                    }
                    if (code < 0x80) {
                        out += static_cast<char>(code);
                    } else if (code < 0x800) {
                        out += static_cast<char>(0xC0 | (code >> 6));
                        out += static_cast<char>(0x80 | (code & 0x3F));
                    } else {
                        out += static_cast<char>(0xE0 | (code >> 12));
                        out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                        out += static_cast<char>(0x80 | (code & 0x3F));
                    }
                    break;
                }
                default:
                    return false;
            }
        }
        return false;
    }

    bool parse_bool(Json& out) {
        if (text_.compare(pos_, 4, "true") == 0) {
            pos_ += 4;
            out.type = Json::Type::Bool;
            out.boolean = true;
            return true;
        }
        if (text_.compare(pos_, 5, "false") == 0) {
            pos_ += 5;
            out.type = Json::Type::Bool;
            out.boolean = false;
            return true;
        }
        return false;
    }

    bool parse_null(Json& out) {
        if (text_.compare(pos_, 4, "null") == 0) {
            pos_ += 4;
            out.type = Json::Type::Null;
            return true;
        }
        return false;
    }

    bool parse_number(Json& out) {
        const size_t start = pos_;
        while (pos_ < text_.size()) {
            const char c = text_[pos_];
            if (std::isdigit(static_cast<unsigned char>(c)) || c == '-' || c == '+' ||
                c == '.' || c == 'e' || c == 'E') {
                ++pos_;
            } else {
                break;
            }
        }
        if (start == pos_) return false;
        out.type = Json::Type::Number;
        try {
            out.number = std::stod(text_.substr(start, pos_ - start));
        } catch (...) {
            return false;
        }
        return true;
    }
};

std::string json_escape(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (char c : input) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// Diagnostics
// ---------------------------------------------------------------------------

class LspDiagnosticSink : public DiagnosticSink {
public:
    std::vector<Diagnostic> diagnostics;

    void emit(const Diagnostic& d) override {
        diagnostics.push_back(d);
        if (d.severity == DiagnosticSeverity::Error) had_error_ = true;
    }
};

std::string diagnostics_json(const std::vector<Diagnostic>& diagnostics) {
    std::string out = "[";
    bool first = true;
    for (const auto& d : diagnostics) {
        if (d.severity != DiagnosticSeverity::Error &&
            d.severity != DiagnosticSeverity::Warning) {
            continue;
        }
        if (!first) out += ",";
        first = false;

        int64_t line = d.location.line > 0 ? static_cast<int64_t>(d.location.line) - 1 : 0;
        int64_t column = d.location.column > 0 ? static_cast<int64_t>(d.location.column) - 1 : 0;
        int severity = d.severity == DiagnosticSeverity::Error ? 1 : 2;

        out += "{\"range\":{\"start\":{\"line\":";
        out += std::to_string(line);
        out += ",\"character\":";
        out += std::to_string(column);
        out += "},\"end\":{\"line\":";
        out += std::to_string(line);
        out += ",\"character\":";
        out += std::to_string(column + 1);
        out += "}},\"severity\":";
        out += std::to_string(severity);
        out += ",\"source\":\"BasicForge\",\"message\":\"";
        out += json_escape(d.message);
        out += "\"}";
    }
    out += "]";
    return out;
}

// ---------------------------------------------------------------------------
// Protocol helpers
// ---------------------------------------------------------------------------

bool read_message(std::istream& in, std::string& body) {
    std::string line;
    int64_t content_length = -1;

    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;
        const std::string lower_prefix = "content-length:";
        if (line.size() >= lower_prefix.size()) {
            std::string prefix = line.substr(0, lower_prefix.size());
            for (char& c : prefix) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (prefix == lower_prefix) {
                std::string value = line.substr(lower_prefix.size());
                size_t start = value.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    try {
                        content_length = std::stoll(value.substr(start));
                    } catch (...) {
                        content_length = -1;
                    }
                }
            }
        }
    }

    if (content_length < 0) return false;
    body.assign(static_cast<size_t>(content_length), '\0');
    in.read(body.data(), content_length);
    return static_cast<bool>(in) || in.gcount() == content_length;
}

void send_message(std::ostream& out, const std::string& body) {
    out << "Content-Length: " << body.size() << "\r\n\r\n" << body;
    out.flush();
}

void send_response(std::ostream& out, const Json& id, const std::string& result_json) {
    std::string body = "{\"jsonrpc\":\"2.0\",\"id\":";
    if (id.type == Json::Type::Number) {
        body += std::to_string(id.as_int());
    } else {
        body += "null";
    }
    body += ",\"result\":";
    body += result_json;
    body += "}";
    send_message(out, body);
}

void send_notification(std::ostream& out, const std::string& method,
                       const std::string& params_json) {
    std::string body = "{\"jsonrpc\":\"2.0\",\"method\":\"";
    body += method;
    body += "\",\"params\":";
    body += params_json;
    body += "}";
    send_message(out, body);
}

std::string id_json(const Json* id) {
    if (!id) return "null";
    if (id->type == Json::Type::Number) return std::to_string(id->as_int());
    if (id->type == Json::Type::String) return "\"" + json_escape(id->string) + "\"";
    return "null";
}

static PrimitiveType type_from_name(std::string name) {
    std::string lower = name;
    for (auto& c : lower) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (lower == "string") return PrimitiveType::String;
    if (lower == "double") return PrimitiveType::Double;
    if (lower == "single") return PrimitiveType::Single;
    if (lower == "boolean") return PrimitiveType::Boolean;
    if (lower == "long") return PrimitiveType::Long;
    return PrimitiveType::Integer;
}

bool is_ident_char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

bool iequals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

size_t position_to_offset(const std::string& text, int64_t line, int64_t character) {
    size_t offset = 0;
    int64_t current_line = 0;
    while (offset < text.size() && current_line < line) {
        if (text[offset] == '\n') ++current_line;
        ++offset;
    }
    for (int64_t i = 0; i < character && offset < text.size(); ++i) {
        if (text[offset] == '\n') break;
        ++offset;
    }
    return offset;
}

std::string word_at(const std::string& text, size_t offset, size_t& start,
                    size_t& end) {
    if (offset > text.size()) offset = text.size();
    size_t s = offset;
    while (s > 0 && is_ident_char(text[s - 1])) --s;
    size_t e = offset;
    while (e < text.size() && is_ident_char(text[e])) ++e;
    if (s == e && offset > 0 && is_ident_char(text[offset - 1])) {
        e = offset;
        s = offset - 1;
        while (s > 0 && is_ident_char(text[s - 1])) --s;
    }
    start = s;
    end = e;
    return text.substr(s, e - s);
}

void offset_to_position(const std::string& text, size_t offset, int64_t& line,
                        int64_t& character) {
    line = 0;
    character = 0;
    for (size_t i = 0; i < offset && i < text.size(); ++i) {
        if (text[i] == '\n') {
            ++line;
            character = 0;
        } else {
            ++character;
        }
    }
}

// ---------------------------------------------------------------------------
// Server state
// ---------------------------------------------------------------------------

struct GlobalSymbol {
    std::string name;
    std::string kind;
    std::string type_name;
    std::string uri;
    uint32_t line = 0;
    uint32_t column = 0;
    std::vector<std::pair<std::string, std::string>> params;
    std::vector<std::pair<std::string, std::string>> members;
};

struct LspServer {
    std::ostream& out;
    std::map<std::string, std::string> documents;
    std::map<std::string, std::vector<SymbolInfo>> symbols;
    std::string root_path;
    std::vector<GlobalSymbol> global_symbols;

    explicit LspServer(std::ostream& output) : out(output) {}

    static std::string uri_to_path(const std::string& uri) {
        if (uri.rfind("file:///", 0) == 0) return uri.substr(8);
        if (uri.rfind("file://", 0) == 0) return uri.substr(7);
        return uri;
    }

    static std::string path_to_uri(const std::string& path) {
        std::string p = path;
        for (auto& c : p) if (c == '\\') c = '/';
        return "file:///" + p;
    }

    void index_workspace() {
        global_symbols.clear();
        if (root_path.empty()) return;
        std::error_code ec;
        if (!std::filesystem::exists(root_path, ec)) return;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root_path, ec)) {
            if (ec) break;
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != ".bf") continue;
            std::ifstream file(entry.path());
            if (!file) continue;
            std::string source((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            // Allow bare procedure files (no Module wrapper) in the index.
            size_t first = source.find_first_not_of(" \t\r\n");
            bool has_top_level = false;
            if (first != std::string::npos) {
                size_t end = first;
                while (end < source.size() &&
                       (std::isalnum(static_cast<unsigned char>(source[end])) || source[end] == '_')) {
                    ++end;
                }
                std::string word = source.substr(first, end - first);
                for (auto& c : word) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (word == "module" || word == "namespace" || word == "interface" ||
                    word == "enum" || word == "class" || word == "struct") {
                    has_top_level = true;
                }
            }
            if (!has_top_level) {
                source = "Module __Index\n" + source + "\nEnd Module";
            }
            LspDiagnosticSink sink;
            Lexer lexer(source, entry.path().string());
            Parser parser(lexer, sink);
            auto program = parser.parse();
            (void)program;
            std::string uri = path_to_uri(entry.path().string());
            if (program) {
                for (const auto& mod : program->modules) {
                    for (const auto& proc : mod->procedures) {
                        GlobalSymbol gs;
                        gs.name = proc->name;
                        gs.kind = proc->is_function ? "Function" : "Sub";
                        gs.type_name = primitive_type_name(proc->return_type.kind);
                        gs.uri = uri;
                        for (const auto& sym : parser.symbols()) {
                            if (iequals(sym.name, proc->name) && sym.kind == gs.kind) {
                                gs.line = sym.location.line;
                                gs.column = sym.location.column;
                                break;
                            }
                        }
                        for (const auto& p : proc->parameters) {
                            gs.params.push_back(
                                {p.name, primitive_type_name(p.type.kind)});
                        }
                        global_symbols.push_back(std::move(gs));
                    }
                    for (const auto& cls : mod->classes) {
                        GlobalSymbol gs;
                        gs.name = cls->name;
                        gs.kind = "Class";
                        gs.uri = uri;
                        for (const auto& prop : cls->properties) {
                            gs.members.push_back(
                                {prop.name, primitive_type_name(prop.type.kind)});
                        }
                        for (const auto& sym : parser.symbols()) {
                            if (iequals(sym.name, cls->name) && sym.kind == "Class") {
                                gs.line = sym.location.line;
                                gs.column = sym.location.column;
                                break;
                            }
                        }
                        global_symbols.push_back(std::move(gs));

                        for (const auto& method : cls->methods) {
                            GlobalSymbol ms;
                            ms.name = method.name;
                            ms.kind = "Method";
                            ms.type_name =
                                primitive_type_name(method.return_type.kind);
                            ms.uri = uri;
                            for (const auto& sym : parser.symbols()) {
                                if (iequals(sym.name, method.name) &&
                                    (sym.kind == "Method" ||
                                     sym.kind == "Function")) {
                                    ms.line = sym.location.line;
                                    ms.column = sym.location.column;
                                    break;
                                }
                            }
                            global_symbols.push_back(std::move(ms));
                        }
                    }
                }
            }
            for (const auto& sym : parser.symbols()) {
                bool duplicate = false;
                for (const auto& existing : global_symbols) {
                    if (existing.uri == uri && iequals(existing.name, sym.name) &&
                        existing.kind == sym.kind) {
                        duplicate = true;
                        break;
                    }
                }
                if (duplicate) continue;
                GlobalSymbol gs;
                gs.name = sym.name;
                gs.kind = sym.kind;
                gs.type_name = sym.type_name;
                gs.uri = uri;
                gs.line = sym.location.line;
                gs.column = sym.location.column;
                global_symbols.push_back(std::move(gs));
            }
        }
    }

    void publish(const std::string& uri) {
        auto it = documents.find(uri);
        if (it == documents.end()) return;

        LspDiagnosticSink sink;
        Lexer lexer(it->second, uri);
        Parser parser(lexer, sink);
        auto program = parser.parse();
        if (program && !sink.has_errors()) {
            SemanticAnalyzer analyzer(sink);
            // Register workspace-wide symbols so calls and variable reads in
            // other files type-check correctly.
            for (const auto& sym : global_symbols) {
                if (sym.kind == "Class") {
                    std::unordered_map<std::string, PrimitiveType> members;
                    for (const auto& m : sym.members) {
                        members[m.first] = type_from_name(m.second);
                    }
                    analyzer.register_external_class(sym.name, members);
                } else if (sym.kind == "Function" || sym.kind == "Sub") {
                    analyzer.register_external(sym.name,
                                               type_from_name(sym.type_name), true);
                    analyzer.register_external_method(sym.name,
                                                      type_from_name(sym.type_name));
                } else if (sym.kind == "Method") {
                    analyzer.register_external_method(sym.name,
                                                      type_from_name(sym.type_name));
                } else if (sym.kind == "Variable" || sym.kind == "Const" ||
                           sym.kind == "Parameter" || sym.kind == "Property") {
                    analyzer.register_external(sym.name,
                                               type_from_name(sym.type_name), false);
                }
            }
            analyzer.analyze(*program);
        }
        symbols[uri] = parser.symbols();
        if (std::getenv("BFC_LSP_DEBUG")) {
            for (const auto& sym : symbols[uri]) {
                std::cerr << "SYM " << sym.name << " " << sym.kind << " "
                          << sym.location.line << ":" << sym.location.column << "\n";
            }
        }

        std::string params = "{\"uri\":\"" + json_escape(uri) + "\",\"diagnostics\":";
        params += diagnostics_json(sink.diagnostics);
        params += "}";
        send_notification(out, "textDocument/publishDiagnostics", params);
    }

    void handle(const std::string& body) {
        Json message;
        JsonParser parser(body);
        if (!parser.parse(message) || message.type != Json::Type::Object) return;

        std::string method = "";
        if (const Json* m = message.find("method")) method = m->as_string();
        const Json* id = message.find("id");
        const Json* params = message.find("params");

        auto respond = [&](const std::string& result) {
            if (id) send_response(out, *id, result);
        };

        if (method == "initialize") {
            if (params) {
                std::string root;
                if (const Json* r = params->find("rootUri")) root = r->as_string();
                if (root.empty()) {
                    if (const Json* ws = params->find("workspaceFolders")) {
                        if (!ws->array.empty()) {
                            if (const Json* u = ws->array[0].find("uri")) {
                                root = u->as_string();
                            }
                        }
                    }
                }
                if (!root.empty()) {
                    root_path = uri_to_path(root);
                    index_workspace();
                }
            }
            respond(
                "{\"capabilities\":{"
                "\"textDocumentSync\":1,"
                "\"hoverProvider\":true,"
                "\"completionProvider\":{\"triggerCharacters\":[\".\"]},"
                "\"definitionProvider\":true"
                "}}");
            return;
        }
        if (method == "initialized") {
            respond("null");
            return;
        }
        if (method == "shutdown") {
            respond("null");
            return;
        }
        if (method == "exit") {
            return;
        }

        if (params && method == "textDocument/didOpen") {
            if (const Json* doc = params->find("textDocument")) {
                std::string uri = "";
                std::string text = "";
                if (const Json* u = doc->find("uri")) uri = u->as_string();
                if (const Json* t = doc->find("text")) text = t->as_string();
                if (!uri.empty()) {
                    documents[uri] = text;
                    if (root_path.empty()) {
                        root_path = std::filesystem::path(uri_to_path(uri))
                                        .parent_path().string();
                        index_workspace();
                    }
                    publish(uri);
                }
            }
            return;
        }

        if (params && method == "textDocument/didChange") {
            if (const Json* doc = params->find("textDocument")) {
                std::string uri = "";
                if (const Json* u = doc->find("uri")) uri = u->as_string();
                // Full sync: the last content change contains the whole text.
                std::string text;
                if (const Json* changes = params->find("contentChanges")) {
                    for (const auto& change : changes->array) {
                        if (const Json* t = change.find("text")) text = t->as_string();
                    }
                }
                if (!uri.empty() && !text.empty()) {
                    documents[uri] = text;
                    publish(uri);
                }
            }
            return;
        }

        if (params && method == "textDocument/didSave") {
            if (const Json* doc = params->find("textDocument")) {
                if (const Json* u = doc->find("uri")) {
                    std::string uri = u->as_string();
                    if (!uri.empty()) publish(uri);
                }
            }
            return;
        }

        if (params && method == "textDocument/didClose") {
            if (const Json* doc = params->find("textDocument")) {
                if (const Json* u = doc->find("uri")) {
                    std::string uri = u->as_string();
                    documents.erase(uri);
                    std::string p = "{\"uri\":\"" + json_escape(uri) + "\",\"diagnostics\":[]}";
                    send_notification(out, "textDocument/publishDiagnostics", p);
                }
            }
            return;
        }

        if (method == "textDocument/completion") {
            static const char* keywords[] = {
                "Module", "Sub", "Function", "Class", "Structure", "Enum",
                "Interface", "Namespace", "Dim", "Const", "As", "New", "Me",
                "If", "Then", "Else", "ElseIf", "End", "For", "To", "Step",
                "Each", "In", "While", "Do", "Loop", "Until", "Select", "Case",
                "Try", "Catch", "Finally", "Throw", "Return", "Print", "Input",
                "True", "False", "Nothing", "And", "Or", "Not", "Xor", "Mod",
                "Integer", "Long", "Single", "Double", "String", "Boolean",
                "Public", "Private", "Protected", "Static", "Shared", "ByVal",
                "ByRef", "Optional", "Property", "Get", "Set", "Exit",
            };
            std::string items = "[";
            bool first = true;
            for (const char* kw : keywords) {
                if (!first) items += ",";
                first = false;
                items += "{\"label\":\"";
                items += kw;
                items += "\",\"kind\":14}";
            }
            items += "]";
            respond("{\"isIncomplete\":false,\"items\":" + items + "}");
            return;
        }

        if (method == "textDocument/hover") {
            std::string uri;
            int64_t line = 0;
            int64_t character = 0;
            if (params) {
                if (const Json* doc = params->find("textDocument")) {
                    if (const Json* u = doc->find("uri")) uri = u->as_string();
                }
                if (const Json* pos = params->find("position")) {
                    if (const Json* l = pos->find("line")) line = l->as_int();
                    if (const Json* c = pos->find("character")) character = c->as_int();
                }
            }
            auto doc_it = documents.find(uri);
            auto sym_it = symbols.find(uri);
            if (doc_it != documents.end() && sym_it != symbols.end()) {
                size_t start = 0;
                size_t end = 0;
                std::string word = word_at(doc_it->second,
                                           position_to_offset(doc_it->second, line, character),
                                           start, end);
                for (const auto& sym : sym_it->second) {
                    if (!iequals(sym.name, word)) continue;
                    std::string value = "**" + sym.kind + " `" + sym.name + "`";
                    if (!sym.type_name.empty()) value += " : " + sym.type_name;
                    value += "\n\nLine " + std::to_string(sym.location.line);
                    respond("{\"contents\":{\"kind\":\"markdown\",\"value\":\"" +
                            json_escape(value) + "\"}}");
                    return;
                }
                // Class members (properties/fields) across files.
                for (const auto& sym : global_symbols) {
                    if (sym.kind != "Class") continue;
                    for (const auto& m : sym.members) {
                        if (!iequals(m.first, word)) continue;
                        std::string value = "**" + m.first + "** : " + m.second +
                                            "\n\nClass " + sym.name;
                        respond("{\"contents\":{\"kind\":\"markdown\",\"value\":\"" +
                                json_escape(value) + "\"}}");
                        return;
                    }
                }
                // Fall back to the workspace-wide symbol index.
                const GlobalSymbol* best = nullptr;
                for (const auto& sym : global_symbols) {
                    if (!iequals(sym.name, word)) continue;
                    if (!best) {
                        best = &sym;
                    } else if (!sym.params.empty() && best->params.empty()) {
                        best = &sym;
                    } else if (!sym.members.empty() && best->members.empty()) {
                        best = &sym;
                    }
                }
                if (best) {
                    std::string sig = best->name;
                    if (!best->params.empty()) {
                        sig += "(";
                        for (size_t i = 0; i < best->params.size(); ++i) {
                            if (i) sig += ", ";
                            sig += best->params[i].first + " As " + best->params[i].second;
                        }
                        sig += ")";
                    }
                    std::string value = "**" + best->kind + " `" + sig + "`";
                    if (!best->type_name.empty()) value += " : " + best->type_name;
                    if (!best->members.empty()) {
                        value += "\n\nMembers:";
                        for (const auto& m : best->members) {
                            value += "\n- " + m.first + " : " + m.second;
                        }
                    }
                    value += "\n\n" + best->uri + ":" + std::to_string(best->line);
                    respond("{\"contents\":{\"kind\":\"markdown\",\"value\":\"" +
                            json_escape(value) + "\"}}");
                    return;
                }
            }
            respond("null");
            return;
        }

        if (method == "textDocument/definition") {
            std::string uri;
            int64_t line = 0;
            int64_t character = 0;
            if (params) {
                if (const Json* doc = params->find("textDocument")) {
                    if (const Json* u = doc->find("uri")) uri = u->as_string();
                }
                if (const Json* pos = params->find("position")) {
                    if (const Json* l = pos->find("line")) line = l->as_int();
                    if (const Json* c = pos->find("character")) character = c->as_int();
                }
            }
            auto doc_it = documents.find(uri);
            auto sym_it = symbols.find(uri);
            if (doc_it != documents.end() && sym_it != symbols.end()) {
                size_t start = 0;
                size_t end = 0;
                std::string word = word_at(doc_it->second,
                                           position_to_offset(doc_it->second, line, character),
                                           start, end);
                for (const auto& sym : sym_it->second) {
                    if (!iequals(sym.name, word)) continue;
                    int64_t def_line = sym.location.line > 0 ? sym.location.line - 1 : 0;
                    int64_t def_char = sym.location.column > 0
                                           ? sym.location.column - 1 -
                                                 static_cast<int64_t>(sym.name.size())
                                           : 0;
                    if (def_char < 0) def_char = 0;
                    std::string result =
                        "{\"uri\":\"" + json_escape(uri) +
                        "\",\"range\":{\"start\":{\"line\":" + std::to_string(def_line) +
                        ",\"character\":" + std::to_string(def_char) +
                        "},\"end\":{\"line\":" + std::to_string(def_line) +
                        ",\"character\":" + std::to_string(def_char + static_cast<int64_t>(word.size())) +
                        "}}}";
                    respond(result);
                    return;
                }
                // Fall back to the workspace-wide symbol index.
                for (const auto& sym : global_symbols) {
                    if (!iequals(sym.name, word)) continue;
                    int64_t def_line = sym.line > 0 ? sym.line - 1 : 0;
                    int64_t def_char = sym.column > 0
                                           ? sym.column - 1 -
                                                 static_cast<int64_t>(sym.name.size())
                                           : 0;
                    if (def_char < 0) def_char = 0;
                    std::string result =
                        "{\"uri\":\"" + json_escape(sym.uri) +
                        "\",\"range\":{\"start\":{\"line\":" + std::to_string(def_line) +
                        ",\"character\":" + std::to_string(def_char) +
                        "},\"end\":{\"line\":" + std::to_string(def_line) +
                        ",\"character\":" + std::to_string(def_char + static_cast<int64_t>(sym.name.size())) +
                        "}}}";
                    respond(result);
                    return;
                }
            }
            respond("null");
            return;
        }

        if (id) {
            // Unknown request: reply with null so clients do not hang.
            respond("null");
        }
    }
};

} // namespace

int run_lsp_server() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    LspServer server(std::cout);
    std::string body;
    while (read_message(std::cin, body)) {
        server.handle(body);
        // Exit notification is a request-less notification; the client closes
        // stdin afterwards, which ends the loop.
    }
    return 0;
}

} // namespace bfc