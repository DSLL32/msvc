#ifndef C1XX_H
#define C1XX_H
extern int argument_count;
extern char **argument_vector;
extern char *fatal_error_label;
char *message_text(unsigned code);
void parse_options(int argc, char **argv);
int guarded_compile(void);
int compile_translation_unit(void);
void interrupt_compile(int signal_number);
#endif
