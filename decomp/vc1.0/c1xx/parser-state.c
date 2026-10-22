/* Shared yacc state and its observed macro-workspace boundary aliases. */
#include "parser.h"

ParserStackStorage parser_stacks; /* original 0x94e44..0x956e4 */
ParserValue parser_value; /* original 0x8c1f0..0x8c1f8 */
ParserValue current_token_value; /* original 0x8c150..0x8c158 */
int parser_lookahead = -1; /* original 0x9eb9c */
int parser_error_count; /* original 0x9eba0 */
short parser_recovery_count; /* original 0x9eba4 */

ClassContext class_contexts[16]; /* original 0x8c1fc..0x8c3fc */
unsigned char expression_storage[2052]; /* original 0x8c408..0x8cc0c */
unsigned char *text_work_cursor = expression_storage; /* original 0x9fc48 */
int initializer_list_started; /* original 0xa0a24 */
unsigned short declarator_nesting; /* original 0x91894 */
/* A parenthesized-declaration lookahead also addresses the preceding entry. */
DeclarationType declaration_spec_storage[61]; /* original 0x948fc..0x94ae4 */
unsigned enumerator_value = (unsigned)-1; /* original 0x9fc0c */
CType *enumerator_context; /* original 0x9f1d8 */
void *based_symbol; /* original 0x91164 */
unsigned array_bound; /* original 0x97bbc */
unsigned new_array_bound; /* original 0x97bc4 */
unsigned char member_lookup_operator; /* original 0x97bb4 */
CppSymbol *member_lookup_context; /* original 0x97bb8 */
LabelStack *control_labels, *break_labels, *continue_labels;
LabelStack *switch_labels, *default_labels;
/* The five original heads are 0x9155c, 0x91560, 0x91564, 0x91568, 0x9156c. */
int label_stack_count; /* original 0x91558 */
IrToken *default_label; /* original 0x91570 */
