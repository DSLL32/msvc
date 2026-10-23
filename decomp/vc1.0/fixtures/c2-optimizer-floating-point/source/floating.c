extern double floating_call(double);

double floating_point(double *destination, const float *source, int count, int bias)
{
    double sum = (double)bias + 0.25;
    double scale = 1.5;
    int index;
    for (index = 0; index < count; ++index) {
        double value = (double)source[index] * scale + (double)(index - bias);
        if (value < 0.0)
            value = -value / (scale + 0.5);
        else
            value = value * value - 0.125;
        destination[index] = value;
        sum += value;
        scale += 0.0625;
    }
    return floating_call(sum) + (double)(long)(sum * 0.5);
}
