/**
 * Tree-sitter grammar for the Xe programming language.
 *
 * Mirrors the lexer/parser in src/lexer.cpp and src/parser.cpp:
 *  - newlines terminate statements (they are real tokens, not whitespace)
 *  - `;` is a line-continuation marker that swallows the following newline
 *  - function, module, record and switch bodies use `begin ... end`
 *  - control-flow bodies use `{ ... }`
 *  - `asm { ... }` holds raw target assembly
 */

const PREC = {
  or: 1,
  and: 2,
  bit_or: 3,
  bit_xor: 4,
  bit_and: 5,
  equality: 6,
  compare: 7,
  shift: 8,
  additive: 9,
  multiplicative: 10,
  unary: 11,
  postfix: 12,
};

const commaSep1 = (rule) => seq(rule, repeat(seq(',', rule)));
const commaSep = (rule) => optional(commaSep1(rule));

module.exports = grammar({
  name: 'xe',

  word: ($) => $.identifier,

  extras: ($) => [/[ \t\r\f]/, $.comment, $.line_continuation],

  conflicts: ($) => [
    [$.if_statement],
    [$._statements],
  ],

  rules: {
    source_file: ($) => repeat(choice($._nl, $._top_level_item)),

    _top_level_item: ($) =>
      choice(
        $.uses_declaration,
        $.module_declaration,
        $.function_declaration,
        $.record_declaration,
        $.type_alias,
      ),

    // A newline token. Blank lines collapse into one token.
    _nl: (_) => /\n/,

    comment: (_) => token(seq('//', /[^\n]*/)),

    line_continuation: (_) => token(seq(';', /[ \t\r]*/, optional(seq('//', /[^\n]*/)), '\n')),

    // ---------------------------------------------------------------
    // Declarations
    // ---------------------------------------------------------------

    uses_declaration: ($) => seq('uses', commaSep1(field('module', $.module_path))),

    module_path: ($) => seq($.identifier, repeat(seq('.', $.identifier))),

    module_declaration: ($) =>
      seq(
        'module',
        field('name', $.module_path),
        repeat($._nl),
        'begin',
        field('body', alias(repeat(choice($._nl, $._module_item)), $.module_body)),
        'end',
      ),

    _module_item: ($) => choice($.function_declaration, $.record_declaration, $.type_alias),

    function_declaration: ($) =>
      seq(
        'fn',
        field('name', $._declaration_name),
        field('parameters', $.parameter_list),
        optional(seq(':', field('return_type', $._type))),
        repeat($._nl),
        field('body', $.function_body),
      ),

    _declaration_name: ($) =>
      choice($.identifier, seq(repeat1(seq($.identifier, '.')), $.identifier)),

    parameter_list: ($) => seq('(', commaSep($.parameter), ')'),

    parameter: ($) => seq(field('name', $.identifier), ':', field('type', $._type)),

    function_body: ($) => seq('begin', optional($._statements), 'end'),

    type_alias: ($) =>
      seq('type', field('name', $._type_declaration_name), ':', field('type', $._type)),

    record_declaration: ($) =>
      seq(
        'type',
        field('name', $._type_declaration_name),
        ':',
        'record',
        repeat($._nl),
        'begin',
        repeat(choice($._nl, $.field_declaration)),
        'end',
      ),

    _type_declaration_name: ($) =>
      choice(
        alias($.identifier, $.type_identifier),
        seq(repeat1(seq($.identifier, '.')), alias($.identifier, $.type_identifier)),
      ),

    field_declaration: ($) =>
      seq(field('name', alias($.identifier, $.field_identifier)), ':', field('type', $._type)),

    // ---------------------------------------------------------------
    // Types
    // ---------------------------------------------------------------

    _type: ($) => choice($.primitive_type, $.pointer_type, $.array_type, $.type_name),

    primitive_type: (_) => choice('int', 'flt', 'string', 'void'),

    pointer_type: ($) => seq('*', $._type),

    array_type: ($) => seq('[', field('length', $.array_length), ']', field('element', $._type)),

    array_length: (_) => /[0-9]+/,

    type_name: ($) =>
      choice(
        alias($.identifier, $.type_identifier),
        seq(repeat1(seq($.identifier, '.')), alias($.identifier, $.type_identifier)),
      ),

    // ---------------------------------------------------------------
    // Statements
    // ---------------------------------------------------------------

    // One or more statements separated by newlines, with optional
    // leading/trailing blank lines.
    _statements: ($) =>
      choice(
        repeat1($._nl),
        seq(
          repeat($._nl),
          $._statement,
          repeat(seq(repeat1($._nl), $._statement)),
          repeat($._nl),
        ),
      ),

    _statement: ($) =>
      choice(
        $.var_declaration,
        $.return_statement,
        $.defer_statement,
        $.if_statement,
        $.while_statement,
        $.switch_statement,
        $.gc_statement,
        $.asm_statement,
        $.block,
        $.assignment_statement,
        $.expression_statement,
      ),

    block: ($) => seq('{', optional($._statements), '}'),

    var_declaration: ($) =>
      seq(
        choice('var', 'const'),
        field('name', $.identifier),
        ':',
        field('type', $._type),
        optional(seq(':=', field('value', $._expression))),
      ),

    return_statement: ($) => prec.right(seq('return', optional($._expression))),

    defer_statement: ($) => seq('defer', $._expression),

    if_statement: ($) =>
      seq(
        'if',
        field('condition', $._expression),
        repeat($._nl),
        field('consequence', $.block),
        optional(seq(repeat($._nl), 'else', repeat($._nl), field('alternative', $.block))),
      ),

    while_statement: ($) =>
      seq(
        choice('while', 'for'),
        field('condition', $._expression),
        repeat($._nl),
        field('body', $.block),
      ),

    switch_statement: ($) =>
      seq(
        'switch',
        field('value', $._expression),
        repeat($._nl),
        'begin',
        repeat($._nl),
        repeat(choice($.case_clause, $.default_clause)),
        'end',
      ),

    case_clause: ($) => seq('case', field('value', $._expression), optional($._statements)),

    default_clause: ($) => seq('default', optional($._statements)),

    gc_statement: ($) =>
      // Both `gc.begin { ... } gc.end` and `gc.begin ... gc.end` forms;
      // the braced form parses as a block statement inside the region.
      seq($.gc_begin, optional($._statements), $.gc_end),

    gc_begin: (_) => seq('gc', '.', 'begin'),

    gc_end: (_) => seq('gc', '.', 'end'),

    asm_statement: ($) =>
      seq('asm', repeat($._nl), '{', optional(alias($._asm_body, $.asm_content)), '}'),

    _asm_body: ($) => repeat1(choice($._asm_text, seq('{', optional($._asm_body), '}'))),

    _asm_text: (_) => token(prec(-1, /[^{}]+/)),

    assignment_statement: ($) =>
      seq(field('left', $._expression), ':=', field('right', $._expression)),

    expression_statement: ($) => $._expression,

    // ---------------------------------------------------------------
    // Expressions
    // ---------------------------------------------------------------

    _expression: ($) =>
      choice(
        $.identifier,
        $.number,
        $.string,
        $.parenthesized_expression,
        $.call_expression,
        $.member_expression,
        $.index_expression,
        $.slice_expression,
        $.unary_expression,
        $.binary_expression,
      ),

    parenthesized_expression: ($) => seq('(', $._expression, ')'),

    call_expression: ($) =>
      prec(
        PREC.postfix,
        seq(
          field('function', choice($.identifier, $.member_expression)),
          field('arguments', $.argument_list),
        ),
      ),

    argument_list: ($) => seq('(', commaSep($._expression), ')'),

    member_expression: ($) =>
      prec(
        PREC.postfix,
        seq(
          field('object', $._expression),
          '.',
          field('field', alias($.identifier, $.field_identifier)),
        ),
      ),

    index_expression: ($) =>
      prec(PREC.postfix, seq(field('object', $._expression), '[', field('index', $._expression), ']')),

    slice_expression: ($) =>
      prec(
        PREC.postfix,
        seq(
          field('object', $._expression),
          '[',
          optional(field('start', $._expression)),
          ':',
          optional(field('end', $._expression)),
          ']',
        ),
      ),

    unary_expression: ($) =>
      prec(
        PREC.unary,
        seq(
          field('operator', choice('-', '!', '~', '*', '&', '@', '#')),
          field('operand', $._expression),
        ),
      ),

    binary_expression: ($) => {
      const table = [
        [PREC.or, '||'],
        [PREC.and, '&&'],
        [PREC.bit_or, '|'],
        [PREC.bit_xor, '^'],
        [PREC.bit_and, '&'],
        [PREC.equality, choice('=', '==', '!=')],
        [PREC.compare, choice('<', '>', '<=', '>=')],
        [PREC.shift, choice('<<', '>>')],
        [PREC.additive, choice('+', '-')],
        [PREC.multiplicative, choice('*', '/', '%')],
      ];
      return choice(
        ...table.map(([p, op]) =>
          prec.left(
            p,
            seq(
              field('left', $._expression),
              field('operator', op),
              field('right', $._expression),
            ),
          ),
        ),
      );
    },

    identifier: (_) => /[A-Za-z_][A-Za-z0-9_]*/,

    number: (_) => /[0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)?/,

    string: ($) =>
      seq(
        '"',
        repeat(choice(alias($._string_content, $.string_content), $.escape_sequence)),
        token.immediate('"'),
      ),

    _string_content: (_) => token.immediate(prec(1, /[^"\\]+/)),

    escape_sequence: (_) => token.immediate(/\\(.|\n)/),
  },
});
