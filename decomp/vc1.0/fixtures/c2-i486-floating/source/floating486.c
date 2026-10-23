extern double floating_call(double);
double floating486(double *out, const double *p, int count, double bias)
{
    double sum=bias, value, previous=0.0;
    int i;
    for(i=0;i<count;++i) {
        value=p[i]*1.25+bias;
        if(value<previous) value=previous-value;
        else if(value>sum) value=(value+sum)*0.5;
        else value=value*value-0.125;
        out[i]=value;
        previous=value;
        sum+=value;
    }
    return floating_call(sum)+previous;
}
