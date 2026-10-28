#include <dlfcn.h>
int main(int argc,char **argv) {void *d=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);int(*run)(void);(void)argc;if(!d)return 91;run=(int(*)(void))dlsym(d,"main");return run?run():92;}
