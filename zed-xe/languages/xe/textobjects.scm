(function_declaration
  body: (function_body) @function.inside) @function.around

(record_declaration) @class.around

(module_declaration
  body: (module_body) @class.inside) @class.around

(comment) @comment.around
