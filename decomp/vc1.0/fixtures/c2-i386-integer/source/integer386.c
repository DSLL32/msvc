extern int integer_call(int,int,int,int);
int integer386(const int *p, int n, int seed)
{
    int i, j, a=seed, b=seed+3, c=seed-7, d=seed^85;
    int e=seed+11, f=seed-13, g=seed*3, h=seed+17;
    for(i=0;i<n;++i) {
        for(j=0;j<3;++j) {
            int v=p[i*3+j];
            a+=(v^b); b=(b<<1)+(v&31); c^=(v+d);
            d+=(a>>3)-v; e^=c+v; f+=(d|v);
            g=(g<<2)-g+v; h^=(f>>2)+e;
        }
        if(a&1) c+=integer_call(a,b,c,d); else f-=g+h;
    }
    return a+b+c+d+e+f+g+h;
}
