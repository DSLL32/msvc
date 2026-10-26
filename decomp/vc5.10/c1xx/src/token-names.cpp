#include "frontend.h"
#include <recon.h>

extern "C" CompilerNameEntry *__cdecl find_or_create_identifier(const char *);
/* The encountered token spelling table at d3d10 contains 246 pointer slots.
 * The next DWORD begins unrelated character data. NULL slots and wider byte
 * tokens are not supported native inputs; no surrounding image is modeled. */
extern const char *const compiler_token_spellings[246] = {
        NULL, /* 0 */
        "newline", /* 1 */
        "#pragma hdrstop", /* 2 */
        "#line directive for PCH creation", /* 3 */
        "PCH creation point", /* 4 */
        "end of file", /* 5 */
        "identifier", /* 6 */
        "string", /* 7 */
        "auto", /* 8 */
        "static", /* 9 */
        "extern", /* 10 */
        "register", /* 11 */
        "typedef", /* 12 */
        "friend", /* 13 */
        "inline", /* 14 */
        "virtual", /* 15 */
        "char", /* 16 */
        "int", /* 17 */
        "float", /* 18 */
        "double", /* 19 */
        "short", /* 20 */
        "long", /* 21 */
        "void", /* 22 */
        "__segname", /* 23 */
        "signed", /* 24 */
        "unsigned", /* 25 */
        "class", /* 26 */
        "enum", /* 27 */
        "struct", /* 28 */
        "union", /* 29 */
        "<type name>", /* 30 */
        "<type>", /* 31 */
        "<storage class>", /* 32 */
        "<storage modifier>", /* 33 */
        "<class-head>", /* 34 */
        "enum [tag]", /* 35 */
        "type", /* 36 */
        "__cdecl", /* 37 */
        "__fastcall", /* 38 */
        "__restrict", /* 39 */
        "right-side modifier", /* 40 */
        "inline trap definition", /* 41 */
        "__asm", /* 42 */
        "asm string", /* 43 */
        "__based", /* 44 */
        "newline", /* 45 */
        "hidden udt return argument", /* 46 */
        "const", /* 47 */
        "volatile", /* 48 */
        "&", /* 49 */
        "&&", /* 50 */
        "&=", /* 51 */
        "=", /* 52 */
        "bit field", /* 53 */
        ":", /* 54 */
        ",", /* 55 */
        "--", /* 56 */
        "/", /* 57 */
        "/=", /* 58 */
        "==", /* 59 */
        "!", /* 60 */
        "*", /* 61 */
        ">", /* 62 */
        ">=", /* 63 */
        "++", /* 64 */
        "[", /* 65 */
        "<<=", /* 66 */
        "<<", /* 67 */
        "<", /* 68 */
        "<=", /* 69 */
        "-", /* 70 */
        "-=", /* 71 */
        "%", /* 72 */
        "%=", /* 73 */
        "*", /* 74 */
        "*=", /* 75 */
        "!=", /* 76 */
        "|", /* 77 */
        "|=", /* 78 */
        "||", /* 79 */
        ".", /* 80 */
        "+", /* 81 */
        "+=", /* 82 */
        "->", /* 83 */
        "--", /* 84 */
        "++", /* 85 */
        "?", /* 86 */
        ">>=", /* 87 */
        ">>", /* 88 */
        "~", /* 89 */
        "-", /* 90 */
        "^", /* 91 */
        "^=", /* 92 */
        "based reference", /* 93 */
        ":>", /* 94 */
        "deseg", /* 95 */
        "argument", /* 96 */
        "assignment operator", /* 97 */
        "break", /* 98 */
        "case", /* 99 */
        "type cast", /* 100 */
        "constant", /* 101 */
        "continue", /* 102 */
        "default", /* 103 */
        "division operator", /* 104 */
        "do", /* 105 */
        "...", /* 106 */
        "else", /* 107 */
        "entry", /* 108 */
        "end of file", /* 109 */
        "equality operator", /* 110 */
        "for", /* 111 */
        "function", /* 112 */
        "goto", /* 113 */
        "if", /* 114 */
        "increment operator", /* 115 */
        "initializing", /* 116 */
        "{", /* 117 */
        "(", /* 118 */
        "intrinsic", /* 119 */
        "left-side modifier", /* 120 */
        "typeid", /* 121 */
        "]", /* 122 */
        "}", /* 123 */
        "relational operator", /* 124 */
        "return", /* 125 */
        ")", /* 126 */
        ";", /* 127 */
        "shift operator", /* 128 */
        "sizeof", /* 129 */
        "__stdcall", /* 130 */
        "struct/union operator", /* 131 */
        "switch", /* 132 */
        NULL, /* 133 */
        "+", /* 134 */
        "while", /* 135 */
        "internal compiler token", /* 136 */
        "private", /* 137 */
        "protected", /* 138 */
        "public", /* 139 */
        "new", /* 140 */
        "delete", /* 141 */
        "operator <op>", /* 142 */
        "user-defined conversion", /* 143 */
        "this", /* 144 */
        "<access specifier>", /* 145 */
        ".*", /* 146 */
        "->*", /* 147 */
        ".*|->*", /* 148 */
        "::", /* 149 */
        "tag::id", /* 150 */
        "operator", /* 151 */
        "function-style cast", /* 152 */
        "<member function>", /* 153 */
        "<virtual function>", /* 154 */
        "<constructor>", /* 155 */
        "<allocate temp>", /* 156 */
        "<thunkx>", /* 157 */
        "<ptradd>", /* 158 */
        "<tag>::*", /* 159 */
        "*", /* 160 */
        "&", /* 161 */
        "modifier", /* 162 */
        "const", /* 163 */
        "volatile", /* 164 */
        "__based", /* 165 */
        "*", /* 166 */
        "__try", /* 167 */
        "__except", /* 168 */
        "__finally", /* 169 */
        ".candidate", /* 170 */
        "const/volatile", /* 171 */
        "<end Parse>", /* 172 */
        "symbol", /* 173 */
        "__novtordisp", /* 174 */
        "asm", /* 175 */
        "catch", /* 176 */
        "template", /* 177 */
        "throw", /* 178 */
        "try", /* 179 */
        "__builtin_alignof", /* 180 */
        "__leave", /* 181 */
        "<constant tree>", /* 182 */
        "inheritance-specifier", /* 183 */
        "__single_inheritance", /* 184 */
        "__multiple_inheritance", /* 185 */
        "__virtual_inheritance", /* 186 */
        "Base/member initializer destructor list", /* 187 */
        "List of destructors in default arg tree", /* 188 */
        "List of keys in default arg tree", /* 189 */
        "__sysapi", /* 190 */
        "__thiscall", /* 191 */
        "__declspec", /* 192 */
        "__unaligned", /* 193 */
        "__unaligned", /* 194 */
        "'<L_TEMPLATEARGS>'", /* 195 */
        "'template<'", /* 196 */
        ">", /* 197 */
        "'<L_ALIGN>'", /* 198 */
        "'<L_PUSHSTATE>'", /* 199 */
        "'<L_POPSTATE>'", /* 200 */
        "'<L_DTORACTION>'", /* 201 */
        "'<L_TEMPLATEDEFN>'", /* 202 */
        "'<L_EHSTATESLIST>'", /* 203 */
        "default argument", /* 204 */
        "'Class MF'", /* 205 */
        "static_cast", /* 206 */
        "'<L_FPSYM_T>'", /* 207 */
        "'<L_TEMPLATEHEADER>'", /* 208 */
        "'<L_TEMPLATEDECL>'", /* 209 */
        "#", /* 210 */
        "$", /* 211 */
        "__int8", /* 212 */
        "__int16", /* 213 */
        "__int32", /* 214 */
        "__int64", /* 215 */
        "__int128", /* 216 */
        "wchar_t", /* 217 */
        "__builtin_isfloat", /* 218 */
        "inline function header", /* 219 */
        "reinterpret_cast", /* 220 */
        "pascal", /* 221 */
        "__pascal", /* 222 */
        "resolved identifier", /* 223 */
        "alignment cast", /* 224 */
        "namespace", /* 225 */
        "using", /* 226 */
        "const_cast", /* 227 */
        "dynamic_cast", /* 228 */
        "mutable", /* 229 */
        "explicit", /* 230 */
        "bool", /* 231 */
        "true", /* 232 */
        "false", /* 233 */
        "<template-id>", /* 234 */
        "<L_GENERICTYPE>", /* 235 */
        "typename", /* 236 */
        "__ptr64", /* 237 */
        "<L_PROPERTY>", /* 238 */
        "__uuidof", /* 239 */
        "<position>\n", /* 240 */
        "'<L_SUBSTREAM>'", /* 241 */
        "CompileTime directive", /* 242 */
        "Disabled keyword", /* 243 */
        "End of Token Stream", /* 244 */
        NULL, /* 245 */
    };
extern "C" CompilerNameEntry *__cdecl operator_identifier(unsigned token)
{

    token &= 0xff;
    if (token == 65) return find_or_create_identifier("[]");
    if (token == 118) return find_or_create_identifier("()");
#ifdef RECON_NATIVE_TEST
    if (token >= 246 || !compiler_token_spellings[token])
        recon_unsupported("operator_identifier: unmodeled token spelling");
#endif
    return find_or_create_identifier(compiler_token_spellings[token]);
}
