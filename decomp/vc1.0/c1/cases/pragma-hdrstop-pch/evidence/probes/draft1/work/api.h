#ifndef CALIBRATION_API_H
#define CALIBRATION_API_H
#define CALIBRATION_GAIN(x) ((x) * 3 + 5)
#define CALIBRATION_BIAS 7
typedef struct Calibration {
    signed char bias;
    unsigned short scale;
    unsigned flags;
} Calibration;
typedef int (__cdecl *Calibrate)(const Calibration *, int);
int __cdecl calibrate_record(const Calibration *, int);
#endif
