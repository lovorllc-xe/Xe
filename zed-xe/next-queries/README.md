These queries cover the newer syntax (`global`, `enum`, generics, `else if`, `break`, `continue`). They only compile
against a parser regenerated from the current `tree-sitter-xe/grammar.js`. After running `tree-sitter generate` in
`tree-sitter-xe/`, copy them over the ones in `languages/xe/` (and `tree-sitter-highlights.scm` over
`tree-sitter-xe/queries/highlights.scm`), commit, and update `rev` in `extension.toml`.
Until then Zed uses the queries in `languages/xe/`, which only mention nodes the committed `parser.c` knows.
