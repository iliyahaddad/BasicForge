// Tree-sitter grammar for BasicForge language
const { Grammar } = require('tree-sitter');

module.exports = Grammar({
  name: 'basicforge',

  extras: [
    /[\s\t\r\f]+/, // whitespace
    /#.*/,         // comments
  ],

  word: $ => $.identifier,

  rules: {
    program: $ => repeat($.statement),

    statement: $ => choice(
      $.dim_statement,
      $.print_statement,
      $.return_statement,
      $.if_statement,
      $.for_statement,
      $.while_statement,
      $.do_loop_statement,
      $.select_statement,
      $.try_statement,
      $.throw_statement,
      $.sub_declaration,
      $.function_declaration,
      $.class_declaration,
      $.module_declaration,
      $.enum_declaration,
      $.interface_declaration,
      $.property_declaration,
      $.assignment_statement,
      $.expression_statement,
    ),

    dim_statement: $ => seq(
      'Dim',
      choice($.identifier, $.array_declaration),
      optional(seq('As', $.type)),
      optional(seq('=', $.expression)),
    ),

    array_declaration: $ => seq(
      $.identifier,
      '(',
      optional($.expression),
      ')',
    ),

    print_statement: $ => seq('Print', $.expression_list),

    return_statement: $ => seq('Return', optional($.expression)),

    if_statement: $ => seq(
      'If',
      $.expression,
      'Then',
      repeat($.statement),
      optional(seq('Else', repeat($.statement))),
      'End',
      'If',
    ),

    for_statement: $ => seq(
      'For',
      $.identifier,
      '=',
      $.expression,
      'To',
      $.expression,
      optional(seq('Step', $.expression)),
      repeat($.statement),
      'Next',
    ),

    while_statement: $ => seq(
      'While',
      $.expression,
      repeat($.statement),
      'End',
      'While',
    ),

    do_loop_statement: $ => seq(
      'Do',
      repeat($.statement),
      'Loop',
      optional(choice(
        seq('While', $.expression),
        seq('Until', $.expression),
      )),
    ),

    select_statement: $ => seq(
      'Select',
      'Case',
      $.expression,
      repeat($.case_clause),
      optional(seq('Case', 'Else', repeat($.statement))),
      'End',
      'Select',
    ),

    case_clause: $ => seq(
      'Case',
      $.expression_list,
      repeat($.statement),
    ),

    try_statement: $ => seq(
      'Try',
      repeat($.statement),
      repeat($.catch_clause),
      optional(seq('Finally', repeat($.statement))),
      'End',
      'Try',
    ),

    catch_clause: $ => seq(
      'Catch',
      optional($.identifier),
      optional(seq('As', $.type)),
      repeat($.statement),
    ),

    throw_statement: $ => seq('Throw', $.expression),

    sub_declaration: $ => seq(
      'Sub',
      $.identifier,
      optional('(' + optional($.parameter_list) + ')'),
      optional(seq('As', $.type)),
      repeat($.statement),
      'End',
      'Sub',
    ),

    function_declaration: $ => seq(
      'Function',
      $.identifier,
      optional('(' + optional($.parameter_list) + ')'),
      'As',
      $.type,
      repeat($.statement),
      'End',
      'Function',
    ),

    class_declaration: $ => seq(
      'Class',
      $.identifier,
      optional(seq('Implements', $.identifier_list)),
      repeat($.class_member),
      'End',
      'Class',
    ),

    class_member: $ => choice(
      $.sub_declaration,
      $.function_declaration,
      $.property_declaration,
    ),

    property_declaration: $ => seq(
      'Property',
      $.identifier,
      'As',
      $.type,
    ),

    module_declaration: $ => seq(
      'Module',
      $.identifier,
      repeat($.statement),
      'End',
      'Module',
    ),

    enum_declaration: $ => seq(
      'Enum',
      $.identifier,
      repeat($.enum_member),
      'End',
      'Enum',
    ),

    interface_declaration: $ => seq(
      'Interface',
      $.identifier,
      repeat($.interface_member),
      'End',
      'Interface',
    ),

    interface_member: $ => seq(
      'Sub',
      $.identifier,
      optional('(' + optional($.parameter_list) + ')'),
    ),

    enum_member: $ => seq(
      $.identifier,
      optional(seq('=', $.number)),
    ),

    parameter_list: $ => sep1($.parameter, ','),

    parameter: $ => seq(
      optional(choice('ByVal', 'ByRef')),
      $.identifier,
      'As',
      $.type,
    ),

    parameter_list: $ => sep1($.parameter, ','),

    expression_list: $ => sep1($.expression, ','),

    expression_list: $ => sep1($.expression, ','),

    assignment_statement: $ => choice(
      $.simple_assignment,
      $.array_assignment,
      $.field_assignment,
    ),

    simple_assignment: $ => seq(
      $.identifier,
      '=',
      $.expression,
    ),

    array_assignment: $ => seq(
      $.identifier,
      '(',
      $.expression,
      ')',
      '=',
      $.expression,
    ),

    field_assignment: $ => seq(
      $.expression,
      '.',
      $.identifier,
      '=',
      $.expression,
    ),

    expression_statement: $ => $.expression,

    expression: $ => choice(
      $.integer_literal,
      $.double_literal,
      $.string_literal,
      $.boolean_literal,
      $.identifier,
      $.call_expression,
      $.array_access,
      $.field_access,
      $.unary_expression,
      $.binary_expression,
      $.ternary_expression,
      $.parenthesized_expression,
    ),

    integer_literal: $ => /\d+/,
    double_literal: $ => /\d+\.\d+/,
    string_literal: $ => /"(?:[^"\\]|\\.)*"/,
    boolean_literal: $ => choice('True', 'False'),

    identifier: $ => /[a-zA-Z_][a-zA-Z0-9_]*/,

    call_expression: $ => seq(
      $.identifier,
      '(',
      optional($.expression_list),
      ')',
    ),

    array_access: $ => seq(
      $.identifier,
      '(',
      $.expression,
      ')',
    ),

    field_access: $ => seq(
      $.expression,
      '.',
      $.identifier,
    ),

    unary_expression: $ => choice(
      seq('-', $.expression),
      seq('+', $.expression),
      seq('Not', $.expression),
    ),

    binary_expression: $ => choice(
      $.addition,
      $.subtraction,
      $.multiplication,
      $.division,
      $.integer_division,
      $.modulo,
      $.exponentiation,
      $.concatenation,
      $.equality,
      $.inequality,
      $.less_than,
      $.greater_than,
      $.less_than_equal,
      $.greater_than_equal,
      $.and_op,
      $.or_op,
      $.xor_op,
    ),

    addition: $ => seq($.expression, '+', $.expression),
    subtraction: $ => seq($.expression, '-', $.expression),
    multiplication: $ => seq($.expression, '*', $.expression),
    division: $ => seq($.expression, '/', $.expression),
    integer_division: $ => seq($.expression, '\\', $.expression),
    modulo: $ => seq($.expression, 'Mod', $.expression),
    exponentiation: $ => seq($.expression, '^', $.expression),
    concatenation: $ => seq($.expression, '&', $.expression),

    equality: $ => seq($.expression, '==', $.expression),
    inequality: $ => seq($.expression, '<>', $.expression),
    less_than: $ => seq($.expression, '<', $.expression),
    greater_than: $ => seq($.expression, '>', $.expression),
    less_than_equal: $ => seq($.expression, '<=', $.expression),
    greater_than_equal: $ => seq($.expression, '>=', $.expression),

    and_op: $ => seq($.expression, 'And', $.expression),
    or_op: $ => seq($.expression, 'Or', $.expression),
    xor_op: $ => seq($.expression, 'Xor', $.expression),

    ternary_expression: $ => seq(
      $.expression,
      '?',
      $.expression,
      ':',
      $.expression,
    ),

    parenthesized_expression: $ => seq(
      '(',
      $.expression,
      ')',
    ),

    identifier_list: $ => sep1($.identifier, ','),
    expression_list: $ => sep1($.expression, ','),
    type: $ => choice(
      'Integer',
      'Long',
      'Single',
      'Double',
      'Boolean',
      'String',
      $.identifier,
    ),
    number: $ => /\d+(\.\d+)?/,
  },
});