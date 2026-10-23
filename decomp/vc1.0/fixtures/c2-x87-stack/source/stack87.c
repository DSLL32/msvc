extern double sin(double);
extern double cos(double);
extern double sqrt(double);
extern double atan2(double,double);
extern double pow(double,double);
extern double floating_sink(double,double,double,double,double,double,double,double);
double stack87(const double *p, double x)
{
    double a=p[0]+x, b=p[1]-x, c=p[2]*x, d=p[3]/(x+1.0);
    double e=p[4]+0.25, f=p[5]-0.5, g=p[6]*1.5, h=p[7]+2.0;
    double u=sin(a+b)+cos(c-d);
    double v=atan2(e+f,g-h)+pow(a+c,b+d);
    return floating_sink(a,b,c,d,e,f,g,h)+u+v+
           sqrt((a*e+b*f)+(c*g+d*h));
}
