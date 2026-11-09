# Editor support

`xe lsp` is a language server (Language Server Protocol over standard input/output). It provides

- **diagnostics** while you type (the compiler's own errors, with line and column),
- **formatting** (the same as `xe fmt`),
- **go to definition** (functions, types, globals, enums, modules in the file and in the libraries),
- **completion** (keywords and the names declared in the file).

Any editor with an LSP client can use it by running the command `xe lsp` for `.xe` and `.xel` files. The server finds `std/` and `libraries/` through `XE_HOME` or next to the `xe` binary.

## Neovim

```lua
vim.filetype.add({ extension = { xe = "xe", xel = "xe" } })
vim.api.nvim_create_autocmd("FileType", {
  pattern = "xe",
  callback = function()
    vim.lsp.start({ name = "xe", cmd = { "xe", "lsp" }, root_dir = vim.fn.getcwd() })
  end,
})
```

## Helix (`languages.toml`)

```toml
[language-server.xe]
command = "xe"
args = ["lsp"]

[[language]]
name = "xe"
scope = "source.xe"
file-types = ["xe", "xel"]
language-servers = ["xe"]
indent = { tab-width = 4, unit = "    " }
```

## Zed

Add to `settings.json`:

```json
{
  "lsp": { "xe": { "binary": { "path": "xe", "arguments": ["lsp"] } } },
  "languages": { "Xe": { "language_servers": ["xe"], "format_on_save": "on" } }
}
```

(`zed-xe/` is the Zed extension with syntax highlighting; its tree-sitter grammar predates `global`, `enum`, generics, `break` and `continue`.)

## VS Code

`vscode-xe/` gives syntax highlighting and snippets. For the language server use any generic LSP client extension (for example "Generic LSP Client") with the command `xe` and the argument `lsp` for the language id `xe`.

## Formatting from the command line

```
xe fmt src/*.xe              # re-indent files in place
xe fmt --check src/*.xe      # list files that need it; exit status 1 if any
xe fmt --stdout file.xe      # print the result
```

The formatter changes only leading whitespace and trailing blanks: four spaces per level (`begin`/`end` and `{ }`), `case` arms one level inside `switch`, one extra level for continuation lines (after `;`), strings, comments and `asm` blocks untouched.

## Debugging

`xe --build prog.xe -g -o prog` adds source line information. Then `gdb prog` works at source level: `break prog.xe:12`, `run`, `next`, `step`, `bt`, `list`. Function names in gdb are `xe` + the Xe name (`xework` for `work`, `xegeo.sq` for `geo.sq`). Debug builds are not optimized, and variables are not described (use `print *(long*)($rbp-8)` or `x/8gx $rsp`).
