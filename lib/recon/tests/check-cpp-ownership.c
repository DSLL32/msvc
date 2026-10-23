#include <assert.h>
#include <sys/resource.h>
#include <recon.h>
int main(int argc,char **argv)
{
    void *p,*q;ReconCppAllocation *record,*previous;
    unsigned n;uint64_t generation=0;
    struct rlimit limit;
    if(argc>1){
        if(!strcmp(argv[1],"null"))crt_operator_delete(NULL);
        if(!strcmp(argv[1],"foreign"))crt_operator_delete(malloc(16));
        p=crt_operator_new(32);record=recon_cpp_allocations;
        if(!strcmp(argv[1],"interior"))crt_operator_delete((char*)p+1);
        if(!strcmp(argv[1],"repeat")){crt_operator_delete(p);crt_operator_delete(p);}
        if(!strcmp(argv[1],"ordinaryfree")){free(p);assert(!record->live);crt_operator_delete(p);}
        if(!strcmp(argv[1],"realloc")){q=realloc(p,64);assert(q && !record->live);crt_operator_delete(q);}
        if(!strcmp(argv[1],"realloczero"))realloc(p,0);
        if(!strcmp(argv[1],"failure")){
            limit.rlim_cur=limit.rlim_max=32*1024*1024;assert(!setrlimit(RLIMIT_AS,&limit));
            assert(!realloc(p,256*1024*1024) && record->live);
            crt_operator_delete(p);assert(!record->live);return 0;
        }
        return 3;
    }
    for(n=0;n<100;++n){
        previous=recon_cpp_allocations;
        errno=111;SetLastError(0x12345678);p=crt_operator_new(16);
        record=recon_cpp_allocations;
        assert(record->live && record->base==p && record->requested==16 && record->rounded==16);
        assert(record->generation>generation);generation=record->generation;
        assert(!previous || (!previous->live && previous->generation<record->generation));
        assert(errno==111 && GetLastError()==0x12345678);
        if(n&1)free(p);else crt_operator_delete(p);
        assert(!record->live && errno==111 && GetLastError()==0x12345678);
    }
    p=malloc(8);memset(p,0x5a,8);q=realloc(p,32);assert(q && ((char*)q)[0]==0x5a);free(q);
    return 0;
}
