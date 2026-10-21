#ifndef C1_H
#define C1_H

extern int driver_argc;
extern char **driver_argv;
extern char *fatal_error_label;
extern int skip_fault_protection;

char *message_text(unsigned code);
void compiler_signal(int signal_number);
void parse_options(int argc, char **argv);
int guarded_compile(void);
int compile_translation_unit(void);

#endif
