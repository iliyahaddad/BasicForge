# BasicForge DAP (Debug Adapter Protocol)

`bfc --dap` starts a minimal Debug Adapter Protocol server on stdio. It can be
used by VS Code and other DAP clients.

## VS Code launch.json

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "type": "basicforge",
      "request": "launch",
      "name": "Debug BasicForge",
      "program": "${workspaceFolder}/myapp.exe",
      "bfc": "C:/.../BasicForge/bfc.exe"
    }
  ]
}
```

## Implemented requests

- `initialize`
- `launch` / `attach` (stores the executable path)
- `setBreakpoints` (stored; line resolution not yet implemented)
- `configurationDone`
- `continue`
- `next` / `stepIn`
- `stackTrace`
- `disconnect` / `terminate`

## Limitations

- Breakpoints are accepted but not yet resolved to addresses (no PDB line
  mapping in the DAP server).
- `scopes`/`variables`/`evaluate` are not implemented.
- The server is a minimal starting point; the CLI debugger (`bfc debug`) is the
  full-featured debugger.

## Usage

```powershell
bfc.exe --dap
```

The server speaks JSON-RPC over stdio with `Content-Length` framing.