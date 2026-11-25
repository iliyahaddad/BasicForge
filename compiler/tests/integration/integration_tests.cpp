// Standalone end-to-end integration tests.
//
// The test locates the compiler through the BFC_EXE environment variable
// (set by CTest) and falls back to a few common relative locations so it can
// also be run directly from a build directory.
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <process.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static std::string find_bfc() {
    if (const char* env = std::getenv("BFC_EXE")) {
        if (env[0] != '\0' && fs::exists(env)) return env;
    }
    const char* candidates[] = {
        "bfc.exe",
        "Release/bfc.exe",
        "../Release/bfc.exe",
        "../../Release/bfc.exe",
        "../../../Release/bfc.exe",
    };
    for (const char* c : candidates) {
        if (fs::exists(c)) return fs::absolute(c).string();
    }
    return "bfc.exe";
}

static int run_compiler(const std::string& bfc, const std::string& src,
                        const std::string& exe) {
    std::string a0 = bfc;
    std::string a1 = src;
    std::string a2 = "-o";
    std::string a3 = exe;
    char* argv[] = {a0.data(), a1.data(), a2.data(), a3.data(), nullptr};
    return _spawnvp(_P_WAIT, bfc.c_str(), argv);
}

static bool compile_and_run(const std::string& bfc, const std::string& name,
                            const std::string& source, const std::string& expected) {
    const fs::path dir = fs::temp_directory_path() / ("bfc_it_" + name);
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir);

    const fs::path src = dir / (name + ".bf");
    const fs::path exe = dir / (name + ".exe");
    {
        std::ofstream out(src);
        out << source;
    }

    if (run_compiler(bfc, src.string(), exe.string()) != 0) {
        std::cerr << name << ": compilation failed\n";
        return false;
    }
    if (!fs::exists(exe)) {
        std::cerr << name << ": executable was not produced\n";
        return false;
    }

    // _popen goes through cmd.exe, so quote the single executable path.
    const std::string run_cmd = "\"" + exe.string() + "\"";
    FILE* pipe = _popen(run_cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << name << ": failed to run executable\n";
        return false;
    }
    std::string output;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) output += buffer;
    _pclose(pipe);

    if (output.find(expected) == std::string::npos) {
        std::cerr << name << ": expected '" << expected << "', got '" << output << "'\n";
        return false;
    }
    std::cout << name << ": ok\n";
    return true;
}

int main() {
    const std::string bfc = find_bfc();

    if (!compile_and_run(bfc, "hello",
            "Module Test\n"
            "Sub Main()\n"
            "    Print(\"Hello World\")\n"
            "End Sub\n"
            "End Module\n",
            "Hello World")) {
        return 1;
    }

    if (!compile_and_run(bfc, "classes",
            "Module Test\n"
            "    Class Counter\n"
            "        Public Value As Integer\n"
            "        Public Sub Add(ByVal n As Integer)\n"
            "            Value = Value + n\n"
            "        End Sub\n"
            "        Public Function Get() As Integer\n"
            "            Return Value\n"
            "        End Function\n"
            "    End Class\n"
            "    Sub Main()\n"
            "        Dim c As New Counter()\n"
            "        c.Add(7)\n"
            "        c.Add(5)\n"
            "        Print(c.Get())\n"
            "    End Sub\n"
            "End Module\n",
            "12")) {
        return 1;
    }

    if (!compile_and_run(bfc, "concat",
            "Module Test\n"
            "    Sub Main()\n"
            "        Dim n As Integer\n"
            "        n = 42\n"
            "        Print(\"n=\" & n)\n"
            "    End Sub\n"
            "End Module\n",
            "n=42")) {
        return 1;
    }

    std::cout << "All integration tests passed!\n";
    return 0;
}