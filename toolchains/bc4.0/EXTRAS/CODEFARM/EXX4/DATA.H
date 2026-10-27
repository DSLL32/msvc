#include "zzincl.h"
#define INF 0X7FFF

class Root {
  ZZ_EXT_Root
public:
  Root(){ZZ_INIT(Root);}
};

class Hwy {
  int id;
  int dist;
  ZZ_EXT_Hwy
public:
  Hwy(){ZZ_INIT(Hwy);};
  Hwy(int a,int b);
  int getDist(void){return dist;}
  int getID(void){return id;}
};

class Town {
  ZZ_EXT_Town
public:
  Town(){ZZ_INIT(Town);};
  Town(char *nm);
  int best;    // temporary
  Hwy *route; // temporary
};

ZZ_HYPER_SINGLE_GRAPH(netw,Town,Hwy);
ZZ_HYPER_NAME(name,Town);
ZZ_HYPER_HASH(hash,Root,Town);
ZZ_HYPER_DOUBLE_COLLECT(wave,Root,Town);
ZZ_HYPER_UTILITIES(util);
