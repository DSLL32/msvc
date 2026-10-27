
Town::Town(char *nm){
    ZZ_INIT(Town);
    char *n=util.strAlloc(nm);
    name.add(this,n);
    route=NULL;
    best=INF;
}

Hwy::Hwy(int a, int b){
    ZZ_INIT(Hwy);
    id=a; dist=b;
}

Root *root; 

#include "zzfunc.c"
// --------------------------------------------------------
int zz_hashCmp_hash(char *t1,char *t2){
    return(strcmp(name.fwd((Town *)t1), name.fwd((Town *)t2)));
}

int zz_hashInd_hash(char *t,int size){
    int ZZhashStr(char *,int);
    return(ZZhashStr(name.fwd((Town *)t),size));
}
