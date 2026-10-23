#include "api.h"
#pragma hdrstop
int main(void)
{
    Calibration record = { -3, 4, 10 };
    Calibrate callback = calibrate_record;
    int first = callback(&record, 6);
    record.bias = CALIBRATION_BIAS;
    record.scale = 2;
    return first != 36 || calibrate_record(&record, -2) != 38;
}
