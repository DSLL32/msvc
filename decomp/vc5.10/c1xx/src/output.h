#ifndef C1XX_OUTPUT_H
#define C1XX_OUTPUT_H
#include <stdio.h>
#include "c1xx.h"
#include <recon.h>

typedef void (__cdecl *OutputRelocation)(void *, void *, void *);

/* Encountered fields only. The original intervening storage has no native
 * placeholder; later consumers must be modeled before they can run. */
typedef struct OutputBuffer {
    unsigned char *data;
    unsigned char *cursor;
    unsigned capacity;
    OutputRelocation relocation;
    void *relocation_context;
    FILE *file;
    int ok;
    int descriptor;
    unsigned char mode;
} OutputBuffer;

enum { FRONTEND_OUTPUT_BUFFERS = 4 };

void initialize_output_buffer_01(void);
void destroy_output_buffer_01(void);
void initialize_output_buffer_02(void);
void destroy_output_buffer_02(void);
void initialize_output_buffer_03(void);
void destroy_output_buffer_03(void);
void initialize_output_buffer_04(void);
void destroy_output_buffer_04(void);
#endif
