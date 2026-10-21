/* Named traps for functions not reconstructed yet. */
#include "cl.h"
/* 00018f2b */
void usage(void) { __asm { int 3 } }
/* 0001a757 */
void fatal(int code, ...) { __asm { int 3 } }
/* 0001ab77 */
void warning(int code, ...) { __asm { int 3 } }
/* 0001a7b3 */
void driver_exit(int status) { __asm { int 3 } }
/* 0001a3c7 */
char *append_string(char *dst, char *src) { __asm { int 3 }; return 0; }
/* 00018a85 */
void prepare_inputs(void) { __asm { int 3 } }
/* 0001b007 */
void prepare_execution(void) { __asm { int 3 } }
/* 000193cd */
void run_driver(Driver *d) { __asm { int 3 } }
/* 0001842b */
void read_response_file(char *name) { __asm { int 3 } }
/* 00017557 */
int classify_input(char *name) { __asm { int 3 }; return 0; }
/* 000175dd */
void add_input(char *name, int kind) { __asm { int 3 } }
/* 0001ad08 */
void ide_message(int code, ...) { __asm { int 3 } }
/* 0001afcc */
BOOL WINAPI control_handler(DWORD code) { __asm { int 3 }; return 0; }
/* 00017f2a */
void warn_override(int code, Item *old, Item *item) { __asm { int 3 } }
/* 00019014 */
int option_callback_19014(char **args, int *index) { __asm { int 3 }; return 0; }
/* 00019198 */
int option_callback_19198(char **args, int *index) { __asm { int 3 }; return 0; }
/* 0001920c */
int option_callback_1920c(char **args, int *index) { __asm { int 3 }; return 0; }
/* 00019228 */
int option_callback_19228(char **args, int *index) { __asm { int 3 }; return 0; }
/* 000176d4 */
Item *copy_item(Item *item) { __asm { int 3 }; return 0; }
/* 0001773c */
Item *detach_item(Item **head, Item *item) { __asm { int 3 }; return 0; }
/* 00017010 */
void driver_switch_17010(Item *item) { __asm { int 3 } }
/* 000170a0 */
void driver_switch_170a0(Item *item) { __asm { int 3 } }
/* 0001713c */
void driver_switch_1713c(Item *item) { __asm { int 3 } }
/* 0001719c */
void driver_switch_1719c(Item *item) { __asm { int 3 } }
/* 00017368 */
void driver_switch_17368(Item *item) { __asm { int 3 } }
/* 000173ac */
void driver_switch_173ac(Item *item) { __asm { int 3 } }
/* 0001a2fc */
char *output_stem(char *out, char *name) { __asm { int 3 }; return 0; }
/* 00019f3c */
char *map_filename(char *out, char *name) { __asm { int 3 }; return 0; }
