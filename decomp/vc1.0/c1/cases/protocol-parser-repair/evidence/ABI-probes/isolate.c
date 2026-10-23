typedef struct S {unsigned a,b;} S;
typedef struct SS {char a[3];char b;void *p;} SS;
typedef union A {unsigned words[2];void *p;double d;char b[8];} A;
typedef union B {S s;A a;} B;
typedef union C {S s;A a;unsigned short modifier;} C;
typedef union D {S s;A a;SS ss;} D;
typedef union E {A a;unsigned short modifier;} E;
typedef union F {unsigned words[2];unsigned short modifier;} F;
typedef union G {char b[8];unsigned short modifier;} G;
#define FN(T,N) T __stdcall N(int v) {T r;return r;}
FN(A,a) FN(B,b) FN(C,c) FN(D,d) FN(E,e) FN(F,f) FN(G,g)
