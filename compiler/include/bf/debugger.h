#pragma once

#include <string>
#include <vector>

namespace bfc {

// Run the native debugger on an executable.
// Returns a process exit code.
int run_debugger(const std::string& executable,
                 const std::vector<std::string>& arguments,
                 const std::string& script_path = "");

} // namespace bfc