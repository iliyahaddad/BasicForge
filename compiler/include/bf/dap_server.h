#pragma once

namespace bfc {

// Run a Debug Adapter Protocol server on stdin/stdout.
// Returns a process exit code.
int run_dap_server();

} // namespace bfc