module.exports = grammar({
  name: 'basic',

  extras: $ => [$.comment, /\s/],

  word: $ => $.identifier,

  rules: {
    program: $ => repeat($._statement),

    _statement: $ => choice(
      $.module_statement,
      $.class_statement,
      $.enum_statement,
      $.interface_statement,
      $.function_statement,
      $.sub_statement,
      $.property_statement,
      $.dim_statement,
      $.assignment_statement,
      $.if_statement,
      $.for_statement,
      $.foreach_statement,
      $.while_statement,
      $.do_loop_statement,
      $.select_statement,
      $.try_catch_statement,
      $.return_statement,
      $.call_statement,
      $.raise_event_statement,
      $.using_statement,
      $.sync_lock_statement,
      $.exit_statement,
      $.continue_statement,
      $.stop_statement,
      $.end_statement,
      $.expression_statement
    ),

    // Module declaration
    module_statement: $ => seq(
      'Module',
      $.identifier,
      repeat($._statement),
      'End', 'Module'
    ),

    // Class declaration
    class_statement: $ => seq(
      choice('Public', 'Private', 'Friend', 'Protected', 'Protected Friend'),
      optional(choice('MustInherit', 'NotInheritable')),
      'Class',
      $.identifier,
      optional($.inherits_clause),
      optional($.implements_clause),
      repeat($._statement),
      'End', 'Class'
    ),

    inherits_clause: $ => seq('Inherits', $.identifier),

    implements_clause: $ => seq('Implements', commaSep($.identifier)),

    // Enum declaration
    enum_statement: $ => seq(
      choice('Public', 'Private', 'Friend'),
      'Enum',
      $.identifier,
      repeat($.enum_member),
      'End', 'Enum'
    ),

    enum_member: $ => seq(
      $.identifier,
      optional(seq('=', $.expression))
    ),

    // Interface declaration
    interface_statement: $ => seq(
      choice('Public', 'Private', 'Friend'),
      'Interface',
      $.identifier,
      repeat($._interface_member),
      'End', 'Interface'
    ),

    _interface_member: $ => choice(
      $.property_statement,
      $.sub_statement,
      $.function_statement,
      $.event_statement
    ),

    // Function declaration
    function_statement: $ => seq(
      choice('Public', 'Private', 'Friend', 'Protected', 'Protected Friend'),
      optional(choice('Overloads', 'Overridable', 'Overrides', 'NotOverridable', 'MustOverride', 'Shared', 'Static')),
      'Function',
      $.identifier,
      $.parameter_list,
      optional(seq('As', $.type_name)),
      repeat($._statement),
      'End', 'Function'
    ),

    // Sub declaration
    sub_statement: $ => seq(
      choice('Public', 'Private', 'Friend', 'Protected', 'Protected Friend'),
      optional(choice('Overloads', 'Overridable', 'Overrides', 'NotOverridable', 'MustOverride', 'Shared', 'Static')),
      'Sub',
      $.identifier,
      $.parameter_list,
      repeat($._statement),
      'End', 'Sub'
    ),

    // Property declaration
    property_statement: $ => seq(
      choice('Public', 'Private', 'Friend', 'Protected', 'Protected Friend'),
      optional(choice('ReadOnly', 'WriteOnly', 'Overloads', 'Overridable', 'Overrides', 'NotOverridable', 'MustOverride', 'Shared', 'Static', 'Default')),
      'Property',
      $.identifier,
      $.parameter_list,
      seq('As', $.type_name),
      repeat($._statement),
      'End', 'Property'
    ),

    // Event declaration
    event_statement: $ => seq(
      choice('Public', 'Private', 'Friend'),
      optional('Shared'),
      'Event',
      $.identifier,
      optional($.parameter_list),
      optional(seq('As', $.type_name))
    ),

    // Parameter list
    parameter_list: $ => seq(
      '(',
      optional(commaSep($.parameter)),
      ')'
    ),

    parameter: $ => seq(
      optional(choice('ByVal', 'ByRef', 'ParamArray', 'Optional')),
      optional('ReadOnly'),
      $.identifier,
      optional(seq('As', $.type_name)),
      optional(seq('=', $.expression))
    ),

    // Type names
    type_name: $ => choice(
      'Boolean', 'Byte', 'SByte', 'Short', 'UShort', 'Integer', 'UInteger',
      'Long', 'ULong', 'Single', 'Double', 'Decimal', 'String', 'Char',
      'Date', 'Object', 'Void',
      $.identifier,
      seq($.identifier, '(', commaSep($.type_name), ')')
    ),

    // Dim statement
    dim_statement: $ => seq(
      choice('Dim', 'Private', 'Public', 'Friend', 'Protected', 'Protected Friend', 'Static', 'ReadOnly', 'Const'),
      commaSep($.variable_declarator)
    ),

    variable_declarator: $ => seq(
      $.identifier,
      optional($.array_rank_specifier),
      optional(seq('As', $.type_name)),
      optional(seq('=', $.expression))
    ),

    array_rank_specifier: $ => seq(
      '(',
      repeat(seq(',', optional($.expression))),
      ')'
    ),

    // Assignment statement
    assignment_statement: $ => seq(
      $.variable_reference,
      choice('=', '+=', '-=', '*=', '/=', '\\=', '^=', '&='),
      $.expression
    ),

    variable_reference: $ => choice(
      $.identifier,
      $.member_access,
      $.array_access
    ),

    member_access: $ => seq(
      $.variable_reference,
      '.',
      $.identifier
    ),

    array_access: $ => seq(
      $.variable_reference,
      '(',
      commaSep($.expression),
      ')'
    ),

    // If statement
    if_statement: $ => seq(
      'If', $.expression, 'Then',
      repeat($._statement),
      repeat($.else_if_clause),
      optional($.else_clause),
      'End', 'If'
    ),

    else_if_clause: $ => seq(
      'ElseIf', $.expression, 'Then',
      repeat($._statement)
    ),

    else_clause: $ => seq(
      'Else',
      repeat($._statement)
    ),

    // For statement
    for_statement: $ => seq(
      'For', $.identifier, '=', $.expression, 'To', $.expression,
      optional(seq('Step', $.expression)),
      repeat($._statement),
      'Next', optional($.identifier)
    ),

    // For Each statement
    foreach_statement: $ => seq(
      'For', 'Each', $.identifier, optional(seq('As', $.type_name)), 'In', $.expression,
      repeat($._statement),
      'Next', optional($.identifier)
    ),

    // While statement
    while_statement: $ => seq(
      'While', $.expression,
      repeat($._statement),
      'End', 'While'
    ),

    // Do Loop statement
    do_loop_statement: $ => choice(
      seq('Do', optional(choice(seq('While', $.expression), seq('Until', $.expression))), repeat($._statement), 'Loop'),
      seq('Do', repeat($._statement), 'Loop', choice(seq('While', $.expression), seq('Until', $.expression)))
    ),

    // Select statement
    select_statement: $ => seq(
      'Select', 'Case', $.expression,
      repeat($.case_clause),
      optional($.case_else_clause),
      'End', 'Select'
    ),

    case_clause: $ => seq(
      'Case', commaSep($.case_expression),
      repeat($._statement)
    ),

    case_else_clause: $ => seq(
      'Case', 'Else',
      repeat($._statement)
    ),

    case_expression: $ => choice(
      $.expression,
      seq($.expression, 'To', $.expression),
      seq('Is', $.expression)
    ),

    // Try Catch statement
    try_catch_statement: $ => seq(
      'Try',
      repeat($._statement),
      repeat($.catch_clause),
      optional($.finally_clause),
      'End', 'Try'
    ),

    catch_clause: $ => seq(
      'Catch', optional(seq($.identifier, optional(seq('As', $.type_name))), optional(seq('When', $.expression))),
      repeat($._statement)
    ),

    finally_clause: $ => seq(
      'Finally',
      repeat($._statement)
    ),

    // Return statement
    return_statement: $ => seq('Return', optional($.expression)),

    // Call statement
    call_statement: $ => seq(optional('Call'), $.call_expression),

    // RaiseEvent statement
    raise_event_statement: $ => seq('RaiseEvent', $.call_expression),

    // Using statement
    using_statement: $ => seq(
      'Using', $.expression,
      repeat($._statement),
      'End', 'Using'
    ),

    // SyncLock statement
    sync_lock_statement: $ => seq(
      'SyncLock', $.expression,
      repeat($._statement),
      'End', 'SyncLock'
    ),

    // Exit statement
    exit_statement: $ => seq('Exit', choice('Function', 'Sub', 'Property', 'For', 'While', 'Do', 'Select', 'Try')),

    // Continue statement
    continue_statement: $ => seq('Continue', choice('For', 'While', 'Do')),

    // Stop statement
    stop_statement: $ => 'Stop',

    // End statement
    end_statement: $ => 'End',

    // Expression statement
    expression_statement: $ => $.expression,

    // Expressions
    expression: $ => choice(
      $.binary_expression,
      $.unary_expression,
      $.call_expression,
      $.member_expression,
      $.array_expression,
      $.object_creation_expression,
      $.cast_expression,
      $.type_of_expression,
      $.is_expression,
      $.like_expression,
      $.parenthesized_expression,
      $.literal,
      $.identifier
    ),

    binary_expression: $ => {
      const operators = [
        ['Or', 'OrElse', 'Xor'],
        ['And', 'AndAlso'],
        ['=', '<>', '<', '>', '<=', '>='],
        ['+', '-'],
        ['*', '/', '\\', 'Mod'],
        ['^'],
        ['&']
      ];

      return choice(...operators.map((ops, i) => {
        const left = i === 0 ? $.expression : choice(...operators.slice(0, i).flatMap(o => o.map(op => seq($._binary_expr_prec(i - 1), op, $._binary_expr_prec(i - 1)))));
        return seq(left, choice(...ops), $._binary_expr_prec(i));
      }));
    },

    _binary_expr_prec: $ => choice(
      $.unary_expression,
      $.call_expression,
      $.member_expression,
      $.array_expression,
      $.object_creation_expression,
      $.cast_expression,
      $.type_of_expression,
      $.is_expression,
      $.like_expression,
      $.parenthesized_expression,
      $.literal,
      $.identifier
    ),

    unary_expression: $ => seq(
      choice('+', '-', 'Not'),
      $.expression
    ),

    call_expression: $ => seq(
      $.identifier,
      '(',
      optional(commaSep($.expression)),
      ')'
    ),

    member_expression: $ => seq(
      $.expression,
      '.',
      $.identifier
    ),

    array_expression: $ => seq(
      $.expression,
      '(',
      commaSep($.expression),
      ')'
    ),

    object_creation_expression: $ => seq(
      'New',
      $.identifier,
      optional($.parameter_list)
    ),

    cast_expression: $ => seq(
      choice('CBool', 'CByte', 'CSByte', 'CShort', 'CUShort', 'CInt', 'CUInt', 'CLng', 'CULng', 'CSng', 'CDbl', 'CDec', 'CStr', 'CChar', 'CDate', 'CObj'),
      '(',
      $.expression,
      ')'
    ),

    type_of_expression: $ => seq('TypeOf', $.expression, 'Is', $.type_name),

    is_expression: $ => seq($.expression, 'Is', $.expression),

    like_expression: $ => seq($.expression, 'Like', $.expression),

    parenthesized_expression: $ => seq('(', $.expression, ')'),

    literal: $ => choice(
      $.string_literal,
      $.char_literal,
      $.number_literal,
      $.boolean_literal,
      $.date_literal,
      $.nothing_literal
    ),

    string_literal: $ => seq('"', repeat(choice(/[^"\\]/, '\\.', '""')), '"'),

    char_literal: $ => seq('"', /[^"\\]/, '"', 'c'),

    number_literal: $ => choice(
      /[0-9]+/,
      /[0-9]+\.[0-9]*/,
      /[0-9]*\.[0-9]+/,
      /&H[0-9A-Fa-f]+/,
      /&O[0-7]+/
    ),

    boolean_literal: $ => choice('True', 'False'),

    date_literal: $ => seq('#', /[^#]+/, '#'),

    nothing_literal: $ => 'Nothing',

    identifier: $ => /[A-Za-z_][A-Za-z0-9_]*/,

    comment: $ => choice(
      seq("'", /.*/),
      seq('Rem', /.*/)
    ),
  }
});

function commaSep(rule) {
  return seq(rule, repeat(seq(',', rule)));
}