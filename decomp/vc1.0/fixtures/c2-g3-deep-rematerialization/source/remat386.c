/* Caller supplies at least96 unsigned elements. Unsigned arithmetic wraps. */
extern unsigned outer_call(unsigned, unsigned, unsigned, unsigned);
extern unsigned inner_call(unsigned, unsigned, unsigned, unsigned);
extern unsigned final_call(unsigned, unsigned, unsigned, unsigned);
unsigned remat386(const unsigned *p, unsigned seed, unsigned count)
{
    const unsigned *a=p+3, *b=p+7, *c=p+11, *d=p+17;
    const unsigned *e=p+23, *f=p+31, *g=p+41, *h=p+53;
    unsigned k0=0x12345678U, k1=0x87654321U, k2=0x13579bdfU, k3=0x2468ace0U;
    unsigned x=seed+3, y=seed+5, z=seed+7, w=seed+11;
    unsigned j=seed+13, k=seed+17, m=seed+19, n=seed+23;
    unsigned i, v, q;
    for (i=0;i<count;++i) {
        q=i&15;
        v=outer_call(inner_call(x+k0,a[q]+y,(unsigned)(c+q),k1),
            inner_call(z+k2,b[q]+w,(unsigned)(d+q),k3),
            final_call(j+k0,e[q]+k,(unsigned)(g+q),k2),
            final_call(m+k1,f[q]+n,(unsigned)(h+q),k3));
        x+=a[q]+v; y^=b[q]+x; z+=c[q]+y; w^=d[q]+z;
        j+=e[q]+w; k^=f[q]+j; m+=g[q]+k; n^=h[q]+m;
        seed+=outer_call(x+z+j+m,y+w+k+n,(unsigned)(a+q),k0+k3);
    }
    return final_call(x+y+z+w,j+k+m+n,seed+k1+k2,(unsigned)(h+15));
}
