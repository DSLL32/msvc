/* C grammar automaton encountered by parse_translation_unit (0x23192).
   Each row below is a logical terminal shift, reduction exception, or goto.
   Original yacc compression has been decoded: there are no image addresses,
   compressed action/check snapshots, or unrelated data here. State and rule
   numbers retain the grammar generator's identities for differential work. */
#include "parser.h"

ParserTransition parser_shifts[] = {
    /* State 0. */
    /* State 1. */
    { TOK_LEFT_BRACE,             9 },
    { TOK_SEMICOLON,              6 },
    { TOK_TYPE_NAME,             11 },
    { TOK_CLASS_HEAD,            15 },
    { TOK_ENUM_TAG,              16 },
    { PARSER_ERROR_TOKEN,         8 },
    /* State 2. */
    /* State 3. */
    /* State 4. */
    /* State 5. */
    { TOK_IDENTIFIER,            21 },
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_ASSIGN,                28 },
    { TOK_MULTIPLY,              23 },
    { TOK_LEFT_PAREN,            27 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    /* State 6. */
    /* State 7. */
    { TOK_SEMICOLON,             34 },
    /* State 8. */
    { TOK_LEFT_BRACE,            36 },
    { TOK_SEMICOLON,             35 },
    /* State 9. */
    /* State 10. */
    /* State 11. */
    /* State 12. */
    /* State 13. */
    /* State 14. */
    /* State 15. */
    { TOK_LEFT_BRACE,            37 },
    /* State 16. */
    { TOK_LEFT_BRACE,            38 },
    /* State 17. */
    { TOK_NEWLINE,               42 },
    { TOK_TYPE_NAME,             11 },
    { TOK_CLASS_HEAD,            15 },
    { TOK_ENUM_TAG,              16 },
    /* State 18. */
    { TOK_COMMA,                 44 },
    { TOK_SEMICOLON,             43 },
    /* State 19. */
    { TOK_ASSIGN,                45 },
    { TOK_LEFT_BRACKET,          46 },
    { TOK_LEFT_PAREN,            48 },
    /* State 20. */
    /* State 21. */
    /* State 22. */
    { TOK_IDENTIFIER,            49 },
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_MULTIPLY,              50 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    /* State 23. */
    { TOK_IDENTIFIER,            21 },
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_MULTIPLY,              23 },
    { TOK_LEFT_PAREN,            27 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    /* State 24. */
    /* State 25. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 26. */
    /* State 27. */
    { TOK_IDENTIFIER,            21 },
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_MULTIPLY,              23 },
    { TOK_LEFT_PAREN,            27 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    /* State 28. */
    /* State 29. */
    /* State 30. */
    /* State 31. */
    /* State 32. */
    /* State 33. */
    { TOK_LEFT_PAREN,            75 },
    /* State 34. */
    /* State 35. */
    /* State 36. */
    /* State 37. */
    { TOK_TYPE_NAME,             11 },
    { TOK_CLASS_HEAD,            15 },
    { TOK_ENUM_TAG,              16 },
    /* State 38. */
    { TOK_IDENTIFIER,            83 },
    { TOK_RIGHT_BRACE,           81 },
    /* State 39. */
    { TOK_LEFT_BRACE,            85 },
    /* State 40. */
    /* State 41. */
    { TOK_IDENTIFIER,            21 },
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_MULTIPLY,              23 },
    { TOK_LEFT_PAREN,            27 },
    { TOK_MODIFIER,              29 },
    { TOK_SEMICOLON,             87 },
    { TOK_UNALIGNED,             32 },
    /* State 42. */
    /* State 43. */
    /* State 44. */
    { TOK_IDENTIFIER,            21 },
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_ASSIGN,                28 },
    { TOK_MULTIPLY,              23 },
    { TOK_LEFT_PAREN,            27 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    /* State 45. */
    /* State 46. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 47. */
    /* State 48. */
    /* State 49. */
    /* State 50. */
    { TOK_IDENTIFIER,            21 },
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_MULTIPLY,              23 },
    { TOK_LEFT_PAREN,            27 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    /* State 51. */
    /* State 52. */
    { TOK_LEFT_BRACKET,          46 },
    { TOK_LEFT_PAREN,            48 },
    /* State 53. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 54. */
    { TOK_LEFT_BRACE,           113 },
    /* State 55. */
    { TOK_LEFT_BRACKET,         115 },
    { TOK_SEGMENT_OFFSET,       117 },
    { TOK_INCREMENT_OPERATOR,   114 },
    { TOK_LEFT_PAREN,           118 },
    { TOK_MEMBER_OPERATOR,      116 },
    /* State 56. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 57. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 58. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 59. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 60. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 61. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 62. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 63. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 64. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 65. */
    { TOK_LEFT_PAREN,           131 },
    /* State 66. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 67. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_RIGHT_PAREN,          133 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 68. */
    { TOK_LEFT_PAREN,           136 },
    /* State 69. */
    /* State 70. */
    { TOK_STRING,               137 },
    /* State 71. */
    /* State 72. */
    /* State 73. */
    { TOK_LEFT_PAREN,           138 },
    /* State 74. */
    { TOK_LEFT_BRACKET,          46 },
    { TOK_LEFT_PAREN,            48 },
    { TOK_RIGHT_PAREN,          139 },
    /* State 75. */
    { TOK_IDENTIFIER,           143 },
    { TOK_SEGNAME,               73 },
    { TOK_LEFT_PAREN,           145 },
    { TOK_TYPE_NAME,            144 },
    /* State 76. */
    { TOK_RIGHT_BRACE,          146 },
    /* State 77. */
    { TOK_RIGHT_BRACE,          147 },
    { TOK_TYPE_NAME,             11 },
    { TOK_CLASS_HEAD,            15 },
    { TOK_ENUM_TAG,              16 },
    /* State 78. */
    /* State 79. */
    { TOK_IDENTIFIER,            21 },
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_MULTIPLY,              23 },
    { TOK_LEFT_PAREN,            27 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    /* State 80. */
    { TOK_COMMA,                155 },
    /* State 81. */
    /* State 82. */
    /* State 83. */
    { TOK_ASSIGN,               156 },
    /* State 84. */
    /* State 85. */
    /* State 86. */
    { TOK_IDENTIFIER,           160 },
    { TOK_COMMA,                159 },
    { TOK_SEMICOLON,            158 },
    /* State 87. */
    /* State 88. */
    /* State 89. */
    { TOK_LEFT_BRACKET,          46 },
    { TOK_LEFT_PAREN,            48 },
    /* State 90. */
    /* State 91. */
    { TOK_ASSIGN,                45 },
    { TOK_LEFT_BRACKET,          46 },
    { TOK_LEFT_PAREN,            48 },
    /* State 92. */
    { TOK_RIGHT_BRACKET,        161 },
    /* State 93. */
    /* State 94. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 95. */
    { TOK_IDENTIFIER,           164 },
    { TOK_TYPE_NAME,             11 },
    { TOK_CLASS_HEAD,            15 },
    { TOK_ENUM_TAG,              16 },
    { PARSER_ERROR_TOKEN,       167 },
    /* State 96. */
    { TOK_LEFT_BRACKET,          46 },
    { TOK_LEFT_PAREN,            48 },
    /* State 97. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 98. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 99. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 100. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 101. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 102. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 103. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 104. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 105. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 106. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 107. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 108. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 109. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 110. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 111. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 112. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 113. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_BRACE,           188 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 114. */
    /* State 115. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 116. */
    /* State 117. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 118. */
    /* State 119. */
    { TOK_LEFT_BRACKET,         115 },
    { TOK_SEGMENT_OFFSET,       117 },
    { TOK_INCREMENT_OPERATOR,   114 },
    { TOK_LEFT_PAREN,           118 },
    { TOK_MEMBER_OPERATOR,      116 },
    /* State 120. */
    { TOK_LEFT_BRACKET,         115 },
    { TOK_SEGMENT_OFFSET,       117 },
    { TOK_INCREMENT_OPERATOR,   114 },
    { TOK_LEFT_PAREN,           118 },
    { TOK_MEMBER_OPERATOR,      116 },
    /* State 121. */
    { TOK_LEFT_BRACKET,         115 },
    { TOK_SEGMENT_OFFSET,       117 },
    { TOK_INCREMENT_OPERATOR,   114 },
    { TOK_LEFT_PAREN,           118 },
    { TOK_MEMBER_OPERATOR,      116 },
    /* State 122. */
    { TOK_LEFT_BRACKET,         115 },
    { TOK_SEGMENT_OFFSET,       117 },
    { TOK_INCREMENT_OPERATOR,   114 },
    { TOK_LEFT_PAREN,           118 },
    { TOK_MEMBER_OPERATOR,      116 },
    /* State 123. */
    { TOK_LEFT_BRACKET,         115 },
    { TOK_INCREMENT_OPERATOR,   114 },
    { TOK_LEFT_PAREN,           118 },
    { TOK_MEMBER_OPERATOR,      116 },
    /* State 124. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_COMMA,                 98 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_RIGHT_PAREN,          192 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 125. */
    { TOK_TYPE_NAME,             11 },
    { TOK_CLASS_HEAD,            15 },
    { TOK_ENUM_TAG,              16 },
    /* State 126. */
    { TOK_LEFT_BRACKET,         115 },
    { TOK_SEGMENT_OFFSET,       117 },
    { TOK_INCREMENT_OPERATOR,   114 },
    { TOK_LEFT_PAREN,           118 },
    { TOK_MEMBER_OPERATOR,      116 },
    /* State 127. */
    { TOK_LEFT_BRACKET,         115 },
    { TOK_SEGMENT_OFFSET,       117 },
    { TOK_INCREMENT_OPERATOR,   114 },
    { TOK_LEFT_PAREN,           118 },
    { TOK_MEMBER_OPERATOR,      116 },
    /* State 128. */
    { TOK_LEFT_BRACKET,         115 },
    { TOK_SEGMENT_OFFSET,       117 },
    { TOK_INCREMENT_OPERATOR,   114 },
    { TOK_LEFT_PAREN,           118 },
    { TOK_MEMBER_OPERATOR,      116 },
    /* State 129. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 130. */
    /* State 131. */
    /* State 132. */
    { TOK_LEFT_BRACKET,         115 },
    { TOK_SEGMENT_OFFSET,       117 },
    { TOK_INCREMENT_OPERATOR,   114 },
    { TOK_LEFT_PAREN,           118 },
    { TOK_MEMBER_OPERATOR,      116 },
    /* State 133. */
    /* State 134. */
    { TOK_COMMA,                196 },
    { TOK_RIGHT_PAREN,          195 },
    /* State 135. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 136. */
    /* State 137. */
    /* State 138. */
    { TOK_STRING,                72 },
    /* State 139. */
    /* State 140. */
    { TOK_RIGHT_PAREN,          198 },
    /* State 141. */
    /* State 142. */
    /* State 143. */
    /* State 144. */
    /* State 145. */
    { TOK_TYPE_NAME,            199 },
    /* State 146. */
    /* State 147. */
    /* State 148. */
    /* State 149. */
    { TOK_SEMICOLON,            200 },
    /* State 150. */
    { TOK_COMMA,                201 },
    { PARSER_ERROR_TOKEN,       202 },
    /* State 151. */
    /* State 152. */
    { TOK_LEFT_BRACKET,          46 },
    { TOK_LEFT_PAREN,            48 },
    /* State 153. */
    { TOK_COLON,                203 },
    /* State 154. */
    { TOK_RIGHT_BRACE,          204 },
    /* State 155. */
    { TOK_IDENTIFIER,            83 },
    /* State 156. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 157. */
    /* State 158. */
    /* State 159. */
    { TOK_IDENTIFIER,            21 },
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_MULTIPLY,              23 },
    { TOK_LEFT_PAREN,            27 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    /* State 160. */
    /* State 161. */
    /* State 162. */
    { TOK_COMMA,                210 },
    /* State 163. */
    /* State 164. */
    /* State 165. */
    /* State 166. */
    /* State 167. */
    /* State 168. */
    { TOK_IDENTIFIER,            21 },
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_MULTIPLY,             214 },
    { TOK_LEFT_PAREN,           217 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    /* State 169. */
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_PLUS,                 100 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 170. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 171. */
    /* State 172. */
    { TOK_MULTIPLY,             103 },
    { TOK_DIVISION_OPERATOR,     99 },
    /* State 173. */
    { TOK_MULTIPLY,             103 },
    { TOK_DIVISION_OPERATOR,     99 },
    /* State 174. */
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_PLUS,                 100 },
    { TOK_DIVISION_OPERATOR,     99 },
    /* State 175. */
    /* State 176. */
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_PLUS,                 100 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 177. */
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_PLUS,                 100 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 178. */
    { TOK_BIT_AND,              105 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_PLUS,                 100 },
    { TOK_BIT_XOR,              107 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 179. */
    { TOK_BIT_AND,              105 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_PLUS,                 100 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 180. */
    { TOK_BIT_AND,              105 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_PLUS,                 100 },
    { TOK_BIT_XOR,              107 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 181. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_PLUS,                 100 },
    { TOK_BIT_XOR,              107 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 182. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 183. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 184. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_COLON,                220 },
    { TOK_COMMA,                 98 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 185. */
    { TOK_COMMA,                222 },
    /* State 186. */
    /* State 187. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 188. */
    /* State 189. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_COMMA,                 98 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RIGHT_BRACKET,        224 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 190. */
    { TOK_IDENTIFIER,           225 },
    /* State 191. */
    { TOK_LEFT_BRACKET,         115 },
    { TOK_INCREMENT_OPERATOR,   114 },
    { TOK_LEFT_PAREN,           118 },
    { TOK_MEMBER_OPERATOR,      116 },
    /* State 192. */
    /* State 193. */
    { TOK_RIGHT_PAREN,          226 },
    /* State 194. */
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_MULTIPLY,             229 },
    { TOK_LEFT_PAREN,           230 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    /* State 195. */
    /* State 196. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 197. */
    { TOK_STRING,               137 },
    { TOK_RIGHT_PAREN,          232 },
    /* State 198. */
    /* State 199. */
    { TOK_RIGHT_PAREN,          233 },
    /* State 200. */
    /* State 201. */
    { TOK_IDENTIFIER,            21 },
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_MULTIPLY,              23 },
    { TOK_LEFT_PAREN,            27 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    { PARSER_ERROR_TOKEN,       235 },
    /* State 202. */
    /* State 203. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 204. */
    /* State 205. */
    /* State 206. */
    /* State 207. */
    { TOK_TYPE_NAME,             11 },
    { TOK_CLASS_HEAD,            15 },
    { TOK_ENUM_TAG,              16 },
    { PARSER_ERROR_TOKEN,       240 },
    /* State 208. */
    /* State 209. */
    { TOK_RIGHT_PAREN,          241 },
    /* State 210. */
    { TOK_IDENTIFIER,           164 },
    { TOK_ELLIPSIS,             243 },
    { TOK_TYPE_NAME,             11 },
    { TOK_CLASS_HEAD,            15 },
    { TOK_ENUM_TAG,              16 },
    { PARSER_ERROR_TOKEN,       167 },
    /* State 211. */
    { TOK_LEFT_BRACKET,          46 },
    { TOK_LEFT_PAREN,            48 },
    /* State 212. */
    { TOK_LEFT_BRACKET,         244 },
    /* State 213. */
    { TOK_IDENTIFIER,            49 },
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_MULTIPLY,             245 },
    { TOK_LEFT_PAREN,           247 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    /* State 214. */
    { TOK_IDENTIFIER,            21 },
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_MULTIPLY,             214 },
    { TOK_LEFT_PAREN,           217 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    /* State 215. */
    /* State 216. */
    /* State 217. */
    { TOK_IDENTIFIER,            21 },
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_MULTIPLY,             214 },
    { TOK_LEFT_PAREN,           217 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    /* State 218. */
    /* State 219. */
    { TOK_LEFT_PAREN,           247 },
    /* State 220. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 221. */
    { TOK_RIGHT_BRACE,          254 },
    /* State 222. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_BRACE,           188 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 223. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_BRACE,           188 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 224. */
    /* State 225. */
    /* State 226. */
    /* State 227. */
    { TOK_LEFT_BRACKET,         244 },
    /* State 228. */
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_MULTIPLY,             257 },
    { TOK_LEFT_PAREN,           247 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    /* State 229. */
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_MULTIPLY,             229 },
    { TOK_LEFT_PAREN,           230 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    /* State 230. */
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_MULTIPLY,             229 },
    { TOK_LEFT_PAREN,           230 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    /* State 231. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 232. */
    /* State 233. */
    { TOK_IDENTIFIER,           258 },
    { TOK_BIT_AND,              259 },
    { TOK_SELF,                 260 },
    /* State 234. */
    /* State 235. */
    /* State 236. */
    /* State 237. */
    /* State 238. */
    /* State 239. */
    { TOK_IDENTIFIER,            21 },
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_ASSIGN,                28 },
    { TOK_MULTIPLY,              23 },
    { TOK_LEFT_PAREN,            27 },
    { TOK_MODIFIER,              29 },
    { TOK_SEMICOLON,            263 },
    { TOK_UNALIGNED,             32 },
    /* State 240. */
    { TOK_SEMICOLON,            264 },
    /* State 241. */
    /* State 242. */
    /* State 243. */
    /* State 244. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 245. */
    { TOK_IDENTIFIER,            21 },
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_MULTIPLY,             214 },
    { TOK_LEFT_PAREN,           217 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    /* State 246. */
    /* State 247. */
    /* State 248. */
    { TOK_LEFT_BRACKET,         244 },
    /* State 249. */
    { TOK_TYPE_NAME,             11 },
    { TOK_CLASS_HEAD,            15 },
    { TOK_ENUM_TAG,              16 },
    { PARSER_ERROR_TOKEN,       271 },
    /* State 250. */
    { TOK_RIGHT_PAREN,          272 },
    /* State 251. */
    { TOK_LEFT_BRACKET,         244 },
    /* State 252. */
    /* State 253. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 254. */
    /* State 255. */
    /* State 256. */
    { TOK_COMMA,                222 },
    /* State 257. */
    { TOK_BASED,                 33 },
    { TOK_CONST,                 30 },
    { TOK_VOLATILE,              31 },
    { TOK_MULTIPLY,             229 },
    { TOK_LEFT_PAREN,           230 },
    { TOK_MODIFIER,              29 },
    { TOK_UNALIGNED,             32 },
    /* State 258. */
    /* State 259. */
    { TOK_IDENTIFIER,           274 },
    /* State 260. */
    /* State 261. */
    /* State 262. */
    { TOK_COMMA,                 44 },
    { TOK_SEMICOLON,            278 },
    /* State 263. */
    /* State 264. */
    /* State 265. */
    { TOK_RIGHT_BRACKET,        279 },
    /* State 266. */
    { TOK_LEFT_BRACKET,         244 },
    /* State 267. */
    { TOK_COMMA,                281 },
    /* State 268. */
    /* State 269. */
    /* State 270. */
    /* State 271. */
    /* State 272. */
    /* State 273. */
    { TOK_RIGHT_BRACE,          282 },
    /* State 274. */
    /* State 275. */
    { TOK_RIGHT_BRACE,          283 },
    /* State 276. */
    /* State 277. */
    { TOK_IDENTIFIER,           305 },
    { TOK_STRING,                72 },
    { TOK_ASM,                  299 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_BREAK,                295 },
    { TOK_CASE,                 306 },
    { TOK_CONSTANT,              69 },
    { TOK_CONTINUE,             296 },
    { TOK_DEFAULT,              307 },
    { TOK_DO,                   292 },
    { TOK_ELSE,                 303 },
    { TOK_FOR,                  291 },
    { TOK_GOTO,                 298 },
    { TOK_IF,                   308 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_BRACE,            85 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_RETURN,               297 },
    { TOK_SEMICOLON,            302 },
    { TOK_SIZEOF,                64 },
    { TOK_SWITCH,               293 },
    { TOK_TRY,                  309 },
    { TOK_WHILE,                290 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    { TOK_LEAVE,                294 },
    { PARSER_ERROR_TOKEN,       304 },
    /* State 278. */
    /* State 279. */
    /* State 280. */
    { TOK_RIGHT_PAREN,          310 },
    /* State 281. */
    { TOK_ELLIPSIS,             243 },
    { TOK_TYPE_NAME,             11 },
    { TOK_CLASS_HEAD,            15 },
    { TOK_ENUM_TAG,              16 },
    { PARSER_ERROR_TOKEN,       271 },
    /* State 282. */
    /* State 283. */
    /* State 284. */
    /* State 285. */
    /* State 286. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_COMMA,                 98 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SEMICOLON,            312 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 287. */
    /* State 288. */
    /* State 289. */
    /* State 290. */
    /* State 291. */
    /* State 292. */
    /* State 293. */
    { TOK_LEFT_PAREN,           318 },
    /* State 294. */
    { TOK_SEMICOLON,            319 },
    /* State 295. */
    { TOK_SEMICOLON,            320 },
    /* State 296. */
    { TOK_SEMICOLON,            321 },
    /* State 297. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SEMICOLON,            322 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 298. */
    { TOK_IDENTIFIER,           324 },
    /* State 299. */
    /* State 300. */
    { TOK_LEFT_BRACE,            85 },
    /* State 301. */
    { TOK_EXCEPT,               327 },
    { TOK_FINALLY,              326 },
    /* State 302. */
    /* State 303. */
    /* State 304. */
    { TOK_SEMICOLON,            328 },
    /* State 305. */
    { TOK_COLON,                329 },
    { TOK_LEFT_PAREN,           136 },
    /* State 306. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 307. */
    { TOK_COLON,                331 },
    /* State 308. */
    { TOK_LEFT_PAREN,           332 },
    /* State 309. */
    /* State 310. */
    /* State 311. */
    /* State 312. */
    /* State 313. */
    { TOK_ELSE,                 334 },
    /* State 314. */
    /* State 315. */
    { TOK_LEFT_PAREN,           335 },
    /* State 316. */
    { TOK_LEFT_PAREN,           336 },
    /* State 317. */
    /* State 318. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 319. */
    /* State 320. */
    /* State 321. */
    /* State 322. */
    /* State 323. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_COMMA,                 98 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SEMICOLON,            339 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 324. */
    { TOK_SEMICOLON,            340 },
    /* State 325. */
    /* State 326. */
    /* State 327. */
    /* State 328. */
    /* State 329. */
    /* State 330. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_COLON,                343 },
    { TOK_COMMA,                 98 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 331. */
    /* State 332. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 333. */
    { TOK_LEFT_BRACE,            85 },
    /* State 334. */
    /* State 335. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 336. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 337. */
    { TOK_WHILE,                349 },
    /* State 338. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_COMMA,                 98 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_RIGHT_PAREN,          350 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 339. */
    /* State 340. */
    /* State 341. */
    { TOK_LEFT_BRACE,            85 },
    /* State 342. */
    { TOK_LEFT_PAREN,           352 },
    /* State 343. */
    /* State 344. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_COMMA,                 98 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_RIGHT_PAREN,          353 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 345. */
    /* State 346. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_COMMA,                 98 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_RIGHT_PAREN,          354 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 347. */
    { TOK_SEMICOLON,            355 },
    /* State 348. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_COMMA,                 98 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 349. */
    { TOK_LEFT_PAREN,           356 },
    /* State 350. */
    /* State 351. */
    /* State 352. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 353. */
    /* State 354. */
    /* State 355. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 356. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 357. */
    /* State 358. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_COMMA,                 98 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_RIGHT_PAREN,          363 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 359. */
    /* State 360. */
    { TOK_SEMICOLON,            365 },
    /* State 361. */
    { TOK_BIT_AND,              105 },
    { TOK_LOGICAL_AND,          108 },
    { TOK_ASSIGN,               111 },
    { TOK_COMMA,                 98 },
    { TOK_MINUS,                101 },
    { TOK_MULTIPLY,             103 },
    { TOK_BIT_OR,               106 },
    { TOK_LOGICAL_OR,           109 },
    { TOK_PLUS,                 100 },
    { TOK_QUESTION,             112 },
    { TOK_BIT_XOR,              107 },
    { TOK_ASSIGNMENT_OPERATOR,  110 },
    { TOK_DIVISION_OPERATOR,     99 },
    { TOK_EQUALITY_OPERATOR,    104 },
    { TOK_RELATIONAL_OPERATOR,   97 },
    { TOK_RIGHT_PAREN,          366 },
    { TOK_SHIFT_OPERATOR,       102 },
    /* State 362. */
    /* State 363. */
    /* State 364. */
    /* State 365. */
    { TOK_IDENTIFIER,            68 },
    { TOK_STRING,                72 },
    { TOK_SEGNAME,               73 },
    { TOK_BIT_AND,               63 },
    { TOK_LOGICAL_NOT,           59 },
    { TOK_MINUS,                 57 },
    { TOK_MULTIPLY,              62 },
    { TOK_PLUS,                  56 },
    { TOK_BIT_NOT,               58 },
    { TOK_CONSTANT,              69 },
    { TOK_INCREMENT_OPERATOR,    60 },
    { TOK_LEFT_PAREN,            61 },
    { TOK_SIZEOF,                64 },
    { TOK_BUILTIN_ALIGNOF,       65 },
    /* State 366. */
    { TOK_SEMICOLON,            368 },
    /* State 367. */
    /* State 368. */
    /* State 369. */
    { TOK_RIGHT_PAREN,          370 },
    /* State 370. */
    /* State 371. */
    /* State 372. */
    /* State 373. */
    { TOK_RIGHT_PAREN,          198 },
    /* State 374. */
    { TOK_RIGHT_PAREN,          198 },
    /* State 375. */
    { TOK_RIGHT_PAREN,          198 },
};

/* A negative rule accepts the translation unit. */
ParserTransition parser_exceptions[] = {
    { TOK_NONE,                 PARSER_ACCEPT },
    { TOK_IDENTIFIER,            23 },
    { TOK_BASED,                 23 },
    { TOK_CONST,                 23 },
    { TOK_VOLATILE,              23 },
    { TOK_ASSIGN,                23 },
    { TOK_MULTIPLY,              23 },
    { TOK_LEFT_PAREN,            23 },
    { TOK_MODIFIER,              23 },
    { TOK_UNALIGNED,             23 },
    { TOK_COMMA,                 52 },
    { TOK_SEMICOLON,             52 },
    { TOK_SEMICOLON,             40 },
    { TOK_COLON,                 49 },
    { TOK_LEFT_BRACKET,         106 },
    { TOK_LEFT_BRACKET,         106 },
    { TOK_RIGHT_BRACE,          121 },
};

ParserState parser_states[376] = {
    {   1, 0,    0,  0,  0,  0 }, /* state 0 */
    {   0, 1,    0,  6,  0, 10 }, /* state 1 */
    {   2, 0,    6,  0, 10,  0 }, /* state 2 */
    {   3, 0,    6,  0, 10,  0 }, /* state 3 */
    {  14, 0,    6,  0, 10,  0 }, /* state 4 */
    {   0, 1,    6,  9, 10,  0 }, /* state 5 */
    {   7, 0,   15,  0, 10,  0 }, /* state 6 */
    {  24, 1,   15,  1, 10,  0 }, /* state 7 */
    {   0, 1,   16,  2, 10,  0 }, /* state 8 */
    {  12, 0,   18,  0, 10,  0 }, /* state 9 */
    {  25, 0,   18,  0, 10,  0 }, /* state 10 */
    {  26, 0,   18,  0, 10,  0 }, /* state 11 */
    {  27, 0,   18,  0, 10,  0 }, /* state 12 */
    {  28, 0,   18,  0, 10,  0 }, /* state 13 */
    {  29, 0,   18,  0, 10,  0 }, /* state 14 */
    {   0, 1,   18,  1, 10,  0 }, /* state 15 */
    {   0, 1,   19,  1, 10,  0 }, /* state 16 */
    {   4, 1,   20,  4, 10,  0 }, /* state 17 */
    {   0, 1,   24,  2, 10,  0 }, /* state 18 */
    {  13, 1,   26,  3, 10,  2 }, /* state 19 */
    {  50, 0,   29,  0, 12,  0 }, /* state 20 */
    {  63, 0,   29,  0, 12,  0 }, /* state 21 */
    {   0, 1,   29,  7, 12,  0 }, /* state 22 */
    {   0, 1,   36,  8, 12,  0 }, /* state 23 */
    {  69, 0,   44,  0, 12,  0 }, /* state 24 */
    {  54, 1,   44, 14, 12,  0 }, /* state 25 */
    {  90, 0,   58,  0, 12,  0 }, /* state 26 */
    {   0, 1,   58,  8, 12,  0 }, /* state 27 */
    {  57, 0,   66,  0, 12,  0 }, /* state 28 */
    {  92, 0,   66,  0, 12,  0 }, /* state 29 */
    {  93, 0,   66,  0, 12,  0 }, /* state 30 */
    {  94, 0,   66,  0, 12,  0 }, /* state 31 */
    {  95, 0,   66,  0, 12,  0 }, /* state 32 */
    {   0, 1,   66,  1, 12,  0 }, /* state 33 */
    {   8, 0,   67,  0, 12,  0 }, /* state 34 */
    {   9, 0,   67,  0, 12,  0 }, /* state 35 */
    {  10, 0,   67,  0, 12,  0 }, /* state 36 */
    {   0, 1,   67,  3, 12,  0 }, /* state 37 */
    {   0, 1,   70,  2, 12,  0 }, /* state 38 */
    {   0, 1,   72,  1, 12,  0 }, /* state 39 */
    {  15, 0,   73,  0, 12,  0 }, /* state 40 */
    {   0, 1,   73,  9, 12,  0 }, /* state 41 */
    {  18, 0,   82,  0, 12,  0 }, /* state 42 */
    {   6, 0,   82,  0, 12,  0 }, /* state 43 */
    {   0, 1,   82,  9, 12,  0 }, /* state 44 */
    {  56, 0,   91,  0, 12,  0 }, /* state 45 */
    { 170, 1,   91, 14, 12,  0 }, /* state 46 */
    {  68, 0,  105,  0, 12,  0 }, /* state 47 */
    {  71, 0,  105,  0, 12,  0 }, /* state 48 */
    {  64, 0,  105,  0, 12,  0 }, /* state 49 */
    {   0, 1,  105,  8, 12,  0 }, /* state 50 */
    {  91, 0,  113,  0, 12,  0 }, /* state 51 */
    {  65, 1,  113,  2, 12,  0 }, /* state 52 */
    {  53, 1,  115, 15, 12,  0 }, /* state 53 */
    {   0, 1,  130,  1, 12,  0 }, /* state 54 */
    { 191, 1,  131,  5, 12,  0 }, /* state 55 */
    {   0, 1,  136, 14, 12,  0 }, /* state 56 */
    {   0, 1,  150, 14, 12,  0 }, /* state 57 */
    {   0, 1,  164, 14, 12,  0 }, /* state 58 */
    {   0, 1,  178, 14, 12,  0 }, /* state 59 */
    {   0, 1,  192, 14, 12,  0 }, /* state 60 */
    { 218, 1,  206, 14, 12,  0 }, /* state 61 */
    {   0, 1,  220, 14, 12,  0 }, /* state 62 */
    {   0, 1,  234, 14, 12,  0 }, /* state 63 */
    {   0, 1,  248, 14, 12,  0 }, /* state 64 */
    {   0, 1,  262,  1, 12,  0 }, /* state 65 */
    {   0, 1,  263, 14, 12,  0 }, /* state 66 */
    {   0, 1,  277, 15, 12,  0 }, /* state 67 */
    { 211, 1,  292,  1, 12,  0 }, /* state 68 */
    { 212, 0,  293,  0, 12,  0 }, /* state 69 */
    { 213, 1,  293,  1, 12,  0 }, /* state 70 */
    { 214, 0,  294,  0, 12,  0 }, /* state 71 */
    { 216, 0,  294,  0, 12,  0 }, /* state 72 */
    {   0, 1,  294,  1, 12,  0 }, /* state 73 */
    {   0, 1,  295,  3, 12,  0 }, /* state 74 */
    {   0, 1,  298,  4, 12,  0 }, /* state 75 */
    {   0, 1,  302,  1, 12,  0 }, /* state 76 */
    {   0, 1,  303,  4, 12,  0 }, /* state 77 */
    {  37, 0,  307,  0, 12,  0 }, /* state 78 */
    {  48, 1,  307,  8, 12,  1 }, /* state 79 */
    { 225, 1,  315,  1, 13,  0 }, /* state 80 */
    {  32, 0,  316,  0, 13,  0 }, /* state 81 */
    {  33, 0,  316,  0, 13,  0 }, /* state 82 */
    {  35, 1,  316,  1, 13,  0 }, /* state 83 */
    {   5, 0,  317,  0, 13,  0 }, /* state 84 */
    { 119, 0,  317,  0, 13,  0 }, /* state 85 */
    {   0, 1,  317,  3, 13,  0 }, /* state 86 */
    {  17, 0,  320,  0, 13,  0 }, /* state 87 */
    {  19, 0,  320,  0, 13,  0 }, /* state 88 */
    {  22, 1,  320,  2, 13,  0 }, /* state 89 */
    {  51, 0,  322,  0, 13,  0 }, /* state 90 */
    {  52, 1,  322,  3, 13,  0 }, /* state 91 */
    {   0, 1,  325,  1, 13,  0 }, /* state 92 */
    { 171, 0,  326,  0, 13,  0 }, /* state 93 */
    { 172, 1,  326, 15, 13,  0 }, /* state 94 */
    {  73, 1,  341,  5, 13,  0 }, /* state 95 */
    {  66, 1,  346,  2, 13,  0 }, /* state 96 */
    {   0, 1,  348, 14, 13,  0 }, /* state 97 */
    {   0, 1,  362, 14, 13,  0 }, /* state 98 */
    {   0, 1,  376, 14, 13,  0 }, /* state 99 */
    {   0, 1,  390, 14, 13,  0 }, /* state 100 */
    {   0, 1,  404, 14, 13,  0 }, /* state 101 */
    {   0, 1,  418, 14, 13,  0 }, /* state 102 */
    {   0, 1,  432, 14, 13,  0 }, /* state 103 */
    {   0, 1,  446, 14, 13,  0 }, /* state 104 */
    {   0, 1,  460, 14, 13,  0 }, /* state 105 */
    {   0, 1,  474, 14, 13,  0 }, /* state 106 */
    {   0, 1,  488, 14, 13,  0 }, /* state 107 */
    {   0, 1,  502, 14, 13,  0 }, /* state 108 */
    {   0, 1,  516, 14, 13,  0 }, /* state 109 */
    {   0, 1,  530, 14, 13,  0 }, /* state 110 */
    {   0, 1,  544, 14, 13,  0 }, /* state 111 */
    {   0, 1,  558, 14, 13,  0 }, /* state 112 */
    {   0, 1,  572, 15, 13,  0 }, /* state 113 */
    { 192, 0,  587,  0, 13,  0 }, /* state 114 */
    {   0, 1,  587, 14, 13,  0 }, /* state 115 */
    { 208, 0,  601,  0, 13,  0 }, /* state 116 */
    {   0, 1,  601, 14, 13,  0 }, /* state 117 */
    { 222, 0,  615,  0, 13,  0 }, /* state 118 */
    { 193, 1,  615,  5, 13,  0 }, /* state 119 */
    { 194, 1,  620,  5, 13,  0 }, /* state 120 */
    { 195, 1,  625,  5, 13,  0 }, /* state 121 */
    { 196, 1,  630,  5, 13,  0 }, /* state 122 */
    { 197, 1,  635,  4, 13,  0 }, /* state 123 */
    {   0, 1,  639, 17, 13,  0 }, /* state 124 */
    {   0, 1,  656,  3, 13,  0 }, /* state 125 */
    { 199, 1,  659,  5, 13,  0 }, /* state 126 */
    { 200, 1,  664,  5, 13,  0 }, /* state 127 */
    { 201, 1,  669,  5, 13,  0 }, /* state 128 */
    { 202, 1,  674, 10, 13,  0 }, /* state 129 */
    { 203, 0,  684,  0, 13,  0 }, /* state 130 */
    { 218, 0,  684,  0, 13,  0 }, /* state 131 */
    { 204, 1,  684,  5, 13,  0 }, /* state 132 */
    { 206, 0,  689,  0, 13,  0 }, /* state 133 */
    {   0, 1,  689,  2, 13,  0 }, /* state 134 */
    { 223, 1,  691, 15, 13,  0 }, /* state 135 */
    { 221, 0,  706,  0, 13,  0 }, /* state 136 */
    { 217, 0,  706,  0, 13,  0 }, /* state 137 */
    {   0, 1,  706,  1, 13,  0 }, /* state 138 */
    {  70, 0,  707,  0, 13,  0 }, /* state 139 */
    {   0, 1,  707,  1, 13,  0 }, /* state 140 */
    {  97, 0,  708,  0, 13,  0 }, /* state 141 */
    {  98, 0,  708,  0, 13,  0 }, /* state 142 */
    {  99, 0,  708,  0, 13,  0 }, /* state 143 */
    { 100, 0,  708,  0, 13,  0 }, /* state 144 */
    {   0, 1,  708,  1, 13,  0 }, /* state 145 */
    {  11, 0,  709,  0, 13,  0 }, /* state 146 */
    {  30, 0,  709,  0, 13,  0 }, /* state 147 */
    {  38, 0,  709,  0, 13,  0 }, /* state 148 */
    {   0, 1,  709,  1, 13,  0 }, /* state 149 */
    {  41, 1,  710,  2, 13,  0 }, /* state 150 */
    {  42, 0,  712,  0, 13,  0 }, /* state 151 */
    {  46, 1,  712,  2, 13,  1 }, /* state 152 */
    {   0, 1,  714,  1, 14,  0 }, /* state 153 */
    {   0, 1,  715,  1, 14,  0 }, /* state 154 */
    { 226, 1,  716,  1, 14,  0 }, /* state 155 */
    {   0, 1,  717, 14, 14,  0 }, /* state 156 */
    { 123, 0,  731,  0, 14,  0 }, /* state 157 */
    {  16, 0,  731,  0, 14,  0 }, /* state 158 */
    {   0, 1,  731,  8, 14,  0 }, /* state 159 */
    {  21, 0,  739,  0, 14,  0 }, /* state 160 */
    {  67, 0,  739,  0, 14,  0 }, /* state 161 */
    { 116, 1,  739,  1, 14,  0 }, /* state 162 */
    {  74, 0,  740,  0, 14,  0 }, /* state 163 */
    {  76, 0,  740,  0, 14,  0 }, /* state 164 */
    {  77, 0,  740,  0, 14,  0 }, /* state 165 */
    {  78, 0,  740,  0, 14,  0 }, /* state 166 */
    {  79, 0,  740,  0, 14,  0 }, /* state 167 */
    { 106, 1,  740,  8, 14,  0 }, /* state 168 */
    { 175, 1,  748,  5, 14,  0 }, /* state 169 */
    { 176, 1,  753, 15, 14,  0 }, /* state 170 */
    { 177, 0,  768,  0, 14,  0 }, /* state 171 */
    { 178, 1,  768,  2, 14,  0 }, /* state 172 */
    { 179, 1,  770,  2, 14,  0 }, /* state 173 */
    { 180, 1,  772,  4, 14,  0 }, /* state 174 */
    { 181, 0,  776,  0, 14,  0 }, /* state 175 */
    { 182, 1,  776,  6, 14,  0 }, /* state 176 */
    { 183, 1,  782,  7, 14,  0 }, /* state 177 */
    { 184, 1,  789,  9, 14,  0 }, /* state 178 */
    { 185, 1,  798,  8, 14,  0 }, /* state 179 */
    { 186, 1,  806, 10, 14,  0 }, /* state 180 */
    { 187, 1,  816, 11, 14,  0 }, /* state 181 */
    { 188, 1,  827, 15, 14,  0 }, /* state 182 */
    { 189, 1,  842, 15, 14,  0 }, /* state 183 */
    {   0, 1,  857, 17, 14,  0 }, /* state 184 */
    { 225, 1,  874,  1, 14,  0 }, /* state 185 */
    {  58, 0,  875,  0, 14,  0 }, /* state 186 */
    {  60, 1,  875, 15, 14,  0 }, /* state 187 */
    {  61, 0,  890,  0, 14,  0 }, /* state 188 */
    {   0, 1,  890, 17, 14,  0 }, /* state 189 */
    {   0, 1,  907,  1, 14,  0 }, /* state 190 */
    { 210, 1,  908,  4, 14,  0 }, /* state 191 */
    { 198, 0,  912,  0, 14,  0 }, /* state 192 */
    {   0, 1,  912,  1, 14,  0 }, /* state 193 */
    { 106, 1,  913,  7, 14,  0 }, /* state 194 */
    { 207, 0,  920,  0, 14,  0 }, /* state 195 */
    {   0, 1,  920, 14, 14,  0 }, /* state 196 */
    {   0, 1,  934,  2, 14,  0 }, /* state 197 */
    {  96, 0,  936,  0, 14,  0 }, /* state 198 */
    {   0, 1,  936,  1, 14,  0 }, /* state 199 */
    {  39, 0,  937,  0, 14,  0 }, /* state 200 */
    {  48, 1,  937,  9, 14,  0 }, /* state 201 */
    {  45, 0,  946,  0, 14,  0 }, /* state 202 */
    {   0, 1,  946, 14, 14,  0 }, /* state 203 */
    {  31, 0,  960,  0, 14,  0 }, /* state 204 */
    {  34, 0,  960,  0, 14,  0 }, /* state 205 */
    {  36, 0,  960,  0, 14,  0 }, /* state 206 */
    { 120, 1,  960,  4, 14,  0 }, /* state 207 */
    {  20, 0,  964,  0, 14,  0 }, /* state 208 */
    {   0, 1,  964,  1, 14,  0 }, /* state 209 */
    { 117, 1,  965,  6, 14,  0 }, /* state 210 */
    {  88, 1,  971,  2, 14,  0 }, /* state 211 */
    {  89, 1,  973,  1, 14,  0 }, /* state 212 */
    { 107, 1,  974,  8, 14,  0 }, /* state 213 */
    { 106, 1,  982,  8, 14,  0 }, /* state 214 */
    { 104, 0,  990,  0, 14,  0 }, /* state 215 */
    { 105, 0,  990,  0, 14,  0 }, /* state 216 */
    {  80, 1,  990,  8, 14,  1 }, /* state 217 */
    { 108, 0,  998,  0, 15,  0 }, /* state 218 */
    { 114, 1,  998,  1, 15,  0 }, /* state 219 */
    {   0, 1,  999, 14, 15,  0 }, /* state 220 */
    {   0, 1, 1013,  1, 15,  0 }, /* state 221 */
    { 226, 1, 1014, 15, 15,  0 }, /* state 222 */
    {   0, 1, 1029, 15, 15,  0 }, /* state 223 */
    { 205, 0, 1044,  0, 15,  0 }, /* state 224 */
    { 209, 0, 1044,  0, 15,  0 }, /* state 225 */
    { 219, 0, 1044,  0, 15,  0 }, /* state 226 */
    { 220, 1, 1044,  1, 15,  0 }, /* state 227 */
    { 107, 1, 1045,  7, 15,  0 }, /* state 228 */
    { 106, 1, 1052,  7, 15,  0 }, /* state 229 */
    {  80, 1, 1059,  7, 15,  1 }, /* state 230 */
    { 224, 1, 1066, 15, 16,  0 }, /* state 231 */
    { 215, 0, 1081,  0, 16,  0 }, /* state 232 */
    {   0, 1, 1081,  3, 16,  0 }, /* state 233 */
    {  43, 0, 1084,  0, 16,  0 }, /* state 234 */
    {  44, 0, 1084,  0, 16,  0 }, /* state 235 */
    {  47, 0, 1084,  0, 16,  0 }, /* state 236 */
    { 128, 0, 1084,  0, 16,  0 }, /* state 237 */
    { 124, 0, 1084,  0, 16,  0 }, /* state 238 */
    {   0, 1, 1084, 10, 16,  0 }, /* state 239 */
    {   0, 1, 1094,  1, 16,  0 }, /* state 240 */
    {  72, 0, 1095,  0, 16,  0 }, /* state 241 */
    {  75, 0, 1095,  0, 16,  0 }, /* state 242 */
    { 118, 0, 1095,  0, 16,  0 }, /* state 243 */
    { 170, 1, 1095, 14, 16,  0 }, /* state 244 */
    { 106, 1, 1109,  8, 16,  0 }, /* state 245 */
    { 109, 0, 1117,  0, 16,  0 }, /* state 246 */
    {  80, 0, 1117,  0, 16,  0 }, /* state 247 */
    { 110, 1, 1117,  1, 16,  0 }, /* state 248 */
    {  82, 1, 1118,  4, 16,  0 }, /* state 249 */
    { 105, 1, 1122,  1, 16,  0 }, /* state 250 */
    {   0, 1, 1123,  1, 16,  0 }, /* state 251 */
    { 113, 0, 1124,  0, 16,  0 }, /* state 252 */
    { 190, 1, 1124, 13, 16,  0 }, /* state 253 */
    {  55, 0, 1137,  0, 16,  0 }, /* state 254 */
    {  59, 0, 1137,  0, 16,  0 }, /* state 255 */
    { 225, 1, 1137,  1, 16,  0 }, /* state 256 */
    { 106, 1, 1138,  7, 16,  0 }, /* state 257 */
    { 101, 0, 1145,  0, 16,  0 }, /* state 258 */
    {   0, 1, 1145,  1, 16,  0 }, /* state 259 */
    { 103, 0, 1146,  0, 16,  0 }, /* state 260 */
    { 163, 1, 1146,  0, 16,  1 }, /* state 261 */
    {   0, 1, 1146,  2, 17,  0 }, /* state 262 */
    { 126, 0, 1148,  0, 17,  0 }, /* state 263 */
    { 127, 0, 1148,  0, 17,  0 }, /* state 264 */
    {   0, 1, 1148,  1, 17,  0 }, /* state 265 */
    { 111, 1, 1149,  1, 17,  0 }, /* state 266 */
    { 116, 1, 1150,  1, 17,  0 }, /* state 267 */
    {  83, 0, 1151,  0, 17,  0 }, /* state 268 */
    {  85, 0, 1151,  0, 17,  0 }, /* state 269 */
    {  86, 0, 1151,  0, 17,  0 }, /* state 270 */
    {  87, 0, 1151,  0, 17,  0 }, /* state 271 */
    { 115, 0, 1151,  0, 17,  0 }, /* state 272 */
    {   0, 1, 1151,  1, 17,  0 }, /* state 273 */
    { 102, 0, 1152,  0, 17,  0 }, /* state 274 */
    {   0, 1, 1152,  1, 17,  0 }, /* state 275 */
    { 129, 0, 1153,  0, 17,  0 }, /* state 276 */
    {   0, 1, 1153, 32, 17,  0 }, /* state 277 */
    { 125, 0, 1185,  0, 17,  0 }, /* state 278 */
    { 112, 0, 1185,  0, 17,  0 }, /* state 279 */
    {   0, 1, 1185,  1, 17,  0 }, /* state 280 */
    { 117, 1, 1186,  5, 17,  0 }, /* state 281 */
    {  62, 0, 1191,  0, 17,  0 }, /* state 282 */
    { 122, 0, 1191,  0, 17,  0 }, /* state 283 */
    { 130, 0, 1191,  0, 17,  0 }, /* state 284 */
    { 164, 0, 1191,  0, 17,  0 }, /* state 285 */
    {   0, 1, 1191, 17, 17,  0 }, /* state 286 */
    { 132, 0, 1208,  0, 17,  0 }, /* state 287 */
    { 163, 0, 1208,  0, 17,  0 }, /* state 288 */
    { 163, 0, 1208,  0, 17,  0 }, /* state 289 */
    { 135, 0, 1208,  0, 17,  0 }, /* state 290 */
    { 138, 0, 1208,  0, 17,  0 }, /* state 291 */
    { 142, 0, 1208,  0, 17,  0 }, /* state 292 */
    {   0, 1, 1208,  1, 17,  0 }, /* state 293 */
    {   0, 1, 1209,  1, 17,  0 }, /* state 294 */
    {   0, 1, 1210,  1, 17,  0 }, /* state 295 */
    {   0, 1, 1211,  1, 17,  0 }, /* state 296 */
    {   0, 1, 1212, 15, 17,  0 }, /* state 297 */
    {   0, 1, 1227,  1, 17,  0 }, /* state 298 */
    { 152, 0, 1228,  0, 17,  0 }, /* state 299 */
    {   0, 1, 1228,  1, 17,  0 }, /* state 300 */
    {   0, 1, 1229,  2, 17,  0 }, /* state 301 */
    { 156, 0, 1231,  0, 17,  0 }, /* state 302 */
    { 157, 0, 1231,  0, 17,  0 }, /* state 303 */
    {   0, 1, 1231,  1, 17,  0 }, /* state 304 */
    { 211, 1, 1232,  2, 17,  0 }, /* state 305 */
    {   0, 1, 1234, 14, 17,  0 }, /* state 306 */
    {   0, 1, 1248,  1, 17,  0 }, /* state 307 */
    {   0, 1, 1249,  1, 17,  0 }, /* state 308 */
    { 159, 0, 1250,  0, 17,  0 }, /* state 309 */
    {  81, 0, 1250,  0, 17,  0 }, /* state 310 */
    {  84, 0, 1250,  0, 17,  0 }, /* state 311 */
    { 131, 0, 1250,  0, 17,  0 }, /* state 312 */
    { 133, 1, 1250,  1, 17,  0 }, /* state 313 */
    { 134, 0, 1251,  0, 17,  0 }, /* state 314 */
    {   0, 1, 1251,  1, 17,  0 }, /* state 315 */
    {   0, 1, 1252,  1, 17,  0 }, /* state 316 */
    { 163, 0, 1253,  0, 17,  0 }, /* state 317 */
    {   0, 1, 1253, 14, 17,  0 }, /* state 318 */
    { 146, 0, 1267,  0, 17,  0 }, /* state 319 */
    { 147, 0, 1267,  0, 17,  0 }, /* state 320 */
    { 148, 0, 1267,  0, 17,  0 }, /* state 321 */
    { 149, 0, 1267,  0, 17,  0 }, /* state 322 */
    {   0, 1, 1267, 17, 17,  0 }, /* state 323 */
    {   0, 1, 1284,  1, 17,  0 }, /* state 324 */
    { 153, 0, 1285,  0, 17,  0 }, /* state 325 */
    { 154, 0, 1285,  0, 17,  0 }, /* state 326 */
    { 161, 0, 1285,  0, 17,  0 }, /* state 327 */
    { 158, 0, 1285,  0, 17,  0 }, /* state 328 */
    { 165, 0, 1285,  0, 17,  0 }, /* state 329 */
    {   0, 1, 1285, 17, 17,  0 }, /* state 330 */
    { 167, 0, 1302,  0, 17,  0 }, /* state 331 */
    {   0, 1, 1302, 14, 17,  0 }, /* state 332 */
    {   0, 1, 1316,  1, 17,  0 }, /* state 333 */
    { 169, 0, 1317,  0, 17,  0 }, /* state 334 */
    {   0, 1, 1317, 14, 17,  0 }, /* state 335 */
    { 173, 1, 1331, 14, 17,  0 }, /* state 336 */
    {   0, 1, 1345,  1, 17,  0 }, /* state 337 */
    {   0, 1, 1346, 17, 17,  0 }, /* state 338 */
    { 150, 0, 1363,  0, 17,  0 }, /* state 339 */
    { 151, 0, 1363,  0, 17,  0 }, /* state 340 */
    {   0, 1, 1363,  1, 17,  0 }, /* state 341 */
    {   0, 1, 1364,  1, 17,  0 }, /* state 342 */
    { 166, 0, 1365,  0, 17,  0 }, /* state 343 */
    {   0, 1, 1365, 17, 17,  0 }, /* state 344 */
    { 160, 0, 1382,  0, 17,  0 }, /* state 345 */
    {   0, 1, 1382, 17, 17,  0 }, /* state 346 */
    {   0, 1, 1399,  1, 17,  0 }, /* state 347 */
    { 174, 1, 1400, 16, 17,  0 }, /* state 348 */
    {   0, 1, 1416,  1, 17,  0 }, /* state 349 */
    { 144, 0, 1417,  0, 17,  0 }, /* state 350 */
    { 155, 0, 1417,  0, 17,  0 }, /* state 351 */
    {   0, 1, 1417, 14, 17,  0 }, /* state 352 */
    { 168, 0, 1431,  0, 17,  0 }, /* state 353 */
    { 136, 0, 1431,  0, 17,  0 }, /* state 354 */
    { 173, 1, 1431, 14, 17,  0 }, /* state 355 */
    {   0, 1, 1445, 14, 17,  0 }, /* state 356 */
    { 163, 0, 1459,  0, 17,  0 }, /* state 357 */
    {   0, 1, 1459, 17, 17,  0 }, /* state 358 */
    { 163, 0, 1476,  0, 17,  0 }, /* state 359 */
    {   0, 1, 1476,  1, 17,  0 }, /* state 360 */
    {   0, 1, 1477, 17, 17,  0 }, /* state 361 */
    { 145, 0, 1494,  0, 17,  0 }, /* state 362 */
    { 162, 0, 1494,  0, 17,  0 }, /* state 363 */
    { 137, 0, 1494,  0, 17,  0 }, /* state 364 */
    { 173, 1, 1494, 14, 17,  0 }, /* state 365 */
    {   0, 1, 1508,  1, 17,  0 }, /* state 366 */
    { 139, 0, 1509,  0, 17,  0 }, /* state 367 */
    { 143, 0, 1509,  0, 17,  0 }, /* state 368 */
    {   0, 1, 1509,  1, 17,  0 }, /* state 369 */
    { 140, 0, 1510,  0, 17,  0 }, /* state 370 */
    { 163, 0, 1510,  0, 17,  0 }, /* state 371 */
    { 141, 0, 1510,  0, 17,  0 }, /* state 372 */
    {   0, 1, 1510,  1, 17,  0 }, /* state 373 */
    {   0, 1, 1511,  1, 17,  0 }, /* state 374 */
    {   0, 1, 1512,  1, 17,  0 }, /* state 375 */
};

/* Goto rows are keyed by predecessor state, not token. */
ParserTransition parser_goto_transitions[] = {
    /* Nonterminal 1. */
    /* Nonterminal 2. */
    {  22,  51 },
    { 213,  51 },
    { 228,  51 },
    /* Nonterminal 3. */
    /* Nonterminal 4. */
    /* Nonterminal 5. */
    /* Nonterminal 6. */
    {   1,   7 },
    {  17,  41 },
    {  37,  79 },
    {  77,  79 },
    { 125, 194 },
    { 207, 239 },
    /* Nonterminal 7. */
    /* Nonterminal 8. */
    /* Nonterminal 9. */
    /* Nonterminal 10. */
    /* Nonterminal 11. */
    {  25,  53 },
    {  61, 124 },
    {  67, 135 },
    {  97, 169 },
    {  98, 170 },
    {  99, 171 },
    { 100, 172 },
    { 101, 173 },
    { 102, 174 },
    { 103, 175 },
    { 104, 176 },
    { 105, 177 },
    { 106, 178 },
    { 107, 179 },
    { 108, 180 },
    { 109, 181 },
    { 110, 182 },
    { 111, 183 },
    { 112, 184 },
    { 113, 187 },
    { 115, 189 },
    { 196, 231 },
    { 220, 253 },
    { 222, 187 },
    { 223, 187 },
    { 277, 286 },
    { 297, 323 },
    { 306, 330 },
    { 318, 338 },
    { 332, 344 },
    { 335, 346 },
    { 336, 348 },
    { 352, 358 },
    { 355, 348 },
    { 356, 361 },
    { 365, 348 },
    /* Nonterminal 12. */
    { 355, 360 },
    { 365, 367 },
    /* Nonterminal 13. */
    {  56, 119 },
    {  57, 120 },
    {  58, 121 },
    {  59, 122 },
    {  60, 123 },
    {  62, 126 },
    {  63, 127 },
    {  64, 128 },
    {  66, 132 },
    { 117, 191 },
    { 129, 132 },
    /* Nonterminal 14. */
    /* Nonterminal 15. */
    /* Nonterminal 16. */
    /* Nonterminal 17. */
    /* Nonterminal 18. */
    { 138, 197 },
    /* Nonterminal 19. */
    /* Nonterminal 20. */
    /* Nonterminal 21. */
    {   5,  19 },
    {  27,  74 },
    {  41,  89 },
    {  44,  91 },
    {  50,  96 },
    {  79, 152 },
    { 159,  89 },
    { 168, 211 },
    { 201, 152 },
    { 217,  74 },
    { 239,  91 },
    { 245,  96 },
    /* Nonterminal 22. */
    /* Nonterminal 23. */
    { 201, 234 },
    /* Nonterminal 24. */
    {  44,  90 },
    /* Nonterminal 25. */
    /* Nonterminal 26. */
    {  64, 129 },
    {  65, 130 },
    /* Nonterminal 27. */
    { 249, 270 },
    { 281, 270 },
    /* Nonterminal 28. */
    { 249, 269 },
    { 281, 269 },
    /* Nonterminal 29. */
    { 213, 246 },
    { 219, 252 },
    { 228, 246 },
    /* Nonterminal 30. */
    /* Nonterminal 31. */
    { 281, 311 },
    /* Nonterminal 32. */
    /* Nonterminal 33. */
    /* Nonterminal 34. */
    { 210, 242 },
    /* Nonterminal 35. */
    { 217, 250 },
    { 230, 250 },
    /* Nonterminal 36. */
    /* Nonterminal 37. */
    { 168, 212 },
    { 194, 227 },
    { 217, 251 },
    { 230, 251 },
    { 245, 266 },
    { 257, 266 },
    /* Nonterminal 38. */
    /* Nonterminal 39. */
    { 267, 280 },
    /* Nonterminal 40. */
    {  75, 141 },
    /* Nonterminal 41. */
    /* Nonterminal 42. */
    { 244, 265 },
    /* Nonterminal 43. */
    { 156, 206 },
    { 203, 236 },
    /* Nonterminal 44. */
    /* Nonterminal 45. */
    { 168, 213 },
    { 194, 228 },
    { 214, 213 },
    { 217, 213 },
    { 229, 228 },
    { 230, 228 },
    { 245, 213 },
    { 257, 228 },
    /* Nonterminal 46. */
    /* Nonterminal 47. */
    /* Nonterminal 48. */
    /* Nonterminal 49. */
    /* Nonterminal 50. */
    { 277, 287 },
    { 300, 325 },
    { 333, 345 },
    { 341, 351 },
    /* Nonterminal 51. */
    { 239, 262 },
    /* Nonterminal 52. */
    /* Nonterminal 53. */
    /* Nonterminal 54. */
    /* Nonterminal 55. */
    { 159, 208 },
    /* Nonterminal 56. */
    /* Nonterminal 57. */
    /* Nonterminal 58. */
    { 185, 221 },
    { 256, 273 },
    /* Nonterminal 59. */
    { 155, 205 },
    /* Nonterminal 60. */
    {  77, 148 },
    /* Nonterminal 61. */
    /* Nonterminal 62. */
    /* Nonterminal 63. */
    /* Nonterminal 64. */
    { 223, 256 },
    /* Nonterminal 65. */
    { 222, 255 },
    /* Nonterminal 66. */
    /* Nonterminal 67. */
    /* Nonterminal 68. */
    /* Nonterminal 69. */
    /* Nonterminal 70. */
    /* Nonterminal 71. */
    /* Nonterminal 72. */
    /* Nonterminal 73. */
    /* Nonterminal 74. */
    /* Nonterminal 75. */
    { 288, 313 },
    { 289, 314 },
    { 317, 337 },
    { 357, 362 },
    { 359, 364 },
    { 371, 372 },
    /* Nonterminal 76. */
    /* Nonterminal 77. */
    /* Nonterminal 78. */
    /* Nonterminal 79. */
    /* Nonterminal 80. */
    /* Nonterminal 81. */
    /* Nonterminal 82. */
    /* Nonterminal 83. */
    /* Nonterminal 84. */
    /* Nonterminal 85. */
    /* Nonterminal 86. */
    /* Nonterminal 87. */
    /* Nonterminal 88. */
    /* Nonterminal 89. */
    /* Nonterminal 90. */
    /* Nonterminal 91. */
};

ParserGoto parser_gotos[92] = {
    {   0,   0,  0 }, /* nonterminal 0 */
    {   1,   0,  0 }, /* nonterminal 1 */
    {  26,   0,  3 }, /* nonterminal 2 */
    { 140,   3,  0 }, /* nonterminal 3 */
    { 142,   3,  0 }, /* nonterminal 4 */
    {   5,   3,  0 }, /* nonterminal 5 */
    { 168,   3,  6 }, /* nonterminal 6 */
    {  10,   9,  0 }, /* nonterminal 7 */
    {  12,   9,  0 }, /* nonterminal 8 */
    {  13,   9,  0 }, /* nonterminal 9 */
    {  14,   9,  0 }, /* nonterminal 10 */
    {  94,   9, 36 }, /* nonterminal 11 */
    { 347,  45,  2 }, /* nonterminal 12 */
    {  55,  47, 11 }, /* nonterminal 13 */
    { 134,  58,  0 }, /* nonterminal 14 */
    {  67,  58,  0 }, /* nonterminal 15 */
    { 288,  58,  0 }, /* nonterminal 16 */
    { 289,  58,  0 }, /* nonterminal 17 */
    {  70,  58,  1 }, /* nonterminal 18 */
    { 153,  59,  0 }, /* nonterminal 19 */
    {  24,  59,  0 }, /* nonterminal 20 */
    {  52,  59, 12 }, /* nonterminal 21 */
    {  25,  71,  0 }, /* nonterminal 22 */
    { 151,  71,  1 }, /* nonterminal 23 */
    {  20,  72,  1 }, /* nonterminal 24 */
    { 193,  73,  0 }, /* nonterminal 25 */
    {  66,  73,  2 }, /* nonterminal 26 */
    { 166,  75,  2 }, /* nonterminal 27 */
    { 165,  77,  2 }, /* nonterminal 28 */
    { 218,  79,  3 }, /* nonterminal 29 */
    { 267,  82,  0 }, /* nonterminal 30 */
    { 268,  82,  1 }, /* nonterminal 31 */
    {  47,  83,  0 }, /* nonterminal 32 */
    { 162,  83,  0 }, /* nonterminal 33 */
    { 163,  83,  1 }, /* nonterminal 34 */
    { 216,  84,  2 }, /* nonterminal 35 */
    { 219,  86,  0 }, /* nonterminal 36 */
    { 248,  86,  6 }, /* nonterminal 37 */
    { 215,  92,  0 }, /* nonterminal 38 */
    { 209,  92,  1 }, /* nonterminal 39 */
    {  71,  93,  1 }, /* nonterminal 40 */
    {   4,  94,  0 }, /* nonterminal 41 */
    {  92,  94,  1 }, /* nonterminal 42 */
    {  93,  95,  2 }, /* nonterminal 43 */
    { 284,  97,  0 }, /* nonterminal 44 */
    {  22,  97,  8 }, /* nonterminal 45 */
    {   2, 105,  0 }, /* nonterminal 46 */
    {   3, 105,  0 }, /* nonterminal 47 */
    {  17, 105,  0 }, /* nonterminal 48 */
    {  39, 105,  0 }, /* nonterminal 49 */
    {  84, 105,  4 }, /* nonterminal 50 */
    {  18, 109,  1 }, /* nonterminal 51 */
    {  76, 110,  0 }, /* nonterminal 52 */
    {  40, 110,  0 }, /* nonterminal 53 */
    {  86, 110,  0 }, /* nonterminal 54 */
    {  88, 110,  1 }, /* nonterminal 55 */
    {  77, 111,  0 }, /* nonterminal 56 */
    {  80, 111,  0 }, /* nonterminal 57 */
    { 154, 111,  2 }, /* nonterminal 58 */
    {  82, 113,  1 }, /* nonterminal 59 */
    {  78, 114,  1 }, /* nonterminal 60 */
    { 149, 115,  0 }, /* nonterminal 61 */
    { 150, 115,  0 }, /* nonterminal 62 */
    {  54, 115,  0 }, /* nonterminal 63 */
    { 185, 115,  1 }, /* nonterminal 64 */
    { 186, 116,  1 }, /* nonterminal 65 */
    { 223, 117,  0 }, /* nonterminal 66 */
    {  95, 117,  0 }, /* nonterminal 67 */
    { 249, 117,  0 }, /* nonterminal 68 */
    { 157, 117,  0 }, /* nonterminal 69 */
    { 207, 117,  0 }, /* nonterminal 70 */
    { 237, 117,  0 }, /* nonterminal 71 */
    { 261, 117,  0 }, /* nonterminal 72 */
    { 275, 117,  0 }, /* nonterminal 73 */
    { 238, 117,  0 }, /* nonterminal 74 */
    { 276, 117,  6 }, /* nonterminal 75 */
    { 277, 123,  0 }, /* nonterminal 76 */
    { 315, 123,  0 }, /* nonterminal 77 */
    { 359, 123,  0 }, /* nonterminal 78 */
    { 316, 123,  0 }, /* nonterminal 79 */
    { 369, 123,  0 }, /* nonterminal 80 */
    { 371, 123,  0 }, /* nonterminal 81 */
    { 317, 123,  0 }, /* nonterminal 82 */
    { 357, 123,  0 }, /* nonterminal 83 */
    { 300, 123,  0 }, /* nonterminal 84 */
    { 301, 123,  0 }, /* nonterminal 85 */
    { 341, 123,  0 }, /* nonterminal 86 */
    { 333, 123,  0 }, /* nonterminal 87 */
    { 342, 123,  0 }, /* nonterminal 88 */
    { 285, 123,  0 }, /* nonterminal 89 */
    { 190, 123,  0 }, /* nonterminal 90 */
    { 125, 123,  0 }, /* nonterminal 91 */
};

/* Production left-hand nonterminal and right-hand symbol count.
   Semantic actions are kept together in parser.c. */
ParserProduction parser_productions[225] = {
    {  0,  0 }, /* rule 0 */
    {  1,  0 }, /* rule 1 */
    {  1,  2 }, /* rule 2 */
    { 46,  1 }, /* rule 3 */
    { 49,  0 }, /* rule 4 */
    { 47,  4 }, /* rule 5 */
    { 47,  3 }, /* rule 6 */
    { 47,  1 }, /* rule 7 */
    { 47,  2 }, /* rule 8 */
    { 47,  2 }, /* rule 9 */
    { 52,  0 }, /* rule 10 */
    { 47,  4 }, /* rule 11 */
    { 47,  1 }, /* rule 12 */
    { 41,  2 }, /* rule 13 */
    { 48,  0 }, /* rule 14 */
    { 48,  2 }, /* rule 15 */
    { 53,  3 }, /* rule 16 */
    { 53,  2 }, /* rule 17 */
    { 53,  1 }, /* rule 18 */
    { 54,  1 }, /* rule 19 */
    { 54,  3 }, /* rule 20 */
    { 54,  2 }, /* rule 21 */
    { 55,  1 }, /* rule 22 */
    {  5,  0 }, /* rule 23 */
    {  5,  1 }, /* rule 24 */
    {  6,  1 }, /* rule 25 */
    {  7,  1 }, /* rule 26 */
    {  7,  1 }, /* rule 27 */
    {  8,  1 }, /* rule 28 */
    {  8,  1 }, /* rule 29 */
    {  9,  4 }, /* rule 30 */
    { 10,  5 }, /* rule 31 */
    { 10,  3 }, /* rule 32 */
    { 57,  1 }, /* rule 33 */
    { 57,  3 }, /* rule 34 */
    { 59,  1 }, /* rule 35 */
    { 59,  3 }, /* rule 36 */
    { 56,  1 }, /* rule 37 */
    { 56,  2 }, /* rule 38 */
    { 60,  3 }, /* rule 39 */
    { 61,  0 }, /* rule 40 */
    { 61,  1 }, /* rule 41 */
    { 62,  1 }, /* rule 42 */
    { 62,  3 }, /* rule 43 */
    { 62,  3 }, /* rule 44 */
    { 62,  2 }, /* rule 45 */
    { 23,  1 }, /* rule 46 */
    { 23,  3 }, /* rule 47 */
    { 19,  0 }, /* rule 48 */
    { 19,  1 }, /* rule 49 */
    { 51,  1 }, /* rule 50 */
    { 51,  3 }, /* rule 51 */
    { 24,  1 }, /* rule 52 */
    { 24,  2 }, /* rule 53 */
    { 63,  0 }, /* rule 54 */
    { 24,  6 }, /* rule 55 */
    { 22,  2 }, /* rule 56 */
    { 22,  1 }, /* rule 57 */
    { 64,  1 }, /* rule 58 */
    { 64,  3 }, /* rule 59 */
    { 65,  1 }, /* rule 60 */
    { 66,  0 }, /* rule 61 */
    { 65,  5 }, /* rule 62 */
    { 21,  1 }, /* rule 63 */
    { 21,  2 }, /* rule 64 */
    { 21,  2 }, /* rule 65 */
    { 21,  3 }, /* rule 66 */
    { 21,  4 }, /* rule 67 */
    { 21,  2 }, /* rule 68 */
    { 21,  1 }, /* rule 69 */
    { 20,  3 }, /* rule 70 */
    { 67,  0 }, /* rule 71 */
    { 32,  5 }, /* rule 72 */
    { 33,  0 }, /* rule 73 */
    { 33,  1 }, /* rule 74 */
    { 33,  3 }, /* rule 75 */
    { 34,  1 }, /* rule 76 */
    { 34,  1 }, /* rule 77 */
    { 34,  1 }, /* rule 78 */
    { 34,  1 }, /* rule 79 */
    { 68,  0 }, /* rule 80 */
    { 29,  5 }, /* rule 81 */
    { 30,  0 }, /* rule 82 */
    { 30,  1 }, /* rule 83 */
    { 30,  3 }, /* rule 84 */
    { 31,  1 }, /* rule 85 */
    { 31,  1 }, /* rule 86 */
    { 31,  1 }, /* rule 87 */
    { 28,  2 }, /* rule 88 */
    { 27,  2 }, /* rule 89 */
    { 45,  1 }, /* rule 90 */
    { 45,  2 }, /* rule 91 */
    {  2,  1 }, /* rule 92 */
    {  2,  1 }, /* rule 93 */
    {  2,  1 }, /* rule 94 */
    {  2,  1 }, /* rule 95 */
    {  2,  4 }, /* rule 96 */
    {  3,  1 }, /* rule 97 */
    {  3,  1 }, /* rule 98 */
    {  3,  1 }, /* rule 99 */
    {  3,  1 }, /* rule 100 */
    {  4,  4 }, /* rule 101 */
    {  4,  5 }, /* rule 102 */
    {  4,  4 }, /* rule 103 */
    { 37,  1 }, /* rule 104 */
    { 37,  1 }, /* rule 105 */
    { 38,  0 }, /* rule 106 */
    { 38,  1 }, /* rule 107 */
    { 38,  1 }, /* rule 108 */
    { 38,  2 }, /* rule 109 */
    { 35,  2 }, /* rule 110 */
    { 35,  3 }, /* rule 111 */
    { 35,  4 }, /* rule 112 */
    { 35,  2 }, /* rule 113 */
    { 35,  1 }, /* rule 114 */
    { 36,  3 }, /* rule 115 */
    { 39,  0 }, /* rule 116 */
    { 39,  1 }, /* rule 117 */
    { 39,  2 }, /* rule 118 */
    { 69,  0 }, /* rule 119 */
    { 71,  0 }, /* rule 120 */
    { 73,  0 }, /* rule 121 */
    { 50,  7 }, /* rule 122 */
    { 70,  0 }, /* rule 123 */
    { 70,  2 }, /* rule 124 */
    { 74,  3 }, /* rule 125 */
    { 74,  2 }, /* rule 126 */
    { 74,  2 }, /* rule 127 */
    { 72,  0 }, /* rule 128 */
    { 72,  2 }, /* rule 129 */
    { 75,  2 }, /* rule 130 */
    { 44,  2 }, /* rule 131 */
    { 44,  1 }, /* rule 132 */
    { 44,  2 }, /* rule 133 */
    { 44,  2 }, /* rule 134 */
    { 77,  0 }, /* rule 135 */
    { 78,  0 }, /* rule 136 */
    { 44,  7 }, /* rule 137 */
    { 79,  0 }, /* rule 138 */
    { 80,  0 }, /* rule 139 */
    { 81,  0 }, /* rule 140 */
    { 44, 12 }, /* rule 141 */
    { 82,  0 }, /* rule 142 */
    { 44,  8 }, /* rule 143 */
    { 83,  0 }, /* rule 144 */
    { 44,  6 }, /* rule 145 */
    { 44,  2 }, /* rule 146 */
    { 44,  2 }, /* rule 147 */
    { 44,  2 }, /* rule 148 */
    { 44,  2 }, /* rule 149 */
    { 44,  3 }, /* rule 150 */
    { 44,  3 }, /* rule 151 */
    { 44,  1 }, /* rule 152 */
    { 44,  2 }, /* rule 153 */
    { 86,  0 }, /* rule 154 */
    { 44,  4 }, /* rule 155 */
    { 44,  1 }, /* rule 156 */
    { 44,  1 }, /* rule 157 */
    { 44,  2 }, /* rule 158 */
    { 87,  0 }, /* rule 159 */
    { 85,  3 }, /* rule 160 */
    { 88,  0 }, /* rule 161 */
    { 84,  6 }, /* rule 162 */
    { 76,  0 }, /* rule 163 */
    { 76,  2 }, /* rule 164 */
    { 89,  2 }, /* rule 165 */
    { 89,  3 }, /* rule 166 */
    { 89,  2 }, /* rule 167 */
    { 16,  4 }, /* rule 168 */
    { 17,  3 }, /* rule 169 */
    { 42,  0 }, /* rule 170 */
    { 42,  1 }, /* rule 171 */
    { 43,  1 }, /* rule 172 */
    { 12,  0 }, /* rule 173 */
    { 12,  1 }, /* rule 174 */
    { 11,  3 }, /* rule 175 */
    { 11,  3 }, /* rule 176 */
    { 11,  3 }, /* rule 177 */
    { 11,  3 }, /* rule 178 */
    { 11,  3 }, /* rule 179 */
    { 11,  3 }, /* rule 180 */
    { 11,  3 }, /* rule 181 */
    { 11,  3 }, /* rule 182 */
    { 11,  3 }, /* rule 183 */
    { 11,  3 }, /* rule 184 */
    { 11,  3 }, /* rule 185 */
    { 11,  3 }, /* rule 186 */
    { 11,  3 }, /* rule 187 */
    { 11,  3 }, /* rule 188 */
    { 11,  3 }, /* rule 189 */
    { 11,  5 }, /* rule 190 */
    { 11,  1 }, /* rule 191 */
    { 13,  2 }, /* rule 192 */
    { 13,  2 }, /* rule 193 */
    { 13,  2 }, /* rule 194 */
    { 13,  2 }, /* rule 195 */
    { 13,  2 }, /* rule 196 */
    { 13,  2 }, /* rule 197 */
    { 13,  3 }, /* rule 198 */
    { 13,  2 }, /* rule 199 */
    { 13,  2 }, /* rule 200 */
    { 13,  2 }, /* rule 201 */
    { 13,  2 }, /* rule 202 */
    { 13,  2 }, /* rule 203 */
    { 13,  2 }, /* rule 204 */
    { 13,  4 }, /* rule 205 */
    { 13,  2 }, /* rule 206 */
    { 13,  3 }, /* rule 207 */
    { 90,  0 }, /* rule 208 */
    { 13,  4 }, /* rule 209 */
    { 13,  3 }, /* rule 210 */
    { 13,  1 }, /* rule 211 */
    { 13,  1 }, /* rule 212 */
    { 13,  1 }, /* rule 213 */
    { 13,  1 }, /* rule 214 */
    { 40,  4 }, /* rule 215 */
    { 18,  1 }, /* rule 216 */
    { 18,  2 }, /* rule 217 */
    { 91,  0 }, /* rule 218 */
    { 26,  4 }, /* rule 219 */
    { 25,  2 }, /* rule 220 */
    { 15,  2 }, /* rule 221 */
    { 15,  2 }, /* rule 222 */
    { 14,  1 }, /* rule 223 */
    { 14,  3 }, /* rule 224 */
};

/* The yacc recovery extensions insert expected tokens at these states. */
ParserRecovery parser_recoveries[] = {
    {   1, -TOK_SEMICOLON,   6 },
    {   8, -TOK_SEMICOLON,  35 },
    {  18, -TOK_SEMICOLON,  43 },
    {  41, -TOK_SEMICOLON,  87 },
    {  67, -TOK_RIGHT_PAREN, 133 },
    {  74, -TOK_RIGHT_PAREN, 139 },
    {  86, -TOK_SEMICOLON, 158 },
    { 124, -TOK_RIGHT_PAREN, 192 },
    { 134, -TOK_RIGHT_PAREN, 195 },
    { 149, TOK_SEMICOLON, 200 },
    { 150, -TOK_COMMA, 201 },
    { 184, -TOK_COLON, 220 },
    { 189, -TOK_RIGHT_BRACKET, 224 },
    { 197, -TOK_RIGHT_PAREN, 232 },
    { 239, -TOK_SEMICOLON, 263 },
    { 240, TOK_SEMICOLON, 264 },
    { 262, -TOK_SEMICOLON, 278 },
    { 277, -TOK_SEMICOLON, 302 },
    { 286, -TOK_SEMICOLON, 312 },
    { 294, TOK_SEMICOLON, 319 },
    { 295, TOK_SEMICOLON, 320 },
    { 296, TOK_SEMICOLON, 321 },
    { 297, -TOK_SEMICOLON, 322 },
    { 304, TOK_SEMICOLON, 328 },
    { 305, -TOK_COLON, 329 },
    { 323, -TOK_SEMICOLON, 339 },
    { 324, TOK_SEMICOLON, 340 },
    { 330, -TOK_COLON, 343 },
    { 338, -TOK_RIGHT_PAREN, 350 },
    { 344, -TOK_RIGHT_PAREN, 353 },
    { 346, -TOK_RIGHT_PAREN, 354 },
    { 347, TOK_SEMICOLON, 355 },
    { 348, -TOK_COMMA,  98 },
    { 358, -TOK_RIGHT_PAREN, 363 },
    { 360, TOK_SEMICOLON, 365 },
    { 361, -TOK_RIGHT_PAREN, 366 },
    { 366, TOK_SEMICOLON, 368 },
    { -1000, 0, 0 }
};
