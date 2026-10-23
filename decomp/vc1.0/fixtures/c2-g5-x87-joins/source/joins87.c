/* Caller supplies eight finite doubles; divisions require nonzero denominators.
   Keep noncommutative values live through branches and loop backedges. */
extern double sin(double);
extern double cos(double);
extern double sqrt(double);

double joined87(const double *p, double bias, unsigned mask)
{
    double a=p[0]+bias, b=p[1]-bias, c=p[2]+1.25, d=p[3]-0.75;
    double e=p[4]+2.5, f=p[5]-3.5, g=p[6]+4.25, h=p[7]-5.25;
    double u, v, w;
    if (mask & 1) { u=(a-b)/(c+d); v=(e-f)/(g+h); }
    else { u=(g-h)/(e+f); v=(c-d)/(a+b); }
    if (a*c > b*d) { w=u-v; a=a-e; c=c-g; }
    else { w=v-u; b=b-f; d=d-h; }
    if (mask & 2) return (a*d-b*c)/(e*h-f*g)+w+sin(u)*cos(v);
    return (e*b-f*a)/(g*d-h*c)-w+sqrt(u*u+v*v+1.0);
}

double rotating87(const double *p, int count, double bias)
{
    double a=p[0]+bias, b=p[1]-bias, c=p[2]+1.0, d=p[3]-1.0;
    double e=p[4]+2.0, f=p[5]-2.0, g=p[6]+3.0, h=p[7]-3.0;
    double t, u, v;
    int i;
    for (i=0; i<count; ++i) {
        t=(a-b)*(c+d); u=(e-f)/(g+h); v=(g-a)/(b+f);
        if (t > u) { a=c-u; c=e-v; e=g-t; g=t-u; }
        else { b=d+v; d=f+t; f=h+u; h=u-v; }
        if (i & 1) { t=a; a=h; h=d; d=t; }
        else { t=b; b=g; g=e; e=t; }
        bias += (a*c-b*d)/(e+f+g+h);
    }
    return bias+(a-f)*(h-c)-(b-e)/(g+d);
}
