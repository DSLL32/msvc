/* Windows and OS/2 DIB headers and their RGB/RGBTRIPLE palettes. */
#include "resource-data.h"
#include <recon.h>
#pragma pack(2)
struct BitmapFileHeader {
    WORD type;
    DWORD size;
    WORD reserved1, reserved2;
    DWORD bits_offset;
};
#pragma pack()

/* Original 0x0100a9f0..0x0100ab53. */
unsigned int __cdecl read_bitmap_file_header(void)
{
    FILE *file = resource_stream(0);
    struct BitmapFileHeader header;
    union {
        BITMAPINFOHEADER windows;
        BITMAPCOREHEADER os2;
    } bitmap;
    DWORD palette_size, bits, size;
    BYTE *palette;
    read_file(file, &header, sizeof(header));
    if (header.type != 0x4d42)
        resource_data_error("Bitmap file %s is not in 3.00 format.", current_token.spelling);
    read_file(file, &bitmap, 12);
    if (bitmap.windows.biSize == 40) {
        read_file(file, (BYTE *)&bitmap + 12, 28);
        bits = (DWORD)bitmap.windows.biPlanes * bitmap.windows.biBitCount;
        if (bitmap.windows.biClrUsed)
            palette_size = bitmap.windows.biClrUsed * 4;
        else if (bits == 24)
            palette_size = 0;
        else
            palette_size = (1UL << (bits & 31)) * 4;
    } else if (bitmap.os2.bcSize == 12) {
        bits = (DWORD)bitmap.os2.bcPlanes * bitmap.os2.bcBitCount;
        palette_size = bits == 24 ? 0 : 3UL << (bits & 31);
    } else
        resource_data_error("Unknown DIB header format", "");
    put_bytes(&bitmap, (WORD)bitmap.windows.biSize);
    if (palette_size) {
        palette = resource_alloc(palette_size);
        read_file(file, palette, palette_size);
        put_bytes(palette, (WORD)palette_size);
        resource_free(palette);
    }
    size = seek_file(file, 0, SEEK_END) - header.bits_offset + palette_size + bitmap.windows.biSize;
    seek_file(file, header.bits_offset, SEEK_SET);
    return size;
}
