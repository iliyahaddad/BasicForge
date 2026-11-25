#pragma once

namespace bfc {

// Run a Language Server Protocol server on stdin/stdout.
// Returns a process exit code.
int run_lsp_server();

} // namespace bfc