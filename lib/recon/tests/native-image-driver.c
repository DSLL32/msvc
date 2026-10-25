#include <assert.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv)
{
    void *app,*fixture,*anchor,*case_anchor,*data,*callback;
    void (*bind)(const void *,unsigned),(*test)(const char *,void *,void *);
    assert(argc==4);app=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);fixture=dlopen(argv[2],RTLD_NOW|RTLD_LOCAL);assert(app && fixture);
    bind=dlsym(app,"recon_register_native_image");test=dlsym(app,"image_test");anchor=dlsym(app,"image_anchor");
    case_anchor=dlsym(fixture,"case_anchor");data=dlsym(fixture,"case_data");callback=dlsym(fixture,"case_callback");assert(bind && test && anchor && case_anchor && data && callback);
    if(!strcmp(argv[3],"role")){bind(anchor,99);return 90;}
    if(!strcmp(argv[3],"order")){bind(case_anchor,2);return 90;}
    if(!strcmp(argv[3],"heap-anchor")){bind(malloc(1),1);return 90;}
    if(strcmp(argv[3],"unbound-app")){bind(anchor,1);bind(anchor,1);}
    if(!strcmp(argv[3],"replace")){bind(case_anchor,1);return 90;}
    if(!strcmp(argv[3],"case") || !strcmp(argv[3],"foreign-data"))bind(case_anchor,2);
    test(argv[3],data,callback);return 0;
}
