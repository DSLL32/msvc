/* Generated from named assembler grammar and precedence declarations.
   These are logical shifts/reductions/gotos, not the original packed tables. */
#include "asm-grammar.h"
#include <recon.h>

ParserTransition asm_shifts[] = {
    /* State 0. */
    { AS_LEFT_BRACE, 6 },
    { AS_ERROR, 4 },
    /* State 1. */
    { AS_LEFT_BRACE, 6 },
    { AS_ERROR, 4 },
    /* State 2. */
    /* State 3. */
    { AS_NEWLINE, 8 },
    /* State 4. */
    { AS_NEWLINE, 9 },
    /* State 5. */
    { AS_IDENTIFIER, 11 },
    /* State 6. */
    /* State 7. */
    /* State 8. */
    /* State 9. */
    /* State 10. */
    { AS_DOT, 15 },
    { AS_PREFIX, 17 },
    /* State 11. */
    { AS_COLON, 18 },
    /* State 12. */
    /* State 13. */
    { AS_DIRECTIVE, 19 },
    /* State 14. */
    { AS_OPCODE, 20 },
    /* State 15. */
    /* State 16. */
    { AS_PREFIX, 21 },
    /* State 17. */
    /* State 18. */
    /* State 19. */
    { AS_CONSTANT, 23 },
    /* State 20. */
    /* State 21. */
    /* State 22. */
    /* State 23. */
    /* State 24. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 25. */
    /* State 26. */
    { AS_COMMA, 51 },
    /* State 27. */
    /* State 28. */
    { AS_EQ, 57 },
    { AS_GE, 58 },
    { AS_GT, 59 },
    { AS_LE, 60 },
    { AS_LT, 61 },
    { AS_NE, 62 },
    { AS_AND, 56 },
    { AS_MOD, 65 },
    { AS_OR, 54 },
    { AS_SHL, 63 },
    { AS_SHR, 64 },
    { AS_XOR, 55 },
    { AS_PTR, 68 },
    { AS_PLUS, 66 },
    { AS_MINUS, 67 },
    { AS_MULTIPLY, 52 },
    { AS_DIVIDE, 53 },
    { AS_DOT, 69 },
    /* State 29. */
    { AS_LEFT_BRACKET, 72 },
    { AS_TYPEDEF_NAME, 71 },
    { AS_IDENTIFIER, 70 },
    /* State 30. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 31. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 32. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 33. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 34. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 35. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 36. */
    { AS_COLON, 79 },
    { AS_LEFT_PAREN, 80 },
    /* State 37. */
    { AS_ASM_DATA_TYPE, 83 },
    { AS_C_DATA_TYPE, 82 },
    { AS_TYPEDEF_NAME, 81 },
    { AS_IDENTIFIER, 84 },
    /* State 38. */
    { AS_IDENTIFIER, 85 },
    /* State 39. */
    { AS_IDENTIFIER, 86 },
    /* State 40. */
    /* State 41. */
    /* State 42. */
    /* State 43. */
    { AS_IDENTIFIER, 87 },
    /* State 44. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 45. */
    { AS_PTR, 89 },
    /* State 46. */
    { AS_PTR, 90 },
    /* State 47. */
    { AS_PTR, 91 },
    { AS_IDENTIFIER, 92 },
    /* State 48. */
    { AS_PTR, 93 },
    { AS_IDENTIFIER, 94 },
    /* State 49. */
    { AS_RIGHT_BRACKET, 95 },
    /* State 50. */
    /* State 51. */
    /* State 52. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 53. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 54. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 55. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 56. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 57. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 58. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 59. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 60. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 61. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 62. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 63. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 64. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 65. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 66. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 67. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 68. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 69. */
    /* State 70. */
    { AS_DOT, 116 },
    /* State 71. */
    { AS_DOT, 117 },
    /* State 72. */
    /* State 73. */
    { AS_EQ, 57 },
    { AS_GE, 58 },
    { AS_GT, 59 },
    { AS_LE, 60 },
    { AS_LT, 61 },
    { AS_NE, 62 },
    { AS_AND, 56 },
    { AS_MOD, 65 },
    { AS_OR, 54 },
    { AS_SHL, 63 },
    { AS_SHR, 64 },
    { AS_XOR, 55 },
    { AS_PTR, 68 },
    { AS_PLUS, 66 },
    { AS_MINUS, 67 },
    { AS_MULTIPLY, 52 },
    { AS_DIVIDE, 53 },
    { AS_RIGHT_PAREN, 119 },
    { AS_DOT, 69 },
    /* State 74. */
    { AS_LEFT_BRACKET, 72 },
    /* State 75. */
    { AS_LEFT_BRACKET, 72 },
    /* State 76. */
    { AS_LEFT_BRACKET, 72 },
    /* State 77. */
    { AS_LEFT_BRACKET, 72 },
    /* State 78. */
    { AS_LEFT_BRACKET, 72 },
    /* State 79. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 80. */
    { AS_CONSTANT, 121 },
    /* State 81. */
    /* State 82. */
    /* State 83. */
    /* State 84. */
    /* State 85. */
    /* State 86. */
    /* State 87. */
    /* State 88. */
    { AS_LEFT_BRACKET, 72 },
    /* State 89. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 90. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 91. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 92. */
    /* State 93. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 94. */
    /* State 95. */
    /* State 96. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 97. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 98. */
    { AS_PTR, 68 },
    { AS_DOT, 69 },
    /* State 99. */
    { AS_PTR, 68 },
    { AS_DOT, 69 },
    /* State 100. */
    { AS_EQ, 57 },
    { AS_GE, 58 },
    { AS_GT, 59 },
    { AS_LE, 60 },
    { AS_LT, 61 },
    { AS_NE, 62 },
    { AS_AND, 56 },
    { AS_MOD, 65 },
    { AS_SHL, 63 },
    { AS_SHR, 64 },
    { AS_PTR, 68 },
    { AS_PLUS, 66 },
    { AS_MINUS, 67 },
    { AS_MULTIPLY, 52 },
    { AS_DIVIDE, 53 },
    { AS_DOT, 69 },
    /* State 101. */
    { AS_EQ, 57 },
    { AS_GE, 58 },
    { AS_GT, 59 },
    { AS_LE, 60 },
    { AS_LT, 61 },
    { AS_NE, 62 },
    { AS_AND, 56 },
    { AS_MOD, 65 },
    { AS_SHL, 63 },
    { AS_SHR, 64 },
    { AS_PTR, 68 },
    { AS_PLUS, 66 },
    { AS_MINUS, 67 },
    { AS_MULTIPLY, 52 },
    { AS_DIVIDE, 53 },
    { AS_DOT, 69 },
    /* State 102. */
    { AS_EQ, 57 },
    { AS_GE, 58 },
    { AS_GT, 59 },
    { AS_LE, 60 },
    { AS_LT, 61 },
    { AS_NE, 62 },
    { AS_MOD, 65 },
    { AS_SHL, 63 },
    { AS_SHR, 64 },
    { AS_PTR, 68 },
    { AS_PLUS, 66 },
    { AS_MINUS, 67 },
    { AS_MULTIPLY, 52 },
    { AS_DIVIDE, 53 },
    { AS_DOT, 69 },
    /* State 103. */
    { AS_MOD, 65 },
    { AS_SHL, 63 },
    { AS_SHR, 64 },
    { AS_PTR, 68 },
    { AS_PLUS, 66 },
    { AS_MINUS, 67 },
    { AS_MULTIPLY, 52 },
    { AS_DIVIDE, 53 },
    { AS_DOT, 69 },
    /* State 104. */
    { AS_MOD, 65 },
    { AS_SHL, 63 },
    { AS_SHR, 64 },
    { AS_PTR, 68 },
    { AS_PLUS, 66 },
    { AS_MINUS, 67 },
    { AS_MULTIPLY, 52 },
    { AS_DIVIDE, 53 },
    { AS_DOT, 69 },
    /* State 105. */
    { AS_MOD, 65 },
    { AS_SHL, 63 },
    { AS_SHR, 64 },
    { AS_PTR, 68 },
    { AS_PLUS, 66 },
    { AS_MINUS, 67 },
    { AS_MULTIPLY, 52 },
    { AS_DIVIDE, 53 },
    { AS_DOT, 69 },
    /* State 106. */
    { AS_MOD, 65 },
    { AS_SHL, 63 },
    { AS_SHR, 64 },
    { AS_PTR, 68 },
    { AS_PLUS, 66 },
    { AS_MINUS, 67 },
    { AS_MULTIPLY, 52 },
    { AS_DIVIDE, 53 },
    { AS_DOT, 69 },
    /* State 107. */
    { AS_MOD, 65 },
    { AS_SHL, 63 },
    { AS_SHR, 64 },
    { AS_PTR, 68 },
    { AS_PLUS, 66 },
    { AS_MINUS, 67 },
    { AS_MULTIPLY, 52 },
    { AS_DIVIDE, 53 },
    { AS_DOT, 69 },
    /* State 108. */
    { AS_MOD, 65 },
    { AS_SHL, 63 },
    { AS_SHR, 64 },
    { AS_PTR, 68 },
    { AS_PLUS, 66 },
    { AS_MINUS, 67 },
    { AS_MULTIPLY, 52 },
    { AS_DIVIDE, 53 },
    { AS_DOT, 69 },
    /* State 109. */
    { AS_PTR, 68 },
    { AS_DOT, 69 },
    /* State 110. */
    { AS_PTR, 68 },
    { AS_DOT, 69 },
    /* State 111. */
    { AS_PTR, 68 },
    { AS_DOT, 69 },
    /* State 112. */
    { AS_MOD, 65 },
    { AS_SHL, 63 },
    { AS_SHR, 64 },
    { AS_PTR, 68 },
    { AS_MULTIPLY, 52 },
    { AS_DIVIDE, 53 },
    { AS_DOT, 69 },
    /* State 113. */
    { AS_MOD, 65 },
    { AS_SHL, 63 },
    { AS_SHR, 64 },
    { AS_PTR, 68 },
    { AS_MULTIPLY, 52 },
    { AS_DIVIDE, 53 },
    { AS_DOT, 69 },
    /* State 114. */
    { AS_DOT, 69 },
    /* State 115. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 116. */
    /* State 117. */
    /* State 118. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 119. */
    /* State 120. */
    { AS_LEFT_BRACKET, 72 },
    /* State 121. */
    { AS_RIGHT_PAREN, 132 },
    /* State 122. */
    { AS_LEFT_BRACKET, 72 },
    /* State 123. */
    { AS_LEFT_BRACKET, 72 },
    /* State 124. */
    { AS_LEFT_BRACKET, 72 },
    /* State 125. */
    { AS_LEFT_BRACKET, 72 },
    /* State 126. */
    { AS_EQ, 57 },
    { AS_GE, 58 },
    { AS_GT, 59 },
    { AS_LE, 60 },
    { AS_LT, 61 },
    { AS_NE, 62 },
    { AS_AND, 56 },
    { AS_MOD, 65 },
    { AS_OR, 54 },
    { AS_SHL, 63 },
    { AS_SHR, 64 },
    { AS_XOR, 55 },
    { AS_PTR, 68 },
    { AS_PLUS, 66 },
    { AS_MINUS, 67 },
    { AS_MULTIPLY, 52 },
    { AS_DIVIDE, 53 },
    { AS_RIGHT_BRACKET, 133 },
    { AS_DOT, 69 },
    /* State 127. */
    /* State 128. */
    /* State 129. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 130. */
    { AS_NOT, 31 },
    { AS_HIGH, 34 },
    { AS_LENGTH, 39 },
    { AS_LOW, 35 },
    { AS_OFFSET, 44 },
    { AS_SEG, 43 },
    { AS_SIZE, 38 },
    { AS_TYPE, 37 },
    { AS_PLUS, 32 },
    { AS_MINUS, 33 },
    { AS_LEFT_BRACKET, 50 },
    { AS_LEFT_PAREN, 30 },
    { AS_REGISTER, 36 },
    { AS_ASM_DATA_TYPE, 45 },
    { AS_ASM_CODE_TYPE, 48 },
    { AS_C_DATA_TYPE, 47 },
    { AS_TYPEDEF_NAME, 46 },
    { AS_IDENTIFIER, 42 },
    { AS_CONSTANT, 40 },
    { AS_ENUM_MEMBER, 41 },
    /* State 131. */
    { AS_EQ, 57 },
    { AS_GE, 58 },
    { AS_GT, 59 },
    { AS_LE, 60 },
    { AS_LT, 61 },
    { AS_NE, 62 },
    { AS_AND, 56 },
    { AS_MOD, 65 },
    { AS_OR, 54 },
    { AS_SHL, 63 },
    { AS_SHR, 64 },
    { AS_XOR, 55 },
    { AS_PTR, 68 },
    { AS_PLUS, 66 },
    { AS_MINUS, 67 },
    { AS_MULTIPLY, 52 },
    { AS_DIVIDE, 53 },
    { AS_DOT, 69 },
    /* State 132. */
    /* State 133. */
    /* State 134. */
    /* State 135. */
};

ParserTransition asm_reductions[] = {
    { AS_END, -1 },
    { AS_NEWLINE, 6 },
    { AS_DOT, 6 },
    { AS_IDENTIFIER, 6 },
    { AS_PREFIX, 6 },
    { AS_OPCODE, 6 },
    { AS_DIRECTIVE, 6 },
    { AS_OPCODE, 18 },
    { AS_DIRECTIVE, 14 },
};

ParserState asm_states[136] = {
    { 6, 1, 0, 2, 0, 0 }, /* state 0 */
    { 0, 1, 2, 2, 0, 7 }, /* state 1 */
    { 1, 0, 4, 0, 7, 0 }, /* state 2 */
    { 0, 1, 4, 1, 7, 0 }, /* state 3 */
    { 0, 1, 5, 1, 7, 0 }, /* state 4 */
    { 8, 1, 6, 1, 7, 0 }, /* state 5 */
    { 7, 0, 7, 0, 7, 0 }, /* state 6 */
    { 2, 0, 7, 0, 7, 0 }, /* state 7 */
    { 3, 0, 7, 0, 7, 0 }, /* state 8 */
    { 4, 0, 7, 0, 7, 0 }, /* state 9 */
    { 10, 1, 7, 2, 7, 2 }, /* state 10 */
    { 0, 1, 9, 1, 9, 0 }, /* state 11 */
    { 5, 0, 10, 0, 9, 0 }, /* state 12 */
    { 0, 1, 10, 1, 9, 0 }, /* state 13 */
    { 0, 1, 11, 1, 9, 0 }, /* state 14 */
    { 15, 0, 12, 0, 9, 0 }, /* state 15 */
    { 19, 1, 12, 1, 9, 0 }, /* state 16 */
    { 20, 0, 13, 0, 9, 0 }, /* state 17 */
    { 9, 0, 13, 0, 9, 0 }, /* state 18 */
    { 16, 1, 13, 1, 9, 0 }, /* state 19 */
    { 12, 0, 14, 0, 9, 0 }, /* state 20 */
    { 21, 0, 14, 0, 9, 0 }, /* state 21 */
    { 11, 0, 14, 0, 9, 0 }, /* state 22 */
    { 17, 0, 14, 0, 9, 0 }, /* state 23 */
    { 22, 1, 14, 20, 9, 0 }, /* state 24 */
    { 13, 0, 34, 0, 9, 0 }, /* state 25 */
    { 23, 1, 34, 1, 9, 0 }, /* state 26 */
    { 24, 0, 35, 0, 9, 0 }, /* state 27 */
    { 27, 1, 35, 18, 9, 0 }, /* state 28 */
    { 51, 1, 53, 3, 9, 0 }, /* state 29 */
    { 0, 1, 56, 20, 9, 0 }, /* state 30 */
    { 0, 1, 76, 20, 9, 0 }, /* state 31 */
    { 0, 1, 96, 20, 9, 0 }, /* state 32 */
    { 0, 1, 116, 20, 9, 0 }, /* state 33 */
    { 0, 1, 136, 20, 9, 0 }, /* state 34 */
    { 0, 1, 156, 20, 9, 0 }, /* state 35 */
    { 70, 1, 176, 2, 9, 0 }, /* state 36 */
    { 0, 1, 178, 4, 9, 0 }, /* state 37 */
    { 0, 1, 182, 1, 9, 0 }, /* state 38 */
    { 0, 1, 183, 1, 9, 0 }, /* state 39 */
    { 65, 0, 184, 0, 9, 0 }, /* state 40 */
    { 66, 0, 184, 0, 9, 0 }, /* state 41 */
    { 67, 0, 184, 0, 9, 0 }, /* state 42 */
    { 0, 1, 184, 1, 9, 0 }, /* state 43 */
    { 0, 1, 185, 20, 9, 0 }, /* state 44 */
    { 0, 1, 205, 1, 9, 0 }, /* state 45 */
    { 0, 1, 206, 1, 9, 0 }, /* state 46 */
    { 0, 1, 207, 2, 9, 0 }, /* state 47 */
    { 0, 1, 209, 2, 9, 0 }, /* state 48 */
    { 0, 1, 211, 1, 9, 0 }, /* state 49 */
    { 79, 0, 212, 0, 9, 0 }, /* state 50 */
    { 25, 0, 212, 0, 9, 0 }, /* state 51 */
    { 0, 1, 212, 20, 9, 0 }, /* state 52 */
    { 0, 1, 232, 20, 9, 0 }, /* state 53 */
    { 0, 1, 252, 20, 9, 0 }, /* state 54 */
    { 0, 1, 272, 20, 9, 0 }, /* state 55 */
    { 0, 1, 292, 20, 9, 0 }, /* state 56 */
    { 0, 1, 312, 20, 9, 0 }, /* state 57 */
    { 0, 1, 332, 20, 9, 0 }, /* state 58 */
    { 0, 1, 352, 20, 9, 0 }, /* state 59 */
    { 0, 1, 372, 20, 9, 0 }, /* state 60 */
    { 0, 1, 392, 20, 9, 0 }, /* state 61 */
    { 0, 1, 412, 20, 9, 0 }, /* state 62 */
    { 0, 1, 432, 20, 9, 0 }, /* state 63 */
    { 0, 1, 452, 20, 9, 0 }, /* state 64 */
    { 0, 1, 472, 20, 9, 0 }, /* state 65 */
    { 0, 1, 492, 20, 9, 0 }, /* state 66 */
    { 0, 1, 512, 20, 9, 0 }, /* state 67 */
    { 0, 1, 532, 20, 9, 0 }, /* state 68 */
    { 49, 0, 552, 0, 9, 0 }, /* state 69 */
    { 0, 1, 552, 1, 9, 0 }, /* state 70 */
    { 0, 1, 553, 1, 9, 0 }, /* state 71 */
    { 81, 0, 554, 0, 9, 0 }, /* state 72 */
    { 0, 1, 554, 19, 9, 0 }, /* state 73 */
    { 53, 1, 573, 1, 9, 0 }, /* state 74 */
    { 54, 1, 574, 1, 9, 0 }, /* state 75 */
    { 55, 1, 575, 1, 9, 0 }, /* state 76 */
    { 56, 1, 576, 1, 9, 0 }, /* state 77 */
    { 57, 1, 577, 1, 9, 0 }, /* state 78 */
    { 0, 1, 578, 20, 9, 0 }, /* state 79 */
    { 0, 1, 598, 1, 9, 0 }, /* state 80 */
    { 59, 0, 599, 0, 9, 0 }, /* state 81 */
    { 60, 0, 599, 0, 9, 0 }, /* state 82 */
    { 61, 0, 599, 0, 9, 0 }, /* state 83 */
    { 62, 0, 599, 0, 9, 0 }, /* state 84 */
    { 63, 0, 599, 0, 9, 0 }, /* state 85 */
    { 64, 0, 599, 0, 9, 0 }, /* state 86 */
    { 68, 0, 599, 0, 9, 0 }, /* state 87 */
    { 69, 1, 599, 1, 9, 0 }, /* state 88 */
    { 0, 1, 600, 20, 9, 0 }, /* state 89 */
    { 0, 1, 620, 20, 9, 0 }, /* state 90 */
    { 0, 1, 640, 20, 9, 0 }, /* state 91 */
    { 76, 0, 660, 0, 9, 0 }, /* state 92 */
    { 0, 1, 660, 20, 9, 0 }, /* state 93 */
    { 77, 0, 680, 0, 9, 0 }, /* state 94 */
    { 78, 0, 680, 0, 9, 0 }, /* state 95 */
    { 0, 1, 680, 20, 9, 0 }, /* state 96 */
    { 0, 1, 700, 20, 9, 0 }, /* state 97 */
    { 28, 1, 720, 2, 9, 0 }, /* state 98 */
    { 29, 1, 722, 2, 9, 0 }, /* state 99 */
    { 30, 1, 724, 16, 9, 0 }, /* state 100 */
    { 31, 1, 740, 16, 9, 0 }, /* state 101 */
    { 32, 1, 756, 15, 9, 0 }, /* state 102 */
    { 33, 1, 771, 9, 9, 0 }, /* state 103 */
    { 34, 1, 780, 9, 9, 0 }, /* state 104 */
    { 35, 1, 789, 9, 9, 0 }, /* state 105 */
    { 36, 1, 798, 9, 9, 0 }, /* state 106 */
    { 37, 1, 807, 9, 9, 0 }, /* state 107 */
    { 38, 1, 816, 9, 9, 0 }, /* state 108 */
    { 39, 1, 825, 2, 9, 0 }, /* state 109 */
    { 40, 1, 827, 2, 9, 0 }, /* state 110 */
    { 41, 1, 829, 2, 9, 0 }, /* state 111 */
    { 42, 1, 831, 7, 9, 0 }, /* state 112 */
    { 43, 1, 838, 7, 9, 0 }, /* state 113 */
    { 44, 1, 845, 1, 9, 0 }, /* state 114 */
    { 0, 1, 846, 20, 9, 0 }, /* state 115 */
    { 45, 0, 866, 0, 9, 0 }, /* state 116 */
    { 47, 0, 866, 0, 9, 0 }, /* state 117 */
    { 0, 1, 866, 20, 9, 0 }, /* state 118 */
    { 52, 0, 886, 0, 9, 0 }, /* state 119 */
    { 58, 1, 886, 1, 9, 0 }, /* state 120 */
    { 0, 1, 887, 1, 9, 0 }, /* state 121 */
    { 72, 1, 888, 1, 9, 0 }, /* state 122 */
    { 73, 1, 889, 1, 9, 0 }, /* state 123 */
    { 74, 1, 890, 1, 9, 0 }, /* state 124 */
    { 75, 1, 891, 1, 9, 0 }, /* state 125 */
    { 0, 1, 892, 19, 9, 0 }, /* state 126 */
    { 26, 0, 911, 0, 9, 0 }, /* state 127 */
    { 50, 0, 911, 0, 9, 0 }, /* state 128 */
    { 0, 1, 911, 20, 9, 0 }, /* state 129 */
    { 0, 1, 931, 20, 9, 0 }, /* state 130 */
    { 82, 1, 951, 18, 9, 0 }, /* state 131 */
    { 71, 0, 969, 0, 9, 0 }, /* state 132 */
    { 80, 0, 969, 0, 9, 0 }, /* state 133 */
    { 46, 0, 969, 0, 9, 0 }, /* state 134 */
    { 48, 0, 969, 0, 9, 0 }, /* state 135 */
};

ParserTransition asm_goto_rows[] = {
    /* directive_value. */
    { 19, 22 },
    /* expression. */
    { 24, 28 },
    { 30, 73 },
    { 52, 98 },
    { 53, 99 },
    { 54, 100 },
    { 55, 101 },
    { 56, 102 },
    { 57, 103 },
    { 58, 104 },
    { 59, 105 },
    { 60, 106 },
    { 61, 107 },
    { 62, 108 },
    { 63, 109 },
    { 64, 110 },
    { 65, 111 },
    { 66, 112 },
    { 67, 113 },
    { 68, 114 },
    { 96, 126 },
    { 97, 28 },
    { 115, 128 },
    { 118, 131 },
    { 129, 134 },
    { 130, 135 },
    /* primary. */
    { 24, 29 },
    { 30, 29 },
    { 31, 74 },
    { 32, 75 },
    { 33, 76 },
    { 34, 77 },
    { 35, 78 },
    { 44, 88 },
    { 52, 29 },
    { 53, 29 },
    { 54, 29 },
    { 55, 29 },
    { 56, 29 },
    { 57, 29 },
    { 58, 29 },
    { 59, 29 },
    { 60, 29 },
    { 61, 29 },
    { 62, 29 },
    { 63, 29 },
    { 64, 29 },
    { 65, 29 },
    { 66, 29 },
    { 67, 29 },
    { 68, 29 },
    { 79, 120 },
    { 89, 122 },
    { 90, 123 },
    { 91, 124 },
    { 93, 125 },
    { 96, 29 },
    { 97, 29 },
    { 115, 29 },
    { 118, 29 },
    { 129, 29 },
    { 130, 29 },
    /* indexed_primary. */
    { 24, 49 },
    { 30, 49 },
    { 31, 49 },
    { 32, 49 },
    { 33, 49 },
    { 34, 49 },
    { 35, 49 },
    { 44, 49 },
    { 52, 49 },
    { 53, 49 },
    { 54, 49 },
    { 55, 49 },
    { 56, 49 },
    { 57, 49 },
    { 58, 49 },
    { 59, 49 },
    { 60, 49 },
    { 61, 49 },
    { 62, 49 },
    { 63, 49 },
    { 64, 49 },
    { 65, 49 },
    { 66, 49 },
    { 67, 49 },
    { 68, 49 },
    { 79, 49 },
    { 89, 49 },
    { 90, 49 },
    { 91, 49 },
    { 93, 49 },
    { 96, 49 },
    { 97, 49 },
    { 115, 49 },
    { 118, 49 },
    { 129, 49 },
    { 130, 49 },
    /* assembly_unit. */
    { 0, 1 },
    /* assembly_line. */
    { 0, 2 },
    { 1, 7 },
    /* statement. */
    { 0, 3 },
    { 1, 3 },
    /* optional_open_brace. */
    { 0, 5 },
    { 1, 5 },
    /* optional_label. */
    { 5, 10 },
    /* instruction. */
    { 10, 12 },
    /* optional_dot. */
    { 10, 13 },
    /* optional_prefixes. */
    { 10, 14 },
    /* begin_instruction. */
    { 20, 24 },
    /* optional_operands. */
    { 24, 25 },
    /* prefixes. */
    { 10, 16 },
    /* operand_list. */
    { 24, 26 },
    /* operand. */
    { 24, 27 },
    { 97, 127 },
    /* next_operand. */
    { 51, 97 },
    /* begin_member_name. */
    { 116, 129 },
    /* begin_member_type. */
    { 117, 130 },
    /* begin_member_expression. */
    { 69, 115 },
    /* begin_bracket. */
    { 50, 96 },
    /* continue_bracket. */
    { 72, 118 },
};

ParserGoto asm_gotos[24] = {
    { 0, 0, 0 }, /* unused */
    { 22, 0, 1 }, /* directive_value */
    { 28, 1, 25 }, /* expression */
    { 29, 26, 36 }, /* primary */
    { 49, 62, 36 }, /* indexed_primary */
    { 1, 98, 1 }, /* assembly_unit */
    { 2, 99, 2 }, /* assembly_line */
    { 3, 101, 2 }, /* statement */
    { 5, 103, 2 }, /* optional_open_brace */
    { 10, 105, 1 }, /* optional_label */
    { 12, 106, 1 }, /* instruction */
    { 13, 107, 1 }, /* optional_dot */
    { 14, 108, 1 }, /* optional_prefixes */
    { 24, 109, 1 }, /* begin_instruction */
    { 25, 110, 1 }, /* optional_operands */
    { 16, 111, 1 }, /* prefixes */
    { 26, 112, 1 }, /* operand_list */
    { 27, 113, 2 }, /* operand */
    { 97, 115, 1 }, /* next_operand */
    { 129, 116, 1 }, /* begin_member_name */
    { 130, 117, 1 }, /* begin_member_type */
    { 115, 118, 1 }, /* begin_member_expression */
    { 96, 119, 1 }, /* begin_bracket */
    { 118, 120, 1 }, /* continue_bracket */
};

ParserProduction asm_productions[83] = {
    { 0, 0 }, /* augmented accept is handled by the automaton */
    { 5, 1 }, /* 1: assembly_unit : assembly_line */
    { 5, 2 }, /* 2: assembly_unit : assembly_unit assembly_line */
    { 6, 2 }, /* 3: assembly_line : statement AS_NEWLINE */
    { 6, 2 }, /* 4: assembly_line : AS_ERROR AS_NEWLINE */
    { 7, 3 }, /* 5: statement : optional_open_brace optional_label instruction */
    { 8, 0 }, /* 6: optional_open_brace :  */
    { 8, 1 }, /* 7: optional_open_brace : AS_LEFT_BRACE */
    { 9, 0 }, /* 8: optional_label :  */
    { 9, 2 }, /* 9: optional_label : AS_IDENTIFIER AS_COLON */
    { 10, 0 }, /* 10: instruction :  */
    { 10, 3 }, /* 11: instruction : optional_dot AS_DIRECTIVE directive_value */
    { 13, 0 }, /* 12: begin_instruction :  */
    { 10, 4 }, /* 13: instruction : optional_prefixes AS_OPCODE begin_instruction optional_operands */
    { 11, 0 }, /* 14: optional_dot :  */
    { 11, 1 }, /* 15: optional_dot : AS_DOT */
    { 1, 0 }, /* 16: directive_value :  */
    { 1, 1 }, /* 17: directive_value : AS_CONSTANT */
    { 12, 0 }, /* 18: optional_prefixes :  */
    { 12, 1 }, /* 19: optional_prefixes : prefixes */
    { 15, 1 }, /* 20: prefixes : AS_PREFIX */
    { 15, 2 }, /* 21: prefixes : prefixes AS_PREFIX */
    { 14, 0 }, /* 22: optional_operands :  */
    { 14, 1 }, /* 23: optional_operands : operand_list */
    { 16, 1 }, /* 24: operand_list : operand */
    { 18, 0 }, /* 25: next_operand :  */
    { 16, 4 }, /* 26: operand_list : operand_list AS_COMMA next_operand operand */
    { 17, 1 }, /* 27: operand : expression */
    { 2, 3 }, /* 28: expression : expression AS_MULTIPLY expression */
    { 2, 3 }, /* 29: expression : expression AS_DIVIDE expression */
    { 2, 3 }, /* 30: expression : expression AS_OR expression */
    { 2, 3 }, /* 31: expression : expression AS_XOR expression */
    { 2, 3 }, /* 32: expression : expression AS_AND expression */
    { 2, 3 }, /* 33: expression : expression AS_EQ expression */
    { 2, 3 }, /* 34: expression : expression AS_GE expression */
    { 2, 3 }, /* 35: expression : expression AS_GT expression */
    { 2, 3 }, /* 36: expression : expression AS_LE expression */
    { 2, 3 }, /* 37: expression : expression AS_LT expression */
    { 2, 3 }, /* 38: expression : expression AS_NE expression */
    { 2, 3 }, /* 39: expression : expression AS_SHL expression */
    { 2, 3 }, /* 40: expression : expression AS_SHR expression */
    { 2, 3 }, /* 41: expression : expression AS_MOD expression */
    { 2, 3 }, /* 42: expression : expression AS_PLUS expression */
    { 2, 3 }, /* 43: expression : expression AS_MINUS expression */
    { 2, 3 }, /* 44: expression : expression AS_PTR expression */
    { 19, 0 }, /* 45: begin_member_name :  */
    { 2, 5 }, /* 46: expression : primary AS_IDENTIFIER AS_DOT begin_member_name expression */
    { 20, 0 }, /* 47: begin_member_type :  */
    { 2, 5 }, /* 48: expression : primary AS_TYPEDEF_NAME AS_DOT begin_member_type expression */
    { 21, 0 }, /* 49: begin_member_expression :  */
    { 2, 4 }, /* 50: expression : expression AS_DOT begin_member_expression expression */
    { 2, 1 }, /* 51: expression : primary */
    { 3, 3 }, /* 52: primary : AS_LEFT_PAREN expression AS_RIGHT_PAREN */
    { 3, 2 }, /* 53: primary : AS_NOT primary */
    { 3, 2 }, /* 54: primary : AS_PLUS primary */
    { 3, 2 }, /* 55: primary : AS_MINUS primary */
    { 3, 2 }, /* 56: primary : AS_HIGH primary */
    { 3, 2 }, /* 57: primary : AS_LOW primary */
    { 3, 3 }, /* 58: primary : AS_REGISTER AS_COLON primary */
    { 3, 2 }, /* 59: primary : AS_TYPE AS_TYPEDEF_NAME */
    { 3, 2 }, /* 60: primary : AS_TYPE AS_C_DATA_TYPE */
    { 3, 2 }, /* 61: primary : AS_TYPE AS_ASM_DATA_TYPE */
    { 3, 2 }, /* 62: primary : AS_TYPE AS_IDENTIFIER */
    { 3, 2 }, /* 63: primary : AS_SIZE AS_IDENTIFIER */
    { 3, 2 }, /* 64: primary : AS_LENGTH AS_IDENTIFIER */
    { 3, 1 }, /* 65: primary : AS_CONSTANT */
    { 3, 1 }, /* 66: primary : AS_ENUM_MEMBER */
    { 3, 1 }, /* 67: primary : AS_IDENTIFIER */
    { 3, 2 }, /* 68: primary : AS_SEG AS_IDENTIFIER */
    { 3, 2 }, /* 69: primary : AS_OFFSET primary */
    { 3, 1 }, /* 70: primary : AS_REGISTER */
    { 3, 4 }, /* 71: primary : AS_REGISTER AS_LEFT_PAREN AS_CONSTANT AS_RIGHT_PAREN */
    { 3, 3 }, /* 72: primary : AS_ASM_DATA_TYPE AS_PTR primary */
    { 3, 3 }, /* 73: primary : AS_TYPEDEF_NAME AS_PTR primary */
    { 3, 3 }, /* 74: primary : AS_C_DATA_TYPE AS_PTR primary */
    { 3, 3 }, /* 75: primary : AS_ASM_CODE_TYPE AS_PTR primary */
    { 3, 2 }, /* 76: primary : AS_C_DATA_TYPE AS_IDENTIFIER */
    { 3, 2 }, /* 77: primary : AS_ASM_CODE_TYPE AS_IDENTIFIER */
    { 3, 2 }, /* 78: primary : indexed_primary AS_RIGHT_BRACKET */
    { 22, 0 }, /* 79: begin_bracket :  */
    { 3, 4 }, /* 80: primary : AS_LEFT_BRACKET begin_bracket expression AS_RIGHT_BRACKET */
    { 23, 0 }, /* 81: continue_bracket :  */
    { 4, 4 }, /* 82: indexed_primary : primary AS_LEFT_BRACKET continue_bracket expression */
};
