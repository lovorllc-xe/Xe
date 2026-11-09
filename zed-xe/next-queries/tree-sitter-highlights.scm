; Xe syntax highlighting for Zed.
;
; Every capture ends in `.xe`. Any Zed theme falls back to its normal colour
; (`keyword.xe` -> `keyword`), and the Xe Vibrant colours in
; zed-xe/README.md can be applied to Xe files only via `theme_overrides`.

; Comments ---------------------------------------------------------------
(comment) @comment.xe

; Keywords ---------------------------------------------------------------
[
  "fn"
  "module"
  "type"
  "begin"
  "end"
  "var"
  "const"
  "uses"
  "if"
  "else"
  "while"
  "for"
  "switch"
  "case"
  "default"
  "return"
  "defer"
  "gc"
  "global"
  "enum"
  "break"
  "continue"
] @keyword.xe

; Types ------------------------------------------------------------------
(primitive_type) @type.xe
"record" @type.xe
(type_identifier) @type.xe
(enum_identifier) @constant.xe
(array_type
  "[" @type.xe
  length: (array_length) @type.xe
  "]" @type.xe)

; Functions --------------------------------------------------------------
(function_declaration
  name: (identifier) @function.xe)

(call_expression
  function: (identifier) @function.xe)

(call_expression
  function: (member_expression
    field: (field_identifier) @function.xe))

; Modules ----------------------------------------------------------------
(module_declaration
  name: (module_path (identifier) @namespace.xe))

(uses_declaration
  module: (module_path (identifier) @namespace.xe))

; Literals ---------------------------------------------------------------
(string) @string.xe
(escape_sequence) @string.escape.xe
(number) @number.xe

; Operators --------------------------------------------------------------
[
  ":="
  "="
  "=="
  "!="
  "<"
  ">"
  "<="
  ">="
  "&&"
  "||"
  "<<"
  ">>"
  "+"
  "-"
  "*"
  "/"
  "%"
  "&"
  "|"
  "^"
  "~"
  "!"
  "@"
  "#"
] @operator.xe

; Inline assembly --------------------------------------------------------
(asm_statement "asm" @keyword.embedded.xe)
(asm_statement ["{" "}"] @punctuation.bracket.embedded.xe)
(asm_content) @embedded.xe
