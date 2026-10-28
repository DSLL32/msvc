#include <stdio.h>
#include <unistd.h>
#include <recon.h>
int consumer_print(const char *text) {return printf("%s",text);}
void consumer_quick_exit(int code) {_exit(code);}
int (*consumer_binding(void))(const char *,...) {return printf;}
