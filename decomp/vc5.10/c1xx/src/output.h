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
    char filename[260]; /* original +1c..11f, then descriptor at+120 */
    int descriptor;
#ifdef __cplusplus
    int RECON_THISCALL Close(void);
    int RECON_THISCALL Open(const char *, const char *, const char *);
    int RECON_THISCALL Grow(int bytes);
    void RECON_THISCALL Flush(void);
    void RECON_THISCALL FlushAlternate(void); /*93b58*/
    int RECON_THISCALL Ensure(int bytes);
    int RECON_THISCALL Prepare(int bytes);
    void RECON_THISCALL WriteIndex(unsigned value);
    int RECON_THISCALL WriteCompact(unsigned value);
    int RECON_THISCALL WriteBytes(const void *data, unsigned bytes);
    void RECON_THISCALL WriteUShort(unsigned value);
    void RECON_THISCALL WriteEncodedShort(unsigned value); /*6e61*/
    void RECON_THISCALL WriteStringData(const void *data);
    void RECON_THISCALL WriteStringTag(unsigned value);
    void RECON_THISCALL WriteCString(const char *text);
    void RECON_THISCALL MarkLocation(unsigned index, unsigned kind);
    void RECON_THISCALL MarkGlobal(unsigned index);
#endif
} OutputBuffer;

enum { FRONTEND_OUTPUT_BUFFERS = 8 };

#ifdef __cplusplus
extern "C" {
#endif
void initialize_output_buffer_01(void);
void destroy_output_buffer_01(void);
void initialize_output_buffer_02(void);
void destroy_output_buffer_02(void);
void initialize_output_buffer_03(void);
void destroy_output_buffer_03(void);
void initialize_output_buffer_04(void);
void destroy_output_buffer_04(void);
void initialize_output_buffer_05(void);
void destroy_output_buffer_05(void);
void initialize_output_buffer_06(void);
void destroy_output_buffer_06(void);
void initialize_output_buffer_07(void);
void destroy_output_buffer_07(void);
void initialize_output_buffer_08(void);
void destroy_output_buffer_08(void);
#ifdef __cplusplus
}
#endif
#endif
