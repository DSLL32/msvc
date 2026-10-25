# Inline-assembler grammar recovered from independently checked C1XX original LR kernels.
# Production order retains original rule identities; empty rows are semantic markers.
%start assembly_unit
# MASM expression precedence, from low to high; all binary forms associate left.
%left AS_OR AS_XOR
%left AS_AND
%left AS_EQ AS_GE AS_GT AS_LE AS_LT AS_NE
%left AS_PLUS AS_MINUS
%left AS_MULTIPLY AS_DIVIDE AS_MOD AS_SHL AS_SHR
%left AS_PTR
%left AS_DOT
%token AS_END 0
%token AS_EQ 1
%token AS_GE 2
%token AS_GT 3
%token AS_LE 4
%token AS_LT 5
%token AS_NE 6
%token AS_AND 7
%token AS_MOD 8
%token AS_NOT 9
%token AS_OR 10
%token AS_SHL 11
%token AS_SHR 12
%token AS_XOR 13
%token AS_HIGH 14
%token AS_LENGTH 15
%token AS_LOW 16
%token AS_OFFSET 17
%token AS_PTR 18
%token AS_SEG 19
%token AS_SIZE 20
%token AS_TYPE 21
%token AS_PLUS 22
%token AS_MINUS 23
%token AS_MULTIPLY 24
%token AS_DIVIDE 25
%token AS_COLON 26
%token AS_COMMA 27
%token AS_LEFT_BRACKET 28
%token AS_RIGHT_BRACKET 29
%token AS_LEFT_PAREN 30
%token AS_RIGHT_PAREN 31
%token AS_LEFT_BRACE 32
%token AS_SEMICOLON 33
%token AS_NEWLINE 34
%token AS_DOT 35
%token AS_END_OF_FILE 36
%token AS_BAD 37
%token AS_MAPPED 38
%token AS_REGISTER 39
%token AS_ASM_DATA_TYPE 40
%token AS_ASM_CODE_TYPE 41
%token AS_C_DATA_TYPE 42
%token AS_TYPEDEF_NAME 43
%token AS_IDENTIFIER 44
%token AS_CONSTANT 45
%token AS_ENUM_MEMBER 46
%token AS_PREFIX 47
%token AS_OPCODE 48
%token AS_DIRECTIVE 49
%token AS_ERROR 256
%nonterminal directive_value 1
%nonterminal expression 2
%nonterminal primary 3
%nonterminal indexed_primary 4
%nonterminal assembly_unit 5
%nonterminal assembly_line 6
%nonterminal statement 7
%nonterminal optional_open_brace 8
%nonterminal optional_label 9
%nonterminal instruction 10
%nonterminal optional_dot 11
%nonterminal optional_prefixes 12
%nonterminal begin_instruction 13
%nonterminal optional_operands 14
%nonterminal prefixes 15
%nonterminal operand_list 16
%nonterminal operand 17
%nonterminal next_operand 18
%nonterminal begin_member_name 19
%nonterminal begin_member_type 20
%nonterminal begin_member_expression 21
%nonterminal begin_bracket 22
%nonterminal continue_bracket 23
assembly_unit : assembly_line ; # rule 1
assembly_unit : assembly_unit assembly_line ; # rule 2
assembly_line : statement AS_NEWLINE ; # rule 3
assembly_line : AS_ERROR AS_NEWLINE ; # rule 4
statement : optional_open_brace optional_label instruction ; # rule 5
optional_open_brace :  ; # rule 6
optional_open_brace : AS_LEFT_BRACE ; # rule 7
optional_label :  ; # rule 8
optional_label : AS_IDENTIFIER AS_COLON ; # rule 9
instruction :  ; # rule 10
instruction : optional_dot AS_DIRECTIVE directive_value ; # rule 11
begin_instruction :  ; # rule 12
instruction : optional_prefixes AS_OPCODE begin_instruction optional_operands ; # rule 13
optional_dot :  ; # rule 14
optional_dot : AS_DOT ; # rule 15
directive_value :  ; # rule 16
directive_value : AS_CONSTANT ; # rule 17
optional_prefixes :  ; # rule 18
optional_prefixes : prefixes ; # rule 19
prefixes : AS_PREFIX ; # rule 20
prefixes : prefixes AS_PREFIX ; # rule 21
optional_operands :  ; # rule 22
optional_operands : operand_list ; # rule 23
operand_list : operand ; # rule 24
next_operand :  ; # rule 25
operand_list : operand_list AS_COMMA next_operand operand ; # rule 26
operand : expression ; # rule 27
expression : expression AS_MULTIPLY expression ; # rule 28
expression : expression AS_DIVIDE expression ; # rule 29
expression : expression AS_OR expression ; # rule 30
expression : expression AS_XOR expression ; # rule 31
expression : expression AS_AND expression ; # rule 32
expression : expression AS_EQ expression ; # rule 33
expression : expression AS_GE expression ; # rule 34
expression : expression AS_GT expression ; # rule 35
expression : expression AS_LE expression ; # rule 36
expression : expression AS_LT expression ; # rule 37
expression : expression AS_NE expression ; # rule 38
expression : expression AS_SHL expression ; # rule 39
expression : expression AS_SHR expression ; # rule 40
expression : expression AS_MOD expression ; # rule 41
expression : expression AS_PLUS expression ; # rule 42
expression : expression AS_MINUS expression ; # rule 43
expression : expression AS_PTR expression ; # rule 44
begin_member_name :  ; # rule 45
expression : primary AS_IDENTIFIER AS_DOT begin_member_name expression ; # rule 46
begin_member_type :  ; # rule 47
expression : primary AS_TYPEDEF_NAME AS_DOT begin_member_type expression ; # rule 48
begin_member_expression :  ; # rule 49
expression : expression AS_DOT begin_member_expression expression ; # rule 50
expression : primary ; # rule 51
primary : AS_LEFT_PAREN expression AS_RIGHT_PAREN ; # rule 52
primary : AS_NOT primary ; # rule 53
primary : AS_PLUS primary ; # rule 54
primary : AS_MINUS primary ; # rule 55
primary : AS_HIGH primary ; # rule 56
primary : AS_LOW primary ; # rule 57
primary : AS_REGISTER AS_COLON primary ; # rule 58
primary : AS_TYPE AS_TYPEDEF_NAME ; # rule 59
primary : AS_TYPE AS_C_DATA_TYPE ; # rule 60
primary : AS_TYPE AS_ASM_DATA_TYPE ; # rule 61
primary : AS_TYPE AS_IDENTIFIER ; # rule 62
primary : AS_SIZE AS_IDENTIFIER ; # rule 63
primary : AS_LENGTH AS_IDENTIFIER ; # rule 64
primary : AS_CONSTANT ; # rule 65
primary : AS_ENUM_MEMBER ; # rule 66
primary : AS_IDENTIFIER ; # rule 67
primary : AS_SEG AS_IDENTIFIER ; # rule 68
primary : AS_OFFSET primary ; # rule 69
primary : AS_REGISTER ; # rule 70
primary : AS_REGISTER AS_LEFT_PAREN AS_CONSTANT AS_RIGHT_PAREN ; # rule 71
primary : AS_ASM_DATA_TYPE AS_PTR primary ; # rule 72
primary : AS_TYPEDEF_NAME AS_PTR primary ; # rule 73
primary : AS_C_DATA_TYPE AS_PTR primary ; # rule 74
primary : AS_ASM_CODE_TYPE AS_PTR primary ; # rule 75
primary : AS_C_DATA_TYPE AS_IDENTIFIER ; # rule 76
primary : AS_ASM_CODE_TYPE AS_IDENTIFIER ; # rule 77
primary : indexed_primary AS_RIGHT_BRACKET ; # rule 78
begin_bracket :  ; # rule 79
primary : AS_LEFT_BRACKET begin_bracket expression AS_RIGHT_BRACKET ; # rule 80
continue_bracket :  ; # rule 81
indexed_primary : primary AS_LEFT_BRACKET continue_bracket expression ; # rule 82
