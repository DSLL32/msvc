/* Target-derived C grammar: logical relations, not yacc compression.
 * Regenerate with scripts/parser-grammar.py. State, production and
 * unknown nonterminal identities remain numbered until recovered. */
#include "parser-grammar.h"
#include <recon.h>

const BYTE parser_token_classes[PARSER_TOKEN_KIND_COUNT] = {
    PARSER_CLASS_1, /* token kind 0 */
    PARSER_CLASS_1, /* token kind 1 */
    PARSER_CLASS_2, /* token kind 2 */
    PARSER_CLASS_3, /* token kind 3 */
    PARSER_CLASS_4, /* token kind 4 */
    PARSER_CLASS_5, /* token kind 5 */
    PARSER_CLASS_6, /* token kind 6 */
    PARSER_CLASS_7, /* token kind 7 */
    PARSER_CLASS_32, /* token kind 8 */
    PARSER_CLASS_32, /* token kind 9 */
    PARSER_CLASS_32, /* token kind 10 */
    PARSER_CLASS_32, /* token kind 11 */
    PARSER_CLASS_32, /* token kind 12 */
    PARSER_CLASS_33, /* token kind 13 */
    PARSER_CLASS_33, /* token kind 14 */
    PARSER_CLASS_15, /* token kind 15 */
    PARSER_CLASS_31, /* token kind 16 */
    PARSER_CLASS_31, /* token kind 17 */
    PARSER_CLASS_31, /* token kind 18 */
    PARSER_CLASS_31, /* token kind 19 */
    PARSER_CLASS_31, /* token kind 20 */
    PARSER_CLASS_31, /* token kind 21 */
    PARSER_CLASS_31, /* token kind 22 */
    PARSER_CLASS_23, /* token kind 23 */
    PARSER_CLASS_31, /* token kind 24 */
    PARSER_CLASS_31, /* token kind 25 */
    PARSER_CLASS_26, /* token kind 26 */
    PARSER_CLASS_27, /* token kind 27 */
    PARSER_CLASS_28, /* token kind 28 */
    PARSER_CLASS_29, /* token kind 29 */
    PARSER_CLASS_30, /* token kind 30 */
    PARSER_CLASS_31, /* token kind 31 */
    PARSER_CLASS_32, /* token kind 32 */
    PARSER_CLASS_33, /* token kind 33 */
    PARSER_CLASS_34, /* token kind 34 */
    PARSER_CLASS_35, /* token kind 35 */
    PARSER_CLASS_36, /* token kind 36 */
    PARSER_CALLING_CONVENTION, /* token kind 37 */
    PARSER_CALLING_CONVENTION, /* token kind 38 */
    PARSER_CLASS_40, /* token kind 39 */
    PARSER_CLASS_40, /* token kind 40 */
    PARSER_CLASS_41, /* token kind 41 */
    PARSER_CLASS_42, /* token kind 42 */
    PARSER_CLASS_43, /* token kind 43 */
    PARSER_BASED, /* token kind 44 */
    PARSER_CLASS_1, /* token kind 45 */
    PARSER_CLASS_96, /* token kind 46 */
    PARSER_CLASS_40, /* token kind 47 */
    PARSER_CLASS_40, /* token kind 48 */
    PARSER_CLASS_49, /* token kind 49 */
    PARSER_CLASS_50, /* token kind 50 */
    PARSER_CLASS_97, /* token kind 51 */
    PARSER_CLASS_52, /* token kind 52 */
    PARSER_CLASS_53, /* token kind 53 */
    PARSER_CLASS_54, /* token kind 54 */
    PARSER_CLASS_55, /* token kind 55 */
    PARSER_CLASS_115, /* token kind 56 */
    PARSER_CLASS_104, /* token kind 57 */
    PARSER_CLASS_97, /* token kind 58 */
    PARSER_CLASS_110, /* token kind 59 */
    PARSER_CLASS_60, /* token kind 60 */
    PARSER_CLASS_61, /* token kind 61 */
    PARSER_CLASS_124, /* token kind 62 */
    PARSER_CLASS_124, /* token kind 63 */
    PARSER_CLASS_115, /* token kind 64 */
    PARSER_CLASS_65, /* token kind 65 */
    PARSER_CLASS_97, /* token kind 66 */
    PARSER_CLASS_128, /* token kind 67 */
    PARSER_CLASS_124, /* token kind 68 */
    PARSER_CLASS_124, /* token kind 69 */
    PARSER_CLASS_70, /* token kind 70 */
    PARSER_CLASS_97, /* token kind 71 */
    PARSER_CLASS_104, /* token kind 72 */
    PARSER_CLASS_97, /* token kind 73 */
    PARSER_CLASS_74, /* token kind 74 */
    PARSER_CLASS_97, /* token kind 75 */
    PARSER_CLASS_110, /* token kind 76 */
    PARSER_CLASS_77, /* token kind 77 */
    PARSER_CLASS_97, /* token kind 78 */
    PARSER_CLASS_79, /* token kind 79 */
    PARSER_CLASS_131, /* token kind 80 */
    PARSER_CLASS_81, /* token kind 81 */
    PARSER_CLASS_97, /* token kind 82 */
    PARSER_CLASS_131, /* token kind 83 */
    PARSER_CLASS_115, /* token kind 84 */
    PARSER_CLASS_115, /* token kind 85 */
    PARSER_CLASS_86, /* token kind 86 */
    PARSER_CLASS_97, /* token kind 87 */
    PARSER_CLASS_128, /* token kind 88 */
    PARSER_CLASS_89, /* token kind 89 */
    PARSER_CLASS_90, /* token kind 90 */
    PARSER_CLASS_91, /* token kind 91 */
    PARSER_CLASS_97, /* token kind 92 */
    PARSER_CLASS_93, /* token kind 93 */
    PARSER_CLASS_94, /* token kind 94 */
    PARSER_CLASS_95, /* token kind 95 */
    PARSER_CLASS_96, /* token kind 96 */
    PARSER_CLASS_97, /* token kind 97 */
    PARSER_BREAK, /* token kind 98 */
    PARSER_CASE, /* token kind 99 */
    PARSER_CLASS_100, /* token kind 100 */
    PARSER_CLASS_101, /* token kind 101 */
    PARSER_CONTINUE, /* token kind 102 */
    PARSER_DEFAULT, /* token kind 103 */
    PARSER_CLASS_104, /* token kind 104 */
    PARSER_DO, /* token kind 105 */
    PARSER_CLASS_106, /* token kind 106 */
    PARSER_ELSE, /* token kind 107 */
    PARSER_CLASS_108, /* token kind 108 */
    PARSER_CLASS_0, /* token kind 109 */
    PARSER_CLASS_110, /* token kind 110 */
    PARSER_FOR, /* token kind 111 */
    PARSER_CLASS_112, /* token kind 112 */
    PARSER_GOTO, /* token kind 113 */
    PARSER_IF, /* token kind 114 */
    PARSER_CLASS_115, /* token kind 115 */
    PARSER_CLASS_116, /* token kind 116 */
    PARSER_CLASS_117, /* token kind 117 */
    PARSER_CLASS_118, /* token kind 118 */
    PARSER_CLASS_119, /* token kind 119 */
    PARSER_CALLING_CONVENTION, /* token kind 120 */
    PARSER_CLASS_121, /* token kind 121 */
    PARSER_CLASS_122, /* token kind 122 */
    PARSER_CLASS_123, /* token kind 123 */
    PARSER_CLASS_124, /* token kind 124 */
    PARSER_RETURN, /* token kind 125 */
    PARSER_CLASS_126, /* token kind 126 */
    PARSER_CLASS_127, /* token kind 127 */
    PARSER_CLASS_128, /* token kind 128 */
    PARSER_SIZEOF, /* token kind 129 */
    PARSER_CALLING_CONVENTION, /* token kind 130 */
    PARSER_CLASS_131, /* token kind 131 */
    PARSER_SWITCH, /* token kind 132 */
    PARSER_CLASS_133, /* token kind 133 */
    PARSER_CLASS_134, /* token kind 134 */
    PARSER_WHILE, /* token kind 135 */
    PARSER_CLASS_136, /* token kind 136 */
    PARSER_CLASS_145, /* token kind 137 */
    PARSER_CLASS_145, /* token kind 138 */
    PARSER_CLASS_145, /* token kind 139 */
    PARSER_CLASS_140, /* token kind 140 */
    PARSER_CLASS_141, /* token kind 141 */
    PARSER_CLASS_142, /* token kind 142 */
    PARSER_CLASS_143, /* token kind 143 */
    PARSER_CLASS_144, /* token kind 144 */
    PARSER_CLASS_145, /* token kind 145 */
    PARSER_CLASS_148, /* token kind 146 */
    PARSER_CLASS_148, /* token kind 147 */
    PARSER_CLASS_148, /* token kind 148 */
    PARSER_CLASS_149, /* token kind 149 */
    PARSER_CLASS_150, /* token kind 150 */
    PARSER_CLASS_151, /* token kind 151 */
    PARSER_CLASS_152, /* token kind 152 */
    PARSER_CLASS_153, /* token kind 153 */
    PARSER_CLASS_154, /* token kind 154 */
    PARSER_CLASS_155, /* token kind 155 */
    PARSER_CLASS_156, /* token kind 156 */
    PARSER_CLASS_157, /* token kind 157 */
    PARSER_CLASS_158, /* token kind 158 */
    PARSER_CLASS_159, /* token kind 159 */
    PARSER_CLASS_160, /* token kind 160 */
    PARSER_CLASS_161, /* token kind 161 */
    PARSER_CLASS_162, /* token kind 162 */
    PARSER_CLASS_163, /* token kind 163 */
    PARSER_CLASS_164, /* token kind 164 */
    PARSER_CLASS_165, /* token kind 165 */
    PARSER_CLASS_61, /* token kind 166 */
    PARSER_TRY, /* token kind 167 */
    PARSER_EXCEPT, /* token kind 168 */
    PARSER_FINALLY, /* token kind 169 */
    PARSER_CLASS_170, /* token kind 170 */
    PARSER_CLASS_171, /* token kind 171 */
    PARSER_CLASS_172, /* token kind 172 */
    PARSER_CLASS_173, /* token kind 173 */
    PARSER_CLASS_174, /* token kind 174 */
    PARSER_CLASS_175, /* token kind 175 */
    PARSER_CLASS_176, /* token kind 176 */
    PARSER_CLASS_177, /* token kind 177 */
    PARSER_CLASS_178, /* token kind 178 */
    PARSER_CLASS_179, /* token kind 179 */
    PARSER_ALIGNOF, /* token kind 180 */
    PARSER_LEAVE, /* token kind 181 */
    PARSER_CLASS_182, /* token kind 182 */
    PARSER_CLASS_183, /* token kind 183 */
    PARSER_CLASS_183, /* token kind 184 */
    PARSER_CLASS_183, /* token kind 185 */
    PARSER_CLASS_183, /* token kind 186 */
    PARSER_CLASS_187, /* token kind 187 */
    PARSER_CLASS_187, /* token kind 188 */
    PARSER_CLASS_189, /* token kind 189 */
    PARSER_CLASS_33, /* token kind 190 */
    PARSER_CALLING_CONVENTION, /* token kind 191 */
    PARSER_CLASS_192, /* token kind 192 */
    PARSER_CLASS_40, /* token kind 193 */
    PARSER_CLASS_193, /* token kind 194 */
    PARSER_CLASS_195, /* token kind 195 */
    PARSER_CLASS_196, /* token kind 196 */
    PARSER_CLASS_197, /* token kind 197 */
    PARSER_CLASS_198, /* token kind 198 */
    PARSER_CLASS_199, /* token kind 199 */
    PARSER_CLASS_200, /* token kind 200 */
    PARSER_CLASS_201, /* token kind 201 */
    PARSER_CLASS_202, /* token kind 202 */
    PARSER_CLASS_203, /* token kind 203 */
    PARSER_CLASS_204, /* token kind 204 */
    PARSER_CLASS_205, /* token kind 205 */
    PARSER_CLASS_100, /* token kind 206 */
    PARSER_CLASS_207, /* token kind 207 */
    PARSER_CLASS_208, /* token kind 208 */
    PARSER_CLASS_209, /* token kind 209 */
    PARSER_CLASS_210, /* token kind 210 */
    PARSER_CLASS_211, /* token kind 211 */
    PARSER_CLASS_31, /* token kind 212 */
    PARSER_CLASS_31, /* token kind 213 */
    PARSER_CLASS_31, /* token kind 214 */
    PARSER_CLASS_31, /* token kind 215 */
    PARSER_CLASS_31, /* token kind 216 */
    PARSER_CLASS_31, /* token kind 217 */
    PARSER_CLASS_218, /* token kind 218 */
    PARSER_CLASS_219, /* token kind 219 */
    PARSER_CLASS_100, /* token kind 220 */
    PARSER_CLASS_33, /* token kind 221 */
    PARSER_CALLING_CONVENTION, /* token kind 222 */
    PARSER_CLASS_6, /* token kind 223 */
    PARSER_CLASS_224, /* token kind 224 */
    PARSER_CLASS_225, /* token kind 225 */
    PARSER_CLASS_226, /* token kind 226 */
    PARSER_CLASS_100, /* token kind 227 */
    PARSER_CLASS_100, /* token kind 228 */
    PARSER_CLASS_33, /* token kind 229 */
    PARSER_CLASS_33, /* token kind 230 */
    PARSER_CLASS_31, /* token kind 231 */
    PARSER_CLASS_232, /* token kind 232 */
    PARSER_CLASS_233, /* token kind 233 */
    PARSER_CLASS_234, /* token kind 234 */
    PARSER_CLASS_235, /* token kind 235 */
    PARSER_CLASS_236, /* token kind 236 */
    PARSER_CLASS_40, /* token kind 237 */
    PARSER_CLASS_131, /* token kind 238 */
    PARSER_CLASS_239, /* token kind 239 */
    PARSER_CLASS_240, /* token kind 240 */
    PARSER_CLASS_241, /* token kind 241 */
    PARSER_CLASS_242, /* token kind 242 */
    PARSER_CLASS_243, /* token kind 243 */
    PARSER_CLASS_244, /* token kind 244 */
};

const ParserTransition parser_shifts[] = {
    /* State 0. */
    { PARSER_CLASS_2, 6 },
    { PARSER_CLASS_3, 8 },
    { PARSER_CLASS_4, 7 },
    { PARSER_CLASS_5, 3 },
    { PARSER_CLASS_34, 20 },
    { PARSER_CLASS_35, 22 },
    { PARSER_CLASS_36, 16 },
    { PARSER_CLASS_117, 14 },
    { PARSER_CLASS_127, 11 },
    { PARSER_ERROR, 13 },
    /* State 2. */
    { PARSER_CLASS_2, 6 },
    { PARSER_CLASS_3, 8 },
    { PARSER_CLASS_4, 7 },
    { PARSER_CLASS_5, 23 },
    { PARSER_CLASS_34, 20 },
    { PARSER_CLASS_35, 22 },
    { PARSER_CLASS_36, 16 },
    { PARSER_CLASS_117, 14 },
    { PARSER_CLASS_127, 11 },
    { PARSER_ERROR, 13 },
    /* State 10. */
    { PARSER_CLASS_6, 30 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_52, 34 },
    { PARSER_CLASS_74, 32 },
    { PARSER_CLASS_118, 36 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 12. */
    { PARSER_CLASS_127, 39 },
    /* State 13. */
    { PARSER_CLASS_117, 41 },
    { PARSER_CLASS_127, 40 },
    /* State 21. */
    { PARSER_CLASS_6, 46 },
    { PARSER_CLASS_123, 44 },
    /* State 22. */
    { PARSER_CLASS_117, 47 },
    /* State 25. */
    { PARSER_CLASS_1, 51 },
    { PARSER_CLASS_34, 20 },
    { PARSER_CLASS_35, 22 },
    { PARSER_CLASS_36, 16 },
    /* State 26. */
    { PARSER_CLASS_55, 53 },
    { PARSER_CLASS_127, 52 },
    /* State 27. */
    { PARSER_CLASS_52, 54 },
    { PARSER_CLASS_65, 55 },
    { PARSER_CLASS_118, 57 },
    /* State 28. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_41, 58 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 31. */
    { PARSER_CLASS_6, 79 },
    { PARSER_CLASS_40, 82 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 80 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 32. */
    { PARSER_CLASS_40, 85 },
    /* State 36. */
    { PARSER_CLASS_6, 30 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 32 },
    { PARSER_CLASS_118, 36 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 38. */
    { PARSER_CLASS_118, 87 },
    /* State 42. */
    { PARSER_CLASS_117, 89 },
    /* State 43. */
    { PARSER_CLASS_55, 91 },
    /* State 46. */
    { PARSER_CLASS_52, 92 },
    /* State 48. */
    { PARSER_CLASS_117, 94 },
    /* State 50. */
    { PARSER_CLASS_6, 30 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 32 },
    { PARSER_CLASS_118, 36 },
    { PARSER_CALLING_CONVENTION, 37 },
    { PARSER_CLASS_127, 96 },
    /* State 53. */
    { PARSER_CLASS_6, 30 },
    { PARSER_CLASS_40, 85 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_52, 34 },
    { PARSER_CLASS_74, 32 },
    { PARSER_CLASS_118, 36 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 55. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 59. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 60. */
    { PARSER_CLASS_117, 123 },
    /* State 61. */
    { PARSER_CLASS_65, 125 },
    { PARSER_CLASS_94, 127 },
    { PARSER_CLASS_115, 124 },
    { PARSER_CLASS_118, 128 },
    { PARSER_CLASS_131, 126 },
    /* State 62. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 63. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 64. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 65. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 66. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 67. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 68. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 69. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 70. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 71. */
    { PARSER_CLASS_118, 141 },
    /* State 72. */
    { PARSER_CLASS_118, 141 },
    /* State 73. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 74. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_CLASS_126, 144 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 75. */
    { PARSER_CLASS_118, 147 },
    /* State 77. */
    { PARSER_CLASS_7, 148 },
    /* State 80. */
    { PARSER_CLASS_40, 85 },
    /* State 83. */
    { PARSER_CLASS_6, 30 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 32 },
    { PARSER_CLASS_118, 36 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 84. */
    { PARSER_CLASS_40, 151 },
    /* State 86. */
    { PARSER_CLASS_65, 55 },
    { PARSER_CLASS_118, 57 },
    { PARSER_CLASS_126, 152 },
    /* State 87. */
    { PARSER_CLASS_6, 154 },
    { PARSER_CLASS_36, 155 },
    { PARSER_ERROR, 156 },
    /* State 88. */
    { PARSER_CLASS_123, 157 },
    /* State 89. */
    { PARSER_CLASS_34, 20 },
    { PARSER_CLASS_35, 22 },
    { PARSER_CLASS_36, 16 },
    /* State 90. */
    { PARSER_CLASS_123, 161 },
    /* State 91. */
    { PARSER_CLASS_6, 46 },
    /* State 92. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 95. */
    { PARSER_CLASS_6, 167 },
    { PARSER_CLASS_55, 166 },
    { PARSER_CLASS_127, 165 },
    /* State 98. */
    { PARSER_CLASS_65, 55 },
    { PARSER_CLASS_118, 57 },
    /* State 100. */
    { PARSER_CLASS_6, 30 },
    { PARSER_CLASS_40, 151 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_52, 34 },
    { PARSER_CLASS_74, 32 },
    { PARSER_CLASS_118, 36 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 101. */
    { PARSER_CLASS_52, 54 },
    { PARSER_CLASS_65, 55 },
    { PARSER_CLASS_118, 57 },
    /* State 102. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 103. */
    { PARSER_CLASS_122, 169 },
    /* State 105. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 106. */
    { PARSER_CLASS_6, 172 },
    { PARSER_CLASS_34, 20 },
    { PARSER_CLASS_35, 22 },
    { PARSER_CLASS_36, 16 },
    { PARSER_ERROR, 175 },
    /* State 107. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 108. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 109. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 110. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 111. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 112. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 113. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 114. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 115. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 116. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 117. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 118. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 119. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 120. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 121. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 122. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 123. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_117, 196 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 125. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 127. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 129. */
    { PARSER_CLASS_65, 125 },
    { PARSER_CLASS_94, 127 },
    { PARSER_CLASS_115, 124 },
    { PARSER_CLASS_118, 128 },
    { PARSER_CLASS_131, 126 },
    /* State 130. */
    { PARSER_CLASS_65, 125 },
    { PARSER_CLASS_94, 127 },
    { PARSER_CLASS_115, 124 },
    { PARSER_CLASS_118, 128 },
    { PARSER_CLASS_131, 126 },
    /* State 131. */
    { PARSER_CLASS_65, 125 },
    { PARSER_CLASS_94, 127 },
    { PARSER_CLASS_115, 124 },
    { PARSER_CLASS_118, 128 },
    { PARSER_CLASS_131, 126 },
    /* State 132. */
    { PARSER_CLASS_65, 125 },
    { PARSER_CLASS_94, 127 },
    { PARSER_CLASS_115, 124 },
    { PARSER_CLASS_118, 128 },
    { PARSER_CLASS_131, 126 },
    /* State 133. */
    { PARSER_CLASS_65, 125 },
    { PARSER_CLASS_115, 124 },
    { PARSER_CLASS_118, 128 },
    { PARSER_CLASS_131, 126 },
    /* State 134. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_55, 108 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_126, 200 },
    { PARSER_CLASS_128, 112 },
    /* State 135. */
    { PARSER_CLASS_34, 20 },
    { PARSER_CLASS_35, 22 },
    { PARSER_CLASS_36, 16 },
    /* State 136. */
    { PARSER_CLASS_65, 125 },
    { PARSER_CLASS_94, 127 },
    { PARSER_CLASS_115, 124 },
    { PARSER_CLASS_118, 128 },
    { PARSER_CLASS_131, 126 },
    /* State 137. */
    { PARSER_CLASS_65, 125 },
    { PARSER_CLASS_94, 127 },
    { PARSER_CLASS_115, 124 },
    { PARSER_CLASS_118, 128 },
    { PARSER_CLASS_131, 126 },
    /* State 138. */
    { PARSER_CLASS_65, 125 },
    { PARSER_CLASS_94, 127 },
    { PARSER_CLASS_115, 124 },
    { PARSER_CLASS_118, 128 },
    { PARSER_CLASS_131, 126 },
    /* State 139. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 143. */
    { PARSER_CLASS_65, 125 },
    { PARSER_CLASS_94, 127 },
    { PARSER_CLASS_115, 124 },
    { PARSER_CLASS_118, 128 },
    { PARSER_CLASS_131, 126 },
    /* State 145. */
    { PARSER_CLASS_55, 204 },
    { PARSER_CLASS_126, 203 },
    /* State 146. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 149. */
    { PARSER_CLASS_6, 30 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 32 },
    { PARSER_CLASS_118, 36 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 150. */
    { PARSER_CLASS_65, 55 },
    { PARSER_CLASS_118, 57 },
    /* State 153. */
    { PARSER_CLASS_126, 206 },
    /* State 158. */
    { PARSER_CLASS_34, 20 },
    { PARSER_CLASS_35, 22 },
    { PARSER_CLASS_36, 16 },
    { PARSER_CLASS_123, 207 },
    /* State 160. */
    { PARSER_CLASS_6, 30 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 32 },
    { PARSER_CLASS_118, 36 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 166. */
    { PARSER_CLASS_6, 30 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 32 },
    { PARSER_CLASS_118, 36 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 170. */
    { PARSER_CLASS_55, 217 },
    /* State 176. */
    { PARSER_CLASS_6, 30 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 221 },
    { PARSER_CLASS_118, 224 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 177. */
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_128, 112 },
    /* State 178. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 180. */
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_104, 109 },
    /* State 181. */
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_104, 109 },
    /* State 182. */
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_104, 109 },
    /* State 184. */
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 185. */
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 186. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 187. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 188. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 189. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 190. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 191. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 192. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_54, 227 },
    { PARSER_CLASS_55, 108 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 193. */
    { PARSER_CLASS_55, 229 },
    /* State 195. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 197. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_55, 108 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_122, 231 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 198. */
    { PARSER_CLASS_6, 232 },
    /* State 199. */
    { PARSER_CLASS_65, 125 },
    { PARSER_CLASS_115, 124 },
    { PARSER_CLASS_118, 128 },
    { PARSER_CLASS_131, 126 },
    /* State 201. */
    { PARSER_CLASS_126, 233 },
    /* State 202. */
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 236 },
    { PARSER_CLASS_118, 237 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 204. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 205. */
    { PARSER_CLASS_65, 55 },
    { PARSER_CLASS_118, 57 },
    /* State 209. */
    { PARSER_CLASS_127, 239 },
    /* State 210. */
    { PARSER_CLASS_55, 240 },
    { PARSER_ERROR, 241 },
    /* State 212. */
    { PARSER_CLASS_65, 55 },
    { PARSER_CLASS_118, 57 },
    /* State 213. */
    { PARSER_CLASS_54, 242 },
    /* State 214. */
    { PARSER_CLASS_34, 20 },
    { PARSER_CLASS_35, 22 },
    { PARSER_CLASS_36, 16 },
    { PARSER_ERROR, 246 },
    /* State 216. */
    { PARSER_CLASS_126, 247 },
    /* State 217. */
    { PARSER_CLASS_6, 172 },
    { PARSER_CLASS_34, 20 },
    { PARSER_CLASS_35, 22 },
    { PARSER_CLASS_36, 16 },
    { PARSER_CLASS_106, 249 },
    { PARSER_ERROR, 175 },
    /* State 218. */
    { PARSER_CLASS_65, 55 },
    { PARSER_CLASS_118, 57 },
    /* State 219. */
    { PARSER_CLASS_65, 250 },
    /* State 220. */
    { PARSER_CLASS_6, 79 },
    { PARSER_CLASS_40, 82 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 251 },
    { PARSER_CLASS_118, 253 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 221. */
    { PARSER_CLASS_40, 85 },
    /* State 224. */
    { PARSER_CLASS_6, 30 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 221 },
    { PARSER_CLASS_118, 224 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 226. */
    { PARSER_CLASS_118, 253 },
    /* State 227. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 228. */
    { PARSER_CLASS_123, 260 },
    /* State 229. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_117, 196 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 230. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_117, 196 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 234. */
    { PARSER_CLASS_65, 250 },
    /* State 235. */
    { PARSER_CLASS_40, 82 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 263 },
    { PARSER_CLASS_118, 253 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 236. */
    { PARSER_CLASS_40, 85 },
    /* State 237. */
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 236 },
    { PARSER_CLASS_118, 237 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 238. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 240. */
    { PARSER_CLASS_6, 30 },
    { PARSER_CLASS_40, 85 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 32 },
    { PARSER_CLASS_118, 36 },
    { PARSER_CALLING_CONVENTION, 37 },
    { PARSER_ERROR, 267 },
    /* State 242. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 245. */
    { PARSER_CLASS_6, 30 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_52, 34 },
    { PARSER_CLASS_74, 32 },
    { PARSER_CLASS_118, 36 },
    { PARSER_CALLING_CONVENTION, 37 },
    { PARSER_CLASS_127, 273 },
    /* State 246. */
    { PARSER_CLASS_127, 274 },
    /* State 250. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 251. */
    { PARSER_CLASS_40, 85 },
    /* State 254. */
    { PARSER_CLASS_6, 30 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 221 },
    { PARSER_CLASS_118, 224 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 255. */
    { PARSER_CLASS_34, 20 },
    { PARSER_CLASS_35, 22 },
    { PARSER_CLASS_36, 16 },
    { PARSER_ERROR, 282 },
    /* State 256. */
    { PARSER_CLASS_126, 283 },
    /* State 257. */
    { PARSER_CLASS_65, 250 },
    /* State 259. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 262. */
    { PARSER_CLASS_55, 229 },
    /* State 263. */
    { PARSER_CLASS_40, 85 },
    /* State 264. */
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 236 },
    { PARSER_CLASS_118, 237 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 266. */
    { PARSER_CLASS_6, 30 },
    { PARSER_CLASS_40, 151 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 32 },
    { PARSER_CLASS_118, 36 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 269. */
    { PARSER_CLASS_123, 287 },
    /* State 271. */
    { PARSER_CLASS_6, 309 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_42, 303 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_BREAK, 299 },
    { PARSER_CASE, 310 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CONTINUE, 300 },
    { PARSER_DEFAULT, 311 },
    { PARSER_DO, 296 },
    { PARSER_ELSE, 307 },
    { PARSER_FOR, 295 },
    { PARSER_GOTO, 302 },
    { PARSER_IF, 312 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_117, 94 },
    { PARSER_CLASS_118, 67 },
    { PARSER_RETURN, 301 },
    { PARSER_CLASS_127, 306 },
    { PARSER_SIZEOF, 70 },
    { PARSER_SWITCH, 297 },
    { PARSER_WHILE, 294 },
    { PARSER_TRY, 313 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_LEAVE, 298 },
    { PARSER_CLASS_218, 72 },
    { PARSER_ERROR, 308 },
    /* State 272. */
    { PARSER_CLASS_55, 53 },
    { PARSER_CLASS_127, 314 },
    /* State 275. */
    { PARSER_CLASS_122, 315 },
    /* State 276. */
    { PARSER_CLASS_6, 30 },
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 221 },
    { PARSER_CLASS_118, 224 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 277. */
    { PARSER_CLASS_65, 250 },
    /* State 278. */
    { PARSER_CLASS_55, 318 },
    /* State 284. */
    { PARSER_CLASS_123, 319 },
    /* State 285. */
    { PARSER_BASED, 38 },
    { PARSER_CLASS_74, 236 },
    { PARSER_CLASS_118, 237 },
    { PARSER_CALLING_CONVENTION, 37 },
    /* State 290. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_55, 108 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_127, 320 },
    { PARSER_CLASS_128, 112 },
    /* State 294. */
    { PARSER_CLASS_118, 323 },
    /* State 295. */
    { PARSER_CLASS_118, 324 },
    /* State 297. */
    { PARSER_CLASS_118, 326 },
    /* State 298. */
    { PARSER_CLASS_127, 327 },
    /* State 299. */
    { PARSER_CLASS_127, 328 },
    /* State 300. */
    { PARSER_CLASS_127, 329 },
    /* State 301. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_CLASS_127, 330 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 302. */
    { PARSER_CLASS_6, 332 },
    /* State 304. */
    { PARSER_CLASS_117, 94 },
    /* State 305. */
    { PARSER_EXCEPT, 335 },
    { PARSER_FINALLY, 334 },
    /* State 308. */
    { PARSER_CLASS_127, 336 },
    /* State 309. */
    { PARSER_CLASS_54, 337 },
    { PARSER_CLASS_118, 147 },
    /* State 310. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 311. */
    { PARSER_CLASS_54, 339 },
    /* State 312. */
    { PARSER_CLASS_118, 340 },
    /* State 316. */
    { PARSER_CLASS_65, 250 },
    /* State 317. */
    { PARSER_CLASS_126, 342 },
    /* State 318. */
    { PARSER_CLASS_34, 20 },
    { PARSER_CLASS_35, 22 },
    { PARSER_CLASS_36, 16 },
    { PARSER_CLASS_106, 249 },
    { PARSER_ERROR, 282 },
    /* State 321. */
    { PARSER_ELSE, 344 },
    /* State 323. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 324. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 326. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 331. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_55, 108 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_127, 350 },
    { PARSER_CLASS_128, 112 },
    /* State 332. */
    { PARSER_CLASS_127, 351 },
    /* State 338. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_54, 354 },
    { PARSER_CLASS_55, 108 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 340. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 341. */
    { PARSER_CLASS_117, 94 },
    /* State 345. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_55, 108 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_126, 357 },
    { PARSER_CLASS_128, 112 },
    /* State 346. */
    { PARSER_CLASS_127, 358 },
    /* State 347. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_55, 108 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_128, 112 },
    /* State 348. */
    { PARSER_WHILE, 359 },
    /* State 349. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_55, 108 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_126, 360 },
    { PARSER_CLASS_128, 112 },
    /* State 352. */
    { PARSER_CLASS_117, 94 },
    /* State 353. */
    { PARSER_CLASS_118, 362 },
    /* State 355. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_55, 108 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_126, 363 },
    { PARSER_CLASS_128, 112 },
    /* State 358. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 359. */
    { PARSER_CLASS_118, 366 },
    /* State 362. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 365. */
    { PARSER_CLASS_127, 370 },
    /* State 366. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 368. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_55, 108 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_126, 373 },
    { PARSER_CLASS_128, 112 },
    /* State 370. */
    { PARSER_CLASS_6, 75 },
    { PARSER_CLASS_7, 78 },
    { PARSER_CLASS_49, 69 },
    { PARSER_CLASS_60, 65 },
    { PARSER_CLASS_70, 63 },
    { PARSER_CLASS_74, 68 },
    { PARSER_CLASS_81, 62 },
    { PARSER_CLASS_89, 64 },
    { PARSER_CLASS_101, 76 },
    { PARSER_CLASS_115, 66 },
    { PARSER_CLASS_118, 67 },
    { PARSER_SIZEOF, 70 },
    { PARSER_ALIGNOF, 71 },
    { PARSER_CLASS_218, 72 },
    /* State 371. */
    { PARSER_CLASS_49, 115 },
    { PARSER_CLASS_50, 118 },
    { PARSER_CLASS_52, 121 },
    { PARSER_CLASS_55, 108 },
    { PARSER_CLASS_70, 111 },
    { PARSER_CLASS_74, 113 },
    { PARSER_CLASS_77, 116 },
    { PARSER_CLASS_79, 119 },
    { PARSER_CLASS_81, 110 },
    { PARSER_CLASS_86, 122 },
    { PARSER_CLASS_91, 117 },
    { PARSER_CLASS_97, 120 },
    { PARSER_CLASS_104, 109 },
    { PARSER_CLASS_110, 114 },
    { PARSER_CLASS_124, 107 },
    { PARSER_CLASS_126, 375 },
    { PARSER_CLASS_128, 112 },
    /* State 375. */
    { PARSER_CLASS_127, 377 },
    /* State 376. */
    { PARSER_CLASS_126, 378 },
};

const ParserTransition parser_exceptions[] = {
    { PARSER_CLASS_0, -1 },
    { PARSER_CLASS_55, 62 },
    { PARSER_CLASS_127, 62 },
    { PARSER_CLASS_127, 48 },
    { PARSER_CLASS_54, 58 },
    { PARSER_CLASS_65, 114 },
    { PARSER_CLASS_65, 114 },
    { PARSER_CLASS_123, 128 },
};

const ParserState parser_states[PARSER_STATE_COUNT] = {
    { 29, 1, 0, 10, 0, 0 }, /* state 0 */
    { 0, 0, 10, 0, 0, 1 }, /* state 1 */
    { 29, 1, 10, 10, 1, 0 }, /* state 2 */
    { 2, 0, 20, 0, 1, 0 }, /* state 3 */
    { 4, 0, 20, 0, 1, 0 }, /* state 4 */
    { 5, 0, 20, 0, 1, 0 }, /* state 5 */
    { 6, 0, 20, 0, 1, 0 }, /* state 6 */
    { 7, 0, 20, 0, 1, 0 }, /* state 7 */
    { 8, 0, 20, 0, 1, 0 }, /* state 8 */
    { 20, 0, 20, 0, 1, 0 }, /* state 9 */
    { 0, 1, 20, 6, 1, 0 }, /* state 10 */
    { 12, 0, 26, 0, 1, 0 }, /* state 11 */
    { 30, 1, 26, 1, 1, 0 }, /* state 12 */
    { 0, 1, 27, 2, 1, 0 }, /* state 13 */
    { 17, 0, 29, 0, 1, 0 }, /* state 14 */
    { 31, 0, 29, 0, 1, 0 }, /* state 15 */
    { 32, 0, 29, 0, 1, 0 }, /* state 16 */
    { 33, 0, 29, 0, 1, 0 }, /* state 17 */
    { 34, 0, 29, 0, 1, 0 }, /* state 18 */
    { 35, 0, 29, 0, 1, 0 }, /* state 19 */
    { 36, 0, 29, 0, 1, 0 }, /* state 20 */
    { 0, 1, 29, 2, 1, 0 }, /* state 21 */
    { 0, 1, 31, 1, 1, 0 }, /* state 22 */
    { 1, 0, 32, 0, 1, 0 }, /* state 23 */
    { 3, 0, 32, 0, 1, 0 }, /* state 24 */
    { 9, 1, 32, 4, 1, 0 }, /* state 25 */
    { 0, 1, 36, 2, 1, 0 }, /* state 26 */
    { 18, 1, 38, 3, 1, 2 }, /* state 27 */
    { 64, 1, 41, 15, 3, 0 }, /* state 28 */
    { 59, 0, 56, 0, 3, 0 }, /* state 29 */
    { 73, 0, 56, 0, 3, 0 }, /* state 30 */
    { 0, 1, 56, 5, 3, 0 }, /* state 31 */
    { 105, 1, 61, 1, 3, 0 }, /* state 32 */
    { 79, 0, 62, 0, 3, 0 }, /* state 33 */
    { 67, 0, 62, 0, 3, 0 }, /* state 34 */
    { 100, 0, 62, 0, 3, 0 }, /* state 35 */
    { 0, 1, 62, 5, 3, 0 }, /* state 36 */
    { 103, 0, 67, 0, 3, 0 }, /* state 37 */
    { 0, 1, 67, 1, 3, 0 }, /* state 38 */
    { 13, 0, 68, 0, 3, 0 }, /* state 39 */
    { 14, 0, 68, 0, 3, 0 }, /* state 40 */
    { 15, 0, 68, 0, 3, 0 }, /* state 41 */
    { 0, 1, 68, 1, 3, 0 }, /* state 42 */
    { 229, 1, 69, 1, 3, 0 }, /* state 43 */
    { 39, 0, 70, 0, 3, 0 }, /* state 44 */
    { 41, 0, 70, 0, 3, 0 }, /* state 45 */
    { 43, 1, 70, 1, 3, 0 }, /* state 46 */
    { 40, 0, 71, 0, 3, 0 }, /* state 47 */
    { 0, 1, 71, 1, 3, 0 }, /* state 48 */
    { 21, 0, 72, 0, 3, 0 }, /* state 49 */
    { 0, 1, 72, 6, 3, 0 }, /* state 50 */
    { 24, 0, 78, 0, 3, 0 }, /* state 51 */
    { 11, 0, 78, 0, 3, 0 }, /* state 52 */
    { 0, 1, 78, 7, 3, 0 }, /* state 53 */
    { 66, 0, 85, 0, 3, 0 }, /* state 54 */
    { 175, 1, 85, 14, 3, 0 }, /* state 55 */
    { 78, 0, 99, 0, 3, 0 }, /* state 56 */
    { 81, 0, 99, 0, 3, 0 }, /* state 57 */
    { 19, 0, 99, 0, 3, 0 }, /* state 58 */
    { 63, 1, 99, 15, 3, 0 }, /* state 59 */
    { 0, 1, 114, 1, 3, 0 }, /* state 60 */
    { 196, 1, 115, 5, 3, 0 }, /* state 61 */
    { 0, 1, 120, 14, 3, 0 }, /* state 62 */
    { 0, 1, 134, 14, 3, 0 }, /* state 63 */
    { 0, 1, 148, 14, 3, 0 }, /* state 64 */
    { 0, 1, 162, 14, 3, 0 }, /* state 65 */
    { 0, 1, 176, 14, 3, 0 }, /* state 66 */
    { 222, 1, 190, 14, 3, 0 }, /* state 67 */
    { 0, 1, 204, 14, 3, 0 }, /* state 68 */
    { 0, 1, 218, 14, 3, 0 }, /* state 69 */
    { 0, 1, 232, 14, 3, 0 }, /* state 70 */
    { 0, 1, 246, 1, 3, 0 }, /* state 71 */
    { 0, 1, 247, 1, 3, 0 }, /* state 72 */
    { 0, 1, 248, 14, 3, 0 }, /* state 73 */
    { 0, 1, 262, 15, 3, 0 }, /* state 74 */
    { 217, 1, 277, 1, 3, 0 }, /* state 75 */
    { 218, 0, 278, 0, 3, 0 }, /* state 76 */
    { 219, 1, 278, 1, 3, 0 }, /* state 77 */
    { 220, 0, 279, 0, 3, 0 }, /* state 78 */
    { 74, 0, 279, 0, 3, 0 }, /* state 79 */
    { 105, 1, 279, 1, 3, 0 }, /* state 80 */
    { 101, 0, 280, 0, 3, 0 }, /* state 81 */
    { 102, 0, 280, 0, 3, 0 }, /* state 82 */
    { 0, 1, 280, 5, 3, 0 }, /* state 83 */
    { 106, 1, 285, 1, 3, 0 }, /* state 84 */
    { 107, 0, 286, 0, 3, 0 }, /* state 85 */
    { 0, 1, 286, 3, 3, 0 }, /* state 86 */
    { 0, 1, 289, 3, 3, 0 }, /* state 87 */
    { 0, 1, 292, 1, 3, 0 }, /* state 88 */
    { 0, 1, 293, 3, 3, 0 }, /* state 89 */
    { 0, 1, 296, 1, 3, 0 }, /* state 90 */
    { 230, 1, 297, 1, 3, 0 }, /* state 91 */
    { 0, 1, 298, 14, 3, 0 }, /* state 92 */
    { 10, 0, 312, 0, 3, 0 }, /* state 93 */
    { 127, 0, 312, 0, 3, 0 }, /* state 94 */
    { 0, 1, 312, 3, 3, 0 }, /* state 95 */
    { 23, 0, 315, 0, 3, 0 }, /* state 96 */
    { 25, 0, 315, 0, 3, 0 }, /* state 97 */
    { 28, 1, 315, 2, 3, 0 }, /* state 98 */
    { 60, 0, 317, 0, 3, 0 }, /* state 99 */
    { 0, 1, 317, 7, 3, 0 }, /* state 100 */
    { 62, 1, 324, 3, 3, 0 }, /* state 101 */
    { 64, 1, 327, 14, 3, 0 }, /* state 102 */
    { 0, 1, 341, 1, 3, 0 }, /* state 103 */
    { 176, 0, 342, 0, 3, 0 }, /* state 104 */
    { 177, 1, 342, 15, 3, 0 }, /* state 105 */
    { 83, 1, 357, 5, 3, 0 }, /* state 106 */
    { 0, 1, 362, 14, 3, 0 }, /* state 107 */
    { 0, 1, 376, 14, 3, 0 }, /* state 108 */
    { 0, 1, 390, 14, 3, 0 }, /* state 109 */
    { 0, 1, 404, 14, 3, 0 }, /* state 110 */
    { 0, 1, 418, 14, 3, 0 }, /* state 111 */
    { 0, 1, 432, 14, 3, 0 }, /* state 112 */
    { 0, 1, 446, 14, 3, 0 }, /* state 113 */
    { 0, 1, 460, 14, 3, 0 }, /* state 114 */
    { 0, 1, 474, 14, 3, 0 }, /* state 115 */
    { 0, 1, 488, 14, 3, 0 }, /* state 116 */
    { 0, 1, 502, 14, 3, 0 }, /* state 117 */
    { 0, 1, 516, 14, 3, 0 }, /* state 118 */
    { 0, 1, 530, 14, 3, 0 }, /* state 119 */
    { 0, 1, 544, 14, 3, 0 }, /* state 120 */
    { 0, 1, 558, 14, 3, 0 }, /* state 121 */
    { 0, 1, 572, 14, 3, 0 }, /* state 122 */
    { 0, 1, 586, 15, 3, 0 }, /* state 123 */
    { 197, 0, 601, 0, 3, 0 }, /* state 124 */
    { 0, 1, 601, 14, 3, 0 }, /* state 125 */
    { 214, 0, 615, 0, 3, 0 }, /* state 126 */
    { 0, 1, 615, 14, 3, 0 }, /* state 127 */
    { 226, 0, 629, 0, 3, 0 }, /* state 128 */
    { 198, 1, 629, 5, 3, 0 }, /* state 129 */
    { 199, 1, 634, 5, 3, 0 }, /* state 130 */
    { 200, 1, 639, 5, 3, 0 }, /* state 131 */
    { 201, 1, 644, 5, 3, 0 }, /* state 132 */
    { 202, 1, 649, 4, 3, 0 }, /* state 133 */
    { 0, 1, 653, 17, 3, 0 }, /* state 134 */
    { 0, 1, 670, 3, 3, 0 }, /* state 135 */
    { 204, 1, 673, 5, 3, 0 }, /* state 136 */
    { 205, 1, 678, 5, 3, 0 }, /* state 137 */
    { 206, 1, 683, 5, 3, 0 }, /* state 138 */
    { 207, 1, 688, 10, 3, 0 }, /* state 139 */
    { 208, 0, 698, 0, 3, 0 }, /* state 140 */
    { 222, 0, 698, 0, 3, 0 }, /* state 141 */
    { 209, 0, 698, 0, 3, 0 }, /* state 142 */
    { 210, 1, 698, 5, 3, 0 }, /* state 143 */
    { 212, 0, 703, 0, 3, 0 }, /* state 144 */
    { 0, 1, 703, 2, 3, 0 }, /* state 145 */
    { 227, 1, 705, 15, 3, 0 }, /* state 146 */
    { 225, 0, 720, 0, 3, 0 }, /* state 147 */
    { 221, 0, 720, 0, 3, 0 }, /* state 148 */
    { 0, 1, 720, 5, 3, 0 }, /* state 149 */
    { 75, 1, 725, 2, 3, 0 }, /* state 150 */
    { 108, 0, 727, 0, 3, 0 }, /* state 151 */
    { 80, 0, 727, 0, 3, 0 }, /* state 152 */
    { 0, 1, 727, 1, 3, 0 }, /* state 153 */
    { 109, 0, 728, 0, 3, 0 }, /* state 154 */
    { 110, 0, 728, 0, 3, 0 }, /* state 155 */
    { 111, 0, 728, 0, 3, 0 }, /* state 156 */
    { 16, 0, 728, 0, 3, 0 }, /* state 157 */
    { 0, 1, 728, 4, 3, 0 }, /* state 158 */
    { 45, 0, 732, 0, 3, 0 }, /* state 159 */
    { 57, 1, 732, 5, 3, 1 }, /* state 160 */
    { 38, 0, 737, 0, 4, 0 }, /* state 161 */
    { 42, 0, 737, 0, 4, 0 }, /* state 162 */
    { 44, 0, 737, 0, 4, 0 }, /* state 163 */
    { 130, 0, 737, 0, 4, 0 }, /* state 164 */
    { 22, 0, 737, 0, 4, 0 }, /* state 165 */
    { 0, 1, 737, 5, 4, 0 }, /* state 166 */
    { 27, 0, 742, 0, 4, 0 }, /* state 167 */
    { 61, 0, 742, 0, 4, 0 }, /* state 168 */
    { 77, 0, 742, 0, 4, 0 }, /* state 169 */
    { 124, 1, 742, 1, 4, 0 }, /* state 170 */
    { 84, 0, 743, 0, 4, 0 }, /* state 171 */
    { 86, 0, 743, 0, 4, 0 }, /* state 172 */
    { 87, 0, 743, 0, 4, 0 }, /* state 173 */
    { 88, 0, 743, 0, 4, 0 }, /* state 174 */
    { 89, 0, 743, 0, 4, 0 }, /* state 175 */
    { 114, 1, 743, 5, 4, 0 }, /* state 176 */
    { 180, 1, 748, 5, 4, 0 }, /* state 177 */
    { 181, 1, 753, 15, 4, 0 }, /* state 178 */
    { 182, 0, 768, 0, 4, 0 }, /* state 179 */
    { 183, 1, 768, 2, 4, 0 }, /* state 180 */
    { 184, 1, 770, 2, 4, 0 }, /* state 181 */
    { 185, 1, 772, 4, 4, 0 }, /* state 182 */
    { 186, 0, 776, 0, 4, 0 }, /* state 183 */
    { 187, 1, 776, 6, 4, 0 }, /* state 184 */
    { 188, 1, 782, 7, 4, 0 }, /* state 185 */
    { 189, 1, 789, 9, 4, 0 }, /* state 186 */
    { 190, 1, 798, 8, 4, 0 }, /* state 187 */
    { 191, 1, 806, 10, 4, 0 }, /* state 188 */
    { 192, 1, 816, 11, 4, 0 }, /* state 189 */
    { 193, 1, 827, 15, 4, 0 }, /* state 190 */
    { 194, 1, 842, 15, 4, 0 }, /* state 191 */
    { 0, 1, 857, 17, 4, 0 }, /* state 192 */
    { 229, 1, 874, 1, 4, 0 }, /* state 193 */
    { 68, 0, 875, 0, 4, 0 }, /* state 194 */
    { 70, 1, 875, 15, 4, 0 }, /* state 195 */
    { 71, 0, 890, 0, 4, 0 }, /* state 196 */
    { 0, 1, 890, 17, 4, 0 }, /* state 197 */
    { 0, 1, 907, 1, 4, 0 }, /* state 198 */
    { 216, 1, 908, 4, 4, 0 }, /* state 199 */
    { 203, 0, 912, 0, 4, 0 }, /* state 200 */
    { 0, 1, 912, 1, 4, 0 }, /* state 201 */
    { 114, 1, 913, 4, 4, 0 }, /* state 202 */
    { 213, 0, 917, 0, 4, 0 }, /* state 203 */
    { 0, 1, 917, 14, 4, 0 }, /* state 204 */
    { 76, 1, 931, 2, 4, 0 }, /* state 205 */
    { 104, 0, 933, 0, 4, 0 }, /* state 206 */
    { 37, 0, 933, 0, 4, 0 }, /* state 207 */
    { 46, 0, 933, 0, 4, 0 }, /* state 208 */
    { 0, 1, 933, 1, 4, 0 }, /* state 209 */
    { 49, 1, 934, 2, 4, 0 }, /* state 210 */
    { 50, 0, 936, 0, 4, 0 }, /* state 211 */
    { 55, 1, 936, 2, 4, 1 }, /* state 212 */
    { 0, 1, 938, 1, 5, 0 }, /* state 213 */
    { 135, 1, 939, 4, 5, 0 }, /* state 214 */
    { 26, 0, 943, 0, 5, 0 }, /* state 215 */
    { 0, 1, 943, 1, 5, 0 }, /* state 216 */
    { 125, 1, 944, 6, 5, 0 }, /* state 217 */
    { 98, 1, 950, 2, 5, 0 }, /* state 218 */
    { 99, 1, 952, 1, 5, 0 }, /* state 219 */
    { 115, 1, 953, 6, 5, 0 }, /* state 220 */
    { 105, 1, 959, 1, 5, 0 }, /* state 221 */
    { 112, 0, 960, 0, 5, 0 }, /* state 222 */
    { 113, 0, 960, 0, 5, 0 }, /* state 223 */
    { 90, 1, 960, 5, 5, 1 }, /* state 224 */
    { 116, 0, 965, 0, 6, 0 }, /* state 225 */
    { 122, 1, 965, 1, 6, 0 }, /* state 226 */
    { 0, 1, 966, 14, 6, 0 }, /* state 227 */
    { 0, 1, 980, 1, 6, 0 }, /* state 228 */
    { 230, 1, 981, 15, 6, 0 }, /* state 229 */
    { 0, 1, 996, 15, 6, 0 }, /* state 230 */
    { 211, 0, 1011, 0, 6, 0 }, /* state 231 */
    { 215, 0, 1011, 0, 6, 0 }, /* state 232 */
    { 223, 0, 1011, 0, 6, 0 }, /* state 233 */
    { 224, 1, 1011, 1, 6, 0 }, /* state 234 */
    { 115, 1, 1012, 5, 6, 0 }, /* state 235 */
    { 105, 1, 1017, 1, 6, 0 }, /* state 236 */
    { 90, 1, 1018, 4, 6, 1 }, /* state 237 */
    { 228, 1, 1022, 15, 7, 0 }, /* state 238 */
    { 47, 0, 1037, 0, 7, 0 }, /* state 239 */
    { 57, 1, 1037, 7, 7, 0 }, /* state 240 */
    { 54, 0, 1044, 0, 7, 0 }, /* state 241 */
    { 0, 1, 1044, 14, 7, 0 }, /* state 242 */
    { 168, 0, 1058, 0, 7, 1 }, /* state 243 */
    { 131, 0, 1058, 0, 8, 0 }, /* state 244 */
    { 0, 1, 1058, 7, 8, 0 }, /* state 245 */
    { 0, 1, 1065, 1, 8, 0 }, /* state 246 */
    { 82, 0, 1066, 0, 8, 0 }, /* state 247 */
    { 85, 0, 1066, 0, 8, 0 }, /* state 248 */
    { 126, 0, 1066, 0, 8, 0 }, /* state 249 */
    { 175, 1, 1066, 14, 8, 0 }, /* state 250 */
    { 105, 1, 1080, 1, 8, 0 }, /* state 251 */
    { 117, 0, 1081, 0, 8, 0 }, /* state 252 */
    { 90, 0, 1081, 0, 8, 0 }, /* state 253 */
    { 114, 1, 1081, 5, 8, 0 }, /* state 254 */
    { 92, 1, 1086, 4, 8, 0 }, /* state 255 */
    { 113, 1, 1090, 1, 8, 0 }, /* state 256 */
    { 0, 1, 1091, 1, 8, 0 }, /* state 257 */
    { 121, 0, 1092, 0, 8, 0 }, /* state 258 */
    { 195, 1, 1092, 13, 8, 0 }, /* state 259 */
    { 65, 0, 1105, 0, 8, 0 }, /* state 260 */
    { 69, 0, 1105, 0, 8, 0 }, /* state 261 */
    { 229, 1, 1105, 1, 8, 0 }, /* state 262 */
    { 105, 1, 1106, 1, 8, 0 }, /* state 263 */
    { 114, 1, 1107, 4, 8, 0 }, /* state 264 */
    { 51, 0, 1111, 0, 8, 0 }, /* state 265 */
    { 57, 1, 1111, 6, 8, 0 }, /* state 266 */
    { 53, 0, 1117, 0, 8, 0 }, /* state 267 */
    { 56, 0, 1117, 0, 8, 0 }, /* state 268 */
    { 0, 1, 1117, 1, 8, 0 }, /* state 269 */
    { 136, 0, 1118, 0, 8, 0 }, /* state 270 */
    { 0, 1, 1118, 32, 8, 0 }, /* state 271 */
    { 0, 1, 1150, 2, 8, 0 }, /* state 272 */
    { 133, 0, 1152, 0, 8, 0 }, /* state 273 */
    { 134, 0, 1152, 0, 8, 0 }, /* state 274 */
    { 0, 1, 1152, 1, 8, 0 }, /* state 275 */
    { 114, 1, 1153, 5, 8, 0 }, /* state 276 */
    { 118, 1, 1158, 1, 8, 0 }, /* state 277 */
    { 124, 1, 1159, 1, 8, 0 }, /* state 278 */
    { 93, 0, 1160, 0, 8, 0 }, /* state 279 */
    { 95, 0, 1160, 0, 8, 0 }, /* state 280 */
    { 96, 0, 1160, 0, 8, 0 }, /* state 281 */
    { 97, 0, 1160, 0, 8, 0 }, /* state 282 */
    { 123, 0, 1160, 0, 8, 0 }, /* state 283 */
    { 0, 1, 1160, 1, 8, 0 }, /* state 284 */
    { 114, 1, 1161, 4, 8, 0 }, /* state 285 */
    { 52, 0, 1165, 0, 8, 0 }, /* state 286 */
    { 129, 0, 1165, 0, 8, 0 }, /* state 287 */
    { 137, 0, 1165, 0, 8, 0 }, /* state 288 */
    { 169, 0, 1165, 0, 8, 0 }, /* state 289 */
    { 0, 1, 1165, 17, 8, 0 }, /* state 290 */
    { 139, 0, 1182, 0, 8, 0 }, /* state 291 */
    { 168, 0, 1182, 0, 8, 0 }, /* state 292 */
    { 168, 0, 1182, 0, 8, 0 }, /* state 293 */
    { 0, 1, 1182, 1, 8, 0 }, /* state 294 */
    { 0, 1, 1183, 1, 8, 0 }, /* state 295 */
    { 147, 0, 1184, 0, 8, 0 }, /* state 296 */
    { 0, 1, 1184, 1, 8, 0 }, /* state 297 */
    { 0, 1, 1185, 1, 8, 0 }, /* state 298 */
    { 0, 1, 1186, 1, 8, 0 }, /* state 299 */
    { 0, 1, 1187, 1, 8, 0 }, /* state 300 */
    { 0, 1, 1188, 15, 8, 0 }, /* state 301 */
    { 0, 1, 1203, 1, 8, 0 }, /* state 302 */
    { 157, 0, 1204, 0, 8, 0 }, /* state 303 */
    { 0, 1, 1204, 1, 8, 0 }, /* state 304 */
    { 0, 1, 1205, 2, 8, 0 }, /* state 305 */
    { 161, 0, 1207, 0, 8, 0 }, /* state 306 */
    { 162, 0, 1207, 0, 8, 0 }, /* state 307 */
    { 0, 1, 1207, 1, 8, 0 }, /* state 308 */
    { 217, 1, 1208, 2, 8, 0 }, /* state 309 */
    { 0, 1, 1210, 14, 8, 0 }, /* state 310 */
    { 0, 1, 1224, 1, 8, 0 }, /* state 311 */
    { 0, 1, 1225, 1, 8, 0 }, /* state 312 */
    { 164, 0, 1226, 0, 8, 0 }, /* state 313 */
    { 132, 0, 1226, 0, 8, 0 }, /* state 314 */
    { 120, 0, 1226, 0, 8, 0 }, /* state 315 */
    { 119, 1, 1226, 1, 8, 0 }, /* state 316 */
    { 0, 1, 1227, 1, 8, 0 }, /* state 317 */
    { 125, 1, 1228, 5, 8, 0 }, /* state 318 */
    { 72, 0, 1233, 0, 8, 0 }, /* state 319 */
    { 138, 0, 1233, 0, 8, 0 }, /* state 320 */
    { 140, 1, 1233, 1, 8, 0 }, /* state 321 */
    { 141, 0, 1234, 0, 8, 0 }, /* state 322 */
    { 0, 1, 1234, 14, 8, 0 }, /* state 323 */
    { 178, 1, 1248, 14, 8, 0 }, /* state 324 */
    { 168, 0, 1262, 0, 8, 0 }, /* state 325 */
    { 0, 1, 1262, 14, 8, 0 }, /* state 326 */
    { 151, 0, 1276, 0, 8, 0 }, /* state 327 */
    { 152, 0, 1276, 0, 8, 0 }, /* state 328 */
    { 153, 0, 1276, 0, 8, 0 }, /* state 329 */
    { 154, 0, 1276, 0, 8, 0 }, /* state 330 */
    { 0, 1, 1276, 17, 8, 0 }, /* state 331 */
    { 0, 1, 1293, 1, 8, 0 }, /* state 332 */
    { 158, 0, 1294, 0, 8, 0 }, /* state 333 */
    { 159, 0, 1294, 0, 8, 0 }, /* state 334 */
    { 166, 0, 1294, 0, 8, 0 }, /* state 335 */
    { 163, 0, 1294, 0, 8, 0 }, /* state 336 */
    { 170, 0, 1294, 0, 8, 0 }, /* state 337 */
    { 0, 1, 1294, 17, 8, 0 }, /* state 338 */
    { 172, 0, 1311, 0, 8, 0 }, /* state 339 */
    { 0, 1, 1311, 14, 8, 0 }, /* state 340 */
    { 0, 1, 1325, 1, 8, 0 }, /* state 341 */
    { 91, 0, 1326, 0, 8, 0 }, /* state 342 */
    { 94, 0, 1326, 0, 8, 0 }, /* state 343 */
    { 174, 0, 1326, 0, 8, 0 }, /* state 344 */
    { 0, 1, 1326, 17, 8, 0 }, /* state 345 */
    { 0, 1, 1343, 1, 8, 0 }, /* state 346 */
    { 179, 1, 1344, 16, 8, 0 }, /* state 347 */
    { 0, 1, 1360, 1, 8, 0 }, /* state 348 */
    { 0, 1, 1361, 17, 8, 0 }, /* state 349 */
    { 155, 0, 1378, 0, 8, 0 }, /* state 350 */
    { 156, 0, 1378, 0, 8, 0 }, /* state 351 */
    { 0, 1, 1378, 1, 8, 0 }, /* state 352 */
    { 0, 1, 1379, 1, 8, 0 }, /* state 353 */
    { 171, 0, 1380, 0, 8, 0 }, /* state 354 */
    { 0, 1, 1380, 17, 8, 0 }, /* state 355 */
    { 165, 0, 1397, 0, 8, 0 }, /* state 356 */
    { 142, 0, 1397, 0, 8, 0 }, /* state 357 */
    { 178, 1, 1397, 14, 8, 0 }, /* state 358 */
    { 0, 1, 1411, 1, 8, 0 }, /* state 359 */
    { 149, 0, 1412, 0, 8, 0 }, /* state 360 */
    { 160, 0, 1412, 0, 8, 0 }, /* state 361 */
    { 0, 1, 1412, 14, 8, 0 }, /* state 362 */
    { 173, 0, 1426, 0, 8, 0 }, /* state 363 */
    { 168, 0, 1426, 0, 8, 0 }, /* state 364 */
    { 0, 1, 1426, 1, 8, 0 }, /* state 365 */
    { 0, 1, 1427, 14, 8, 0 }, /* state 366 */
    { 168, 0, 1441, 0, 8, 0 }, /* state 367 */
    { 0, 1, 1441, 17, 8, 0 }, /* state 368 */
    { 143, 0, 1458, 0, 8, 0 }, /* state 369 */
    { 178, 1, 1458, 14, 8, 0 }, /* state 370 */
    { 0, 1, 1472, 17, 8, 0 }, /* state 371 */
    { 150, 0, 1489, 0, 8, 0 }, /* state 372 */
    { 167, 0, 1489, 0, 8, 0 }, /* state 373 */
    { 144, 0, 1489, 0, 8, 0 }, /* state 374 */
    { 0, 1, 1489, 1, 8, 0 }, /* state 375 */
    { 0, 1, 1490, 1, 8, 0 }, /* state 376 */
    { 148, 0, 1491, 0, 8, 0 }, /* state 377 */
    { 145, 0, 1491, 0, 8, 0 }, /* state 378 */
    { 168, 0, 1491, 0, 8, 0 }, /* state 379 */
    { 146, 0, 1491, 0, 8, 0 }, /* state 380 */
    { 0, 1, 1491, 0, 8, 0 }, /* state 381 */
    { 0, 1, 1491, 0, 8, 0 }, /* state 382 */
    { 0, 1, 1491, 0, 8, 0 }, /* state 383 */
};

const ParserGoto parser_gotos[PARSER_NONTERMINAL_COUNT] = {
    { 105, 0, 0 }, /* nonterminal 0 */
    { 1, 0, 0 }, /* nonterminal 1 */
    { 35, 0, 3 }, /* nonterminal 2 */
    { 153, 3, 0 }, /* nonterminal 3 */
    { 31, 3, 8 }, /* nonterminal 4 */
    { 84, 11, 2 }, /* nonterminal 5 */
    { 83, 13, 5 }, /* nonterminal 6 */
    { 10, 18, 0 }, /* nonterminal 7 */
    { 176, 18, 7 }, /* nonterminal 8 */
    { 15, 25, 0 }, /* nonterminal 9 */
    { 17, 25, 0 }, /* nonterminal 10 */
    { 18, 25, 0 }, /* nonterminal 11 */
    { 19, 25, 0 }, /* nonterminal 12 */
    { 21, 25, 0 }, /* nonterminal 13 */
    { 105, 25, 37 }, /* nonterminal 14 */
    { 346, 62, 2 }, /* nonterminal 15 */
    { 61, 64, 11 }, /* nonterminal 16 */
    { 145, 75, 0 }, /* nonterminal 17 */
    { 74, 75, 0 }, /* nonterminal 18 */
    { 292, 75, 0 }, /* nonterminal 19 */
    { 293, 75, 0 }, /* nonterminal 20 */
    { 77, 75, 0 }, /* nonterminal 21 */
    { 29, 75, 2 }, /* nonterminal 22 */
    { 102, 77, 1 }, /* nonterminal 23 */
    { 201, 78, 0 }, /* nonterminal 24 */
    { 73, 78, 3 }, /* nonterminal 25 */
    { 174, 81, 2 }, /* nonterminal 26 */
    { 173, 83, 2 }, /* nonterminal 27 */
    { 279, 85, 1 }, /* nonterminal 28 */
    { 171, 86, 1 }, /* nonterminal 29 */
    { 216, 87, 1 }, /* nonterminal 30 */
    { 9, 88, 0 }, /* nonterminal 31 */
    { 213, 88, 0 }, /* nonterminal 32 */
    { 33, 88, 0 }, /* nonterminal 33 */
    { 101, 88, 13 }, /* nonterminal 34 */
    { 223, 101, 2 }, /* nonterminal 35 */
    { 226, 103, 0 }, /* nonterminal 36 */
    { 257, 103, 6 }, /* nonterminal 37 */
    { 222, 109, 0 }, /* nonterminal 38 */
    { 211, 109, 2 }, /* nonterminal 39 */
    { 56, 111, 0 }, /* nonterminal 40 */
    { 170, 111, 0 }, /* nonterminal 41 */
    { 225, 111, 3 }, /* nonterminal 42 */
    { 278, 114, 0 }, /* nonterminal 43 */
    { 103, 114, 1 }, /* nonterminal 44 */
    { 104, 115, 2 }, /* nonterminal 45 */
    { 288, 117, 0 }, /* nonterminal 46 */
    { 2, 117, 0 }, /* nonterminal 47 */
    { 4, 117, 1 }, /* nonterminal 48 */
    { 5, 118, 0 }, /* nonterminal 49 */
    { 25, 118, 0 }, /* nonterminal 50 */
    { 48, 118, 0 }, /* nonterminal 51 */
    { 93, 118, 4 }, /* nonterminal 52 */
    { 26, 122, 1 }, /* nonterminal 53 */
    { 88, 123, 0 }, /* nonterminal 54 */
    { 49, 123, 0 }, /* nonterminal 55 */
    { 95, 123, 0 }, /* nonterminal 56 */
    { 97, 123, 1 }, /* nonterminal 57 */
    { 42, 124, 0 }, /* nonterminal 58 */
    { 158, 124, 0 }, /* nonterminal 59 */
    { 43, 124, 0 }, /* nonterminal 60 */
    { 90, 124, 2 }, /* nonterminal 61 */
    { 45, 126, 1 }, /* nonterminal 62 */
    { 159, 127, 1 }, /* nonterminal 63 */
    { 209, 128, 0 }, /* nonterminal 64 */
    { 210, 128, 0 }, /* nonterminal 65 */
    { 60, 128, 0 }, /* nonterminal 66 */
    { 193, 128, 1 }, /* nonterminal 67 */
    { 194, 129, 1 }, /* nonterminal 68 */
    { 230, 130, 0 }, /* nonterminal 69 */
    { 106, 130, 0 }, /* nonterminal 70 */
    { 255, 130, 0 }, /* nonterminal 71 */
    { 164, 130, 0 }, /* nonterminal 72 */
    { 214, 130, 0 }, /* nonterminal 73 */
    { 243, 130, 0 }, /* nonterminal 74 */
    { 269, 130, 0 }, /* nonterminal 75 */
    { 244, 130, 0 }, /* nonterminal 76 */
    { 270, 130, 6 }, /* nonterminal 77 */
    { 271, 136, 0 }, /* nonterminal 78 */
    { 364, 136, 0 }, /* nonterminal 79 */
    { 376, 136, 0 }, /* nonterminal 80 */
    { 379, 136, 0 }, /* nonterminal 81 */
    { 325, 136, 0 }, /* nonterminal 82 */
    { 367, 136, 0 }, /* nonterminal 83 */
    { 304, 136, 0 }, /* nonterminal 84 */
    { 305, 136, 0 }, /* nonterminal 85 */
    { 352, 136, 0 }, /* nonterminal 86 */
    { 341, 136, 0 }, /* nonterminal 87 */
    { 353, 136, 0 }, /* nonterminal 88 */
    { 289, 136, 0 }, /* nonterminal 89 */
    { 198, 136, 0 }, /* nonterminal 90 */
    { 135, 136, 0 }, /* nonterminal 91 */
};

const ParserTransition parser_goto_transitions[] = {
    /* Nonterminal 2: source state, destination. */
    { 31, 81 },
    { 220, 81 },
    { 235, 81 },
    /* Nonterminal 4: source state, destination. */
    { 176, 220 },
    { 202, 235 },
    { 224, 220 },
    { 237, 235 },
    { 254, 220 },
    { 264, 235 },
    { 276, 220 },
    { 285, 235 },
    /* Nonterminal 5: source state, destination. */
    { 53, 100 },
    { 240, 266 },
    /* Nonterminal 6: source state, destination. */
    { 80, 149 },
    { 221, 254 },
    { 236, 264 },
    { 251, 276 },
    { 263, 285 },
    /* Nonterminal 8: source state, destination. */
    { 0, 12 },
    { 2, 12 },
    { 25, 50 },
    { 89, 160 },
    { 135, 202 },
    { 158, 160 },
    { 214, 245 },
    /* Nonterminal 14: source state, destination. */
    { 28, 59 },
    { 67, 134 },
    { 74, 146 },
    { 102, 59 },
    { 107, 177 },
    { 108, 178 },
    { 109, 179 },
    { 110, 180 },
    { 111, 181 },
    { 112, 182 },
    { 113, 183 },
    { 114, 184 },
    { 115, 185 },
    { 116, 186 },
    { 117, 187 },
    { 118, 188 },
    { 119, 189 },
    { 120, 190 },
    { 121, 191 },
    { 122, 192 },
    { 123, 195 },
    { 125, 197 },
    { 204, 238 },
    { 227, 259 },
    { 229, 195 },
    { 230, 195 },
    { 271, 290 },
    { 301, 331 },
    { 310, 338 },
    { 323, 345 },
    { 324, 347 },
    { 326, 349 },
    { 340, 355 },
    { 358, 347 },
    { 362, 368 },
    { 366, 371 },
    { 370, 347 },
    /* Nonterminal 15: source state, destination. */
    { 358, 365 },
    { 370, 374 },
    /* Nonterminal 16: source state, destination. */
    { 62, 129 },
    { 63, 130 },
    { 64, 131 },
    { 65, 132 },
    { 66, 133 },
    { 68, 136 },
    { 69, 137 },
    { 70, 138 },
    { 73, 143 },
    { 127, 199 },
    { 139, 143 },
    /* Nonterminal 22: source state, destination. */
    { 53, 99 },
    { 100, 168 },
    /* Nonterminal 23: source state, destination. */
    { 10, 28 },
    /* Nonterminal 25: source state, destination. */
    { 70, 139 },
    { 71, 140 },
    { 72, 142 },
    /* Nonterminal 26: source state, destination. */
    { 255, 281 },
    { 318, 281 },
    /* Nonterminal 27: source state, destination. */
    { 255, 280 },
    { 318, 280 },
    /* Nonterminal 28: source state, destination. */
    { 318, 343 },
    /* Nonterminal 29: source state, destination. */
    { 217, 248 },
    /* Nonterminal 30: source state, destination. */
    { 278, 317 },
    /* Nonterminal 34: source state, destination. */
    { 10, 27 },
    { 36, 86 },
    { 50, 98 },
    { 83, 150 },
    { 149, 205 },
    { 160, 212 },
    { 166, 98 },
    { 176, 218 },
    { 224, 86 },
    { 240, 212 },
    { 254, 150 },
    { 266, 212 },
    { 276, 205 },
    /* Nonterminal 35: source state, destination. */
    { 224, 256 },
    { 237, 256 },
    /* Nonterminal 37: source state, destination. */
    { 176, 219 },
    { 202, 234 },
    { 254, 277 },
    { 264, 277 },
    { 276, 316 },
    { 285, 316 },
    /* Nonterminal 39: source state, destination. */
    { 240, 265 },
    { 266, 286 },
    /* Nonterminal 42: source state, destination. */
    { 220, 252 },
    { 226, 258 },
    { 235, 252 },
    /* Nonterminal 44: source state, destination. */
    { 250, 275 },
    /* Nonterminal 45: source state, destination. */
    { 92, 163 },
    { 242, 268 },
    /* Nonterminal 48: source state, destination. */
    { 2, 24 },
    /* Nonterminal 52: source state, destination. */
    { 271, 291 },
    { 304, 333 },
    { 341, 356 },
    { 352, 361 },
    /* Nonterminal 53: source state, destination. */
    { 245, 272 },
    /* Nonterminal 57: source state, destination. */
    { 166, 215 },
    /* Nonterminal 61: source state, destination. */
    { 193, 228 },
    { 262, 284 },
    /* Nonterminal 62: source state, destination. */
    { 91, 162 },
    /* Nonterminal 63: source state, destination. */
    { 158, 208 },
    /* Nonterminal 67: source state, destination. */
    { 230, 262 },
    /* Nonterminal 68: source state, destination. */
    { 229, 261 },
    /* Nonterminal 77: source state, destination. */
    { 292, 321 },
    { 293, 322 },
    { 325, 348 },
    { 364, 369 },
    { 367, 372 },
    { 379, 380 },
};

const ParserProduction parser_productions[PARSER_PRODUCTION_COUNT + 1] = {
    { 0, 0 }, /* production 0 */
    { 1, 2 }, /* production 1 */
    { 1, 1 }, /* production 2 */
    { 47, 2 }, /* production 3 */
    { 47, 1 }, /* production 4 */
    { 48, 1 }, /* production 5 */
    { 48, 1 }, /* production 6 */
    { 48, 1 }, /* production 7 */
    { 48, 1 }, /* production 8 */
    { 51, 0 }, /* production 9 */
    { 49, 4 }, /* production 10 */
    { 49, 3 }, /* production 11 */
    { 49, 1 }, /* production 12 */
    { 49, 2 }, /* production 13 */
    { 49, 2 }, /* production 14 */
    { 54, 0 }, /* production 15 */
    { 49, 4 }, /* production 16 */
    { 49, 1 }, /* production 17 */
    { 31, 2 }, /* production 18 */
    { 31, 3 }, /* production 19 */
    { 50, 0 }, /* production 20 */
    { 50, 2 }, /* production 21 */
    { 55, 3 }, /* production 22 */
    { 55, 2 }, /* production 23 */
    { 55, 1 }, /* production 24 */
    { 56, 1 }, /* production 25 */
    { 56, 3 }, /* production 26 */
    { 56, 2 }, /* production 27 */
    { 57, 1 }, /* production 28 */
    { 7, 0 }, /* production 29 */
    { 7, 1 }, /* production 30 */
    { 8, 1 }, /* production 31 */
    { 9, 1 }, /* production 32 */
    { 9, 1 }, /* production 33 */
    { 10, 1 }, /* production 34 */
    { 10, 1 }, /* production 35 */
    { 58, 0 }, /* production 36 */
    { 11, 5 }, /* production 37 */
    { 12, 4 }, /* production 38 */
    { 12, 2 }, /* production 39 */
    { 13, 2 }, /* production 40 */
    { 60, 1 }, /* production 41 */
    { 60, 3 }, /* production 42 */
    { 62, 1 }, /* production 43 */
    { 62, 3 }, /* production 44 */
    { 59, 1 }, /* production 45 */
    { 59, 2 }, /* production 46 */
    { 63, 3 }, /* production 47 */
    { 64, 0 }, /* production 48 */
    { 64, 1 }, /* production 49 */
    { 65, 1 }, /* production 50 */
    { 65, 3 }, /* production 51 */
    { 65, 4 }, /* production 52 */
    { 65, 3 }, /* production 53 */
    { 65, 2 }, /* production 54 */
    { 39, 1 }, /* production 55 */
    { 39, 3 }, /* production 56 */
    { 32, 0 }, /* production 57 */
    { 32, 1 }, /* production 58 */
    { 53, 1 }, /* production 59 */
    { 53, 3 }, /* production 60 */
    { 53, 4 }, /* production 61 */
    { 22, 1 }, /* production 62 */
    { 22, 2 }, /* production 63 */
    { 66, 0 }, /* production 64 */
    { 22, 6 }, /* production 65 */
    { 23, 2 }, /* production 66 */
    { 23, 1 }, /* production 67 */
    { 67, 1 }, /* production 68 */
    { 67, 3 }, /* production 69 */
    { 68, 1 }, /* production 70 */
    { 69, 0 }, /* production 71 */
    { 68, 5 }, /* production 72 */
    { 34, 1 }, /* production 73 */
    { 34, 2 }, /* production 74 */
    { 34, 3 }, /* production 75 */
    { 34, 4 }, /* production 76 */
    { 34, 4 }, /* production 77 */
    { 34, 2 }, /* production 78 */
    { 34, 1 }, /* production 79 */
    { 33, 3 }, /* production 80 */
    { 70, 0 }, /* production 81 */
    { 40, 5 }, /* production 82 */
    { 41, 0 }, /* production 83 */
    { 41, 1 }, /* production 84 */
    { 41, 3 }, /* production 85 */
    { 29, 1 }, /* production 86 */
    { 29, 1 }, /* production 87 */
    { 29, 1 }, /* production 88 */
    { 29, 1 }, /* production 89 */
    { 71, 0 }, /* production 90 */
    { 42, 5 }, /* production 91 */
    { 43, 0 }, /* production 92 */
    { 43, 1 }, /* production 93 */
    { 43, 3 }, /* production 94 */
    { 28, 1 }, /* production 95 */
    { 28, 1 }, /* production 96 */
    { 28, 1 }, /* production 97 */
    { 27, 2 }, /* production 98 */
    { 26, 2 }, /* production 99 */
    { 4, 1 }, /* production 100 */
    { 4, 2 }, /* production 101 */
    { 4, 2 }, /* production 102 */
    { 2, 1 }, /* production 103 */
    { 2, 4 }, /* production 104 */
    { 6, 0 }, /* production 105 */
    { 6, 1 }, /* production 106 */
    { 5, 1 }, /* production 107 */
    { 5, 2 }, /* production 108 */
    { 3, 1 }, /* production 109 */
    { 3, 1 }, /* production 110 */
    { 3, 1 }, /* production 111 */
    { 37, 1 }, /* production 112 */
    { 37, 1 }, /* production 113 */
    { 38, 0 }, /* production 114 */
    { 38, 1 }, /* production 115 */
    { 38, 1 }, /* production 116 */
    { 38, 2 }, /* production 117 */
    { 35, 3 }, /* production 118 */
    { 35, 4 }, /* production 119 */
    { 35, 4 }, /* production 120 */
    { 35, 2 }, /* production 121 */
    { 35, 1 }, /* production 122 */
    { 36, 3 }, /* production 123 */
    { 30, 0 }, /* production 124 */
    { 30, 1 }, /* production 125 */
    { 30, 2 }, /* production 126 */
    { 72, 0 }, /* production 127 */
    { 75, 0 }, /* production 128 */
    { 52, 6 }, /* production 129 */
    { 73, 0 }, /* production 130 */
    { 73, 2 }, /* production 131 */
    { 76, 3 }, /* production 132 */
    { 76, 2 }, /* production 133 */
    { 76, 2 }, /* production 134 */
    { 74, 0 }, /* production 135 */
    { 74, 2 }, /* production 136 */
    { 77, 2 }, /* production 137 */
    { 46, 2 }, /* production 138 */
    { 46, 1 }, /* production 139 */
    { 46, 2 }, /* production 140 */
    { 46, 2 }, /* production 141 */
    { 79, 0 }, /* production 142 */
    { 46, 6 }, /* production 143 */
    { 80, 0 }, /* production 144 */
    { 81, 0 }, /* production 145 */
    { 46, 11 }, /* production 146 */
    { 82, 0 }, /* production 147 */
    { 46, 8 }, /* production 148 */
    { 83, 0 }, /* production 149 */
    { 46, 6 }, /* production 150 */
    { 46, 2 }, /* production 151 */
    { 46, 2 }, /* production 152 */
    { 46, 2 }, /* production 153 */
    { 46, 2 }, /* production 154 */
    { 46, 3 }, /* production 155 */
    { 46, 3 }, /* production 156 */
    { 46, 1 }, /* production 157 */
    { 46, 2 }, /* production 158 */
    { 86, 0 }, /* production 159 */
    { 46, 4 }, /* production 160 */
    { 46, 1 }, /* production 161 */
    { 46, 1 }, /* production 162 */
    { 46, 2 }, /* production 163 */
    { 87, 0 }, /* production 164 */
    { 85, 3 }, /* production 165 */
    { 88, 0 }, /* production 166 */
    { 84, 6 }, /* production 167 */
    { 78, 0 }, /* production 168 */
    { 78, 2 }, /* production 169 */
    { 89, 2 }, /* production 170 */
    { 89, 3 }, /* production 171 */
    { 89, 2 }, /* production 172 */
    { 19, 4 }, /* production 173 */
    { 20, 3 }, /* production 174 */
    { 44, 0 }, /* production 175 */
    { 44, 1 }, /* production 176 */
    { 45, 1 }, /* production 177 */
    { 15, 0 }, /* production 178 */
    { 15, 1 }, /* production 179 */
    { 14, 3 }, /* production 180 */
    { 14, 3 }, /* production 181 */
    { 14, 3 }, /* production 182 */
    { 14, 3 }, /* production 183 */
    { 14, 3 }, /* production 184 */
    { 14, 3 }, /* production 185 */
    { 14, 3 }, /* production 186 */
    { 14, 3 }, /* production 187 */
    { 14, 3 }, /* production 188 */
    { 14, 3 }, /* production 189 */
    { 14, 3 }, /* production 190 */
    { 14, 3 }, /* production 191 */
    { 14, 3 }, /* production 192 */
    { 14, 3 }, /* production 193 */
    { 14, 3 }, /* production 194 */
    { 14, 5 }, /* production 195 */
    { 14, 1 }, /* production 196 */
    { 16, 2 }, /* production 197 */
    { 16, 2 }, /* production 198 */
    { 16, 2 }, /* production 199 */
    { 16, 2 }, /* production 200 */
    { 16, 2 }, /* production 201 */
    { 16, 2 }, /* production 202 */
    { 16, 3 }, /* production 203 */
    { 16, 2 }, /* production 204 */
    { 16, 2 }, /* production 205 */
    { 16, 2 }, /* production 206 */
    { 16, 2 }, /* production 207 */
    { 16, 2 }, /* production 208 */
    { 16, 2 }, /* production 209 */
    { 16, 2 }, /* production 210 */
    { 16, 4 }, /* production 211 */
    { 16, 2 }, /* production 212 */
    { 16, 3 }, /* production 213 */
    { 90, 0 }, /* production 214 */
    { 16, 4 }, /* production 215 */
    { 16, 3 }, /* production 216 */
    { 16, 1 }, /* production 217 */
    { 16, 1 }, /* production 218 */
    { 16, 1 }, /* production 219 */
    { 21, 1 }, /* production 220 */
    { 21, 2 }, /* production 221 */
    { 91, 0 }, /* production 222 */
    { 25, 4 }, /* production 223 */
    { 24, 2 }, /* production 224 */
    { 18, 2 }, /* production 225 */
    { 18, 2 }, /* production 226 */
    { 17, 1 }, /* production 227 */
    { 17, 3 }, /* production 228 */
    { 61, 0 }, /* production 229 */
    { 61, 1 }, /* production 230 */
};

const ParserRecovery parser_recoveries[46] = {
    { 0, -PARSER_CLASS_127, 11 },
    { 2, -PARSER_CLASS_127, 11 },
    { 13, -PARSER_CLASS_127, 40 },
    { 26, -PARSER_CLASS_127, 52 },
    { 50, -PARSER_CLASS_127, 96 },
    { 74, -PARSER_CLASS_126, 144 },
    { 86, -PARSER_CLASS_126, 152 },
    { 95, -PARSER_CLASS_127, 165 },
    { 103, PARSER_CLASS_122, 169 },
    { 134, -PARSER_CLASS_126, 200 },
    { 145, -PARSER_CLASS_126, 203 },
    { 153, PARSER_CLASS_126, 206 },
    { 192, -PARSER_CLASS_54, 227 },
    { 197, -PARSER_CLASS_122, 231 },
    { 201, PARSER_CLASS_126, 233 },
    { 209, PARSER_CLASS_127, 239 },
    { 210, -PARSER_CLASS_55, 240 },
    { 213, PARSER_CLASS_54, 242 },
    { 216, PARSER_CLASS_126, 247 },
    { 245, -PARSER_CLASS_127, 273 },
    { 246, PARSER_CLASS_127, 274 },
    { 271, -PARSER_CLASS_127, 306 },
    { 272, -PARSER_CLASS_127, 314 },
    { 275, PARSER_CLASS_122, 315 },
    { 290, -PARSER_CLASS_127, 320 },
    { 298, PARSER_CLASS_127, 327 },
    { 299, PARSER_CLASS_127, 328 },
    { 300, PARSER_CLASS_127, 329 },
    { 301, -PARSER_CLASS_127, 330 },
    { 308, PARSER_CLASS_127, 336 },
    { 309, -PARSER_CLASS_54, 337 },
    { 311, PARSER_CLASS_54, 339 },
    { 317, PARSER_CLASS_126, 342 },
    { 331, -PARSER_CLASS_127, 350 },
    { 332, PARSER_CLASS_127, 351 },
    { 338, -PARSER_CLASS_54, 354 },
    { 345, -PARSER_CLASS_126, 357 },
    { 346, PARSER_CLASS_127, 358 },
    { 347, -PARSER_CLASS_55, 108 },
    { 349, -PARSER_CLASS_126, 360 },
    { 355, -PARSER_CLASS_126, 363 },
    { 365, PARSER_CLASS_127, 370 },
    { 368, -PARSER_CLASS_126, 373 },
    { 371, -PARSER_CLASS_126, 375 },
    { 375, PARSER_CLASS_127, 377 },
    { 376, PARSER_CLASS_126, 378 },
};
