# Xe for Zed

Syntax highlighting, bracket matching, auto-indent, outline, word handling and the Xe language server for
the Xe programming language (`.xe`, `.xel`) in Zed.

- **Words:** letters, digits and `_` form a word (double-click, word motions); completion queries include `.` so
  `module.function` completes.
- **Auto-indent:** the line after `begin`, `{`, `case ...` or `default` is indented; a line that starts with `end`, `}`, `case`
  or `default` is outdented. This works from patterns in `languages/xe/config.toml`, so it does not depend on the parser.
- **Brackets:** `( ) [ ] { }` pair and auto-close, `begin`/`end` match (functions, modules, records, enums, `switch`).
- **Outline and text objects:** functions, modules, records, aliases, enums (with members) and globals.
- **Language server:** `src/lib.rs` starts `xe lsp`: diagnostics as you type, format on save (`xe fmt` rules), go to
  definition and completion. `xe` must be on `PATH`. The extension is built by Zed when you install it as a dev extension
  and needs Rust installed through `rustup` (Zed adds the wasm target itself). It has not been built or run by the author's
  tooling: if Zed reports a build error, the message will say what to fix.

Suggested `settings.json`:

```json
{
  "languages": {
    "Xe": { "format_on_save": "on", "tab_size": 4, "language_servers": ["xe-lsp"] }
  }
}
```

```
Extensions/zed-xe/
├── extension.toml          extension manifest
├── languages/xe/           Zed language config + tree-sitter queries
├── tree-sitter-xe/         tree-sitter grammar (grammar.js + generated src/)
└── xe-vibrant-syntax.json  Xe Vibrant syntax colours for settings.json
```

## Install (dev extension)

1. Commit `Extensions/zed-xe/` and put that commit's SHA in `rev` in `extension.toml`
   (Zed fetches the grammar with git, so it must be committed).
2. In Zed: command palette → `zed: install dev extension` → pick `Extensions/zed-xe/`.

## Xe Vibrant colours (syntax only)

All highlight captures end in `.xe` (`keyword.xe`, `function.xe`, …). With
no extra config, Xe uses your current theme's colours. To get the same
colours as the VS Code "Xe Vibrant" theme, merge `xe-vibrant-syntax.json`
into `~/.config/zed/settings.json`. These keys only match Xe captures,
so other languages and the rest of your theme are not affected.

| Element                      | Capture                           | Colour    |
|------------------------------|-----------------------------------|-----------|
| comments (italic)            | `comment.xe`                      | `#858A9E` |
| keywords, `fn module type`   | `keyword.xe`                      | `#C792EA` |
| `int flt string void record`, `[N]`, type names | `type.xe`      | `#66D9EF` |
| function declarations/calls  | `function.xe`                     | `#82AAFF` |
| module names                 | `namespace.xe`                    | `#F78C6C` |
| strings                      | `string.xe`                       | `#A6E22E` |
| escapes                      | `string.escape.xe`                | `#FD971F` |
| numbers                      | `number.xe`                       | `#FD971F` |
| operators                    | `operator.xe`                     | `#F92672` |
| `asm` + assembly body        | `keyword.embedded.xe`, `embedded.xe` | `#E6DB74` |
| `asm { }` braces             | `punctuation.bracket.embedded.xe` | `#F0F0FA` |

## Changing the grammar

```sh
cd Extensions/zed-xe/tree-sitter-xe
npx tree-sitter-cli generate --abi 14
npx tree-sitter-cli parse ../../../libraries/*.xel   # sanity check
```

Then commit, update `rev`, and rebuild the dev extension in Zed.
