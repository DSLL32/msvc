/* Named unvisited assembler callees; reconstructed individually at traps. */
#include "asm.h"
#include <recon.h>

/* Original 0x4c93b. */
void __stdcall report_asm_expected_token(int actual, int expected) { __asm { int 3 } }
/* Original 0x4c98c. */
ParserValue __stdcall asm_recovery_value(int token) { ParserValue value; __asm { int 3 } return value; }
/* Original 0x4c9f5. */
void __stdcall report_asm_parser_error(char *text) { __asm { int 3 } }
/* Original 0x4dcf4. */

/* Original 0x4d6ce. */

/* Original 0x4e449. */
void __stdcall set_asm_directive(short directive, unsigned value) { __asm { int 3 } }
/* Original 0x4de19. */

/* Original 0x4fdb1. */

/* Original 0x4e274. */

/* Original 0x5004f. */

/* Original 0x4fdfc. */

/* Original 0x5010f. */

/* Original 0x4defc. */
void __stdcall set_asm_address_operator(short operation) { __asm { int 3 } }
/* Original 0x4e324. */
char * asm_error_context(void) { __asm { int 3 } return 0; }
/* Original 0x4e290. */

/* Original 0x50260. */
AsmExpression * __stdcall evaluate_asm_unary(unsigned short operation, AsmExpression *value) { __asm { int 3 } return 0; }
/* Original 0x50541. */
AsmExpression * __stdcall apply_asm_segment(short segment, AsmExpression *value) { __asm { int 3 } return 0; }
/* Original 0x4df51. */

/* Original 0x4df9f. */

/* Original 0x50308. */

/* Original 0x50342. */

/* Original 0x505ba. */
AsmExpression * __stdcall make_asm_segment_value(AsmExpression *value) { __asm { int 3 } return 0; }
/* Original 0x5061c. */

/* Original 0x507e1. */

/* Original 0x508c8. */

/* Original 0x4de77. */

/* Original 0x4dfd8. */

/* Original 0x4e151. */


/* Original 0x2caa0: assembler labels use case-insensitive lookup. */


/* Original 0x4d7a1. */

/* Original 0x4dc65. */
void copy_asm_comment(void) { __asm { int 3 } }

/* Original 0x4d98b. */

/* Original 0x4d9f6. */


/* Original 0x35780. */


/* Original 0x50695. */


/* Original 0x4f475. */

/* Original 0x4f6ab. */

/* Original 0x4ec52. */

/* Original 0x4f1ee. */


/* Original 0x4f62b. */
int __stdcall fits_asm_integer(unsigned char bits, unsigned value) { __asm { int 3 } return 0; }

/* Original 0x4e657. */

/* Original 0x4e991. */

/* Original 0x4eb41. */

/* Original 0x4e97c. */
void __stdcall swap_asm_pointer_slots(void **first, void **second) { __asm { int 3 } }

/* Original 0x4e4f8. */

/* Original 0x4f689. */
int __stdcall asm_operand_accepts_size(AsmOperand *operand, unsigned char size) { __asm { int 3 } return 0; }
/* Original 0x4f64e. */
int __stdcall asm_operand_has_type_flags(AsmOperand *operand, unsigned short flags) { __asm { int 3 } return 0; }

/* Original 0x4eb15. */
void __stdcall report_asm_operand_error(unsigned code, int operand) { __asm { int 3 } }

/* Original 0x4f12f. */


/* Original0x4da61. */
unsigned __stdcall parse_asm_binary_digits(char *digits) { __asm { int 3 } return 0; }

/* Original0x3f3d0. */

/* Original0x4e047 and0x4e06c: member search paths. */



/* Original0x50019. */
