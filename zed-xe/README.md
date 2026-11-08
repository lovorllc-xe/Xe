# Xe for Zed

Syntax highlighting, bracket matching, auto-indent and outline support for
the Xe programming language (`.xe`, `.xel`) in Zed.

```
zed-xe/
├── extension.toml          extension manifest
├── languages/xe/           Zed language config + tree-sitter queries
├── tree-sitter-xe/         tree-sitter grammar (grammar.js + generated src/)
└── xe-vibrant-syntax.json  Xe Vibrant syntax colours for settings.json
```

## Install (dev extension)

1. Commit `zed-xe/` and put that commit's SHA in `rev` in `extension.toml`
   (Zed fetches the grammar with git, so it must be committed).
2. In Zed: command palette → `zed: install dev extension` → pick `zed-xe/`.

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
cd zed-xe/tree-sitter-xe
npx tree-sitter-cli generate --abi 14
npx tree-sitter-cli parse ../../libraries/*.xel   # sanity check
```

Then commit, update `rev`, and rebuild the dev extension in Zed.
