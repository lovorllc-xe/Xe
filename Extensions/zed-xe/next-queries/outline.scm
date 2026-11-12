(module_declaration
  "module" @context
  name: (module_path) @name) @item

(function_declaration
  "fn" @context
  name: (_) @name) @item

(record_declaration
  "type" @context
  name: (_) @name) @item

(type_alias
  "type" @context
  name: (_) @name) @item

(enum_declaration
  "enum" @context
  name: (_) @name) @item

(enum_member
  name: (_) @name) @item

(global_declaration
  "global" @context
  name: (identifier) @name) @item
