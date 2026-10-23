#include "cvtres.h"
#include <recon.h>

int __stdcall convert_ce_image(int ce, FILE *input, FILE *output,
                               ResourceEntry *entry, int type)
{
    BITMAPINFOHEADER *original, *converted;
    DWORD *palette, *old_palette;
    BYTE *source, *destination, *pixels;
    DWORD input_stride, output_stride, mask_stride = 0;
    DWORD rows, palette_count, output_size, header_size, remaining, count, i;
    unsigned int old_bits, new_bits;
    long position;

    original = allocate_block(entry->header->data_size);
    if (!original) return 0;
    position = tell_file(input);
    if (read_bytes(input, original, entry->header->data_size)) {
        free_block(original);
        seek_file(input, position, SEEK_SET);
        return 0;
    }
    old_bits = original->biBitCount;
    input_stride = (original->biWidth * old_bits + 31) >> 5;
    rows = original->biHeight < 0 ? -original->biHeight : original->biHeight;
    if (type == 3) rows /= 2;
    if (ce) {
        if (old_bits == 1 || old_bits == 2) {
            free_block(original);
            seek_file(input, position, SEEK_SET);
            return 0;
        }
        new_bits = 2;
        header_size = 56;
        output_stride = (original->biWidth * 2 + 31) >> 5;
    } else {
        if (old_bits != 2) {
            free_block(original);
            seek_file(input, position, SEEK_SET);
            return 0;
        }
        new_bits = 4;
        header_size = 104;
        output_stride = (original->biWidth * 4 + 31) >> 5;
    }
    if (type == 3) mask_stride = (original->biWidth + 31) >> 5;
    output_size = (output_stride + mask_stride) * rows * 4 + header_size;
    converted = allocate_block(output_size + (ce ? 0 : 4));
    palette = (DWORD *)((BYTE *)converted + 40);
    printf(load_message(5007), old_bits, resource_type_names[type],
           new_bits, resource_type_names[type]);
    palette[0] = 0;
    palette[1] = 0x808080;
    palette[2] = 0xc0c0c0;
    palette[3] = 0xffffff;
    palette_count = ce ? (old_bits == 24 ? 0 : 1U << old_bits) : 4;
    old_palette = (DWORD *)((BYTE *)original + 40);
    if (ce) {
        for (i = 0; i < palette_count; ++i)
            old_palette[i] = color_to_gray(old_palette[i]);
    } else {
        for (i = 4; i < 16; ++i) palette[i] = 0xffffff;
    }
    *converted = *original;
    converted->biBitCount = (WORD)new_bits;
    if (type == 3) converted->biSizeImage = (output_stride + mask_stride) * rows * 4;
    if (fwrite(converted, 40, 1, output) != 1) {
        fprintf(stderr, "Cannot write BITMAPINFOHEADER\r\n");
        return 0;
    }
    count = ce ? 4 : 16;
    if (fwrite(palette, 4, count, output) != count) {
        fprintf(stderr, "Cannot write color table\r\n");
        return 0;
    }
    pixels = (BYTE *)converted + header_size;
    destination = pixels;
    source = (BYTE *)original + 40 + palette_count * 4;
    convert_rows(old_palette, (WORD)old_bits, rows, input_stride, output_stride,
                 &source, &destination);
    if (type == 3) {
        remaining = mask_stride * rows * 4;
        while (remaining--) *destination++ = *source++;
    }
    entry->header->data_size = output_size;
    remaining = output_size - header_size;
    while (remaining) {
        count = remaining > 1024 ? 1024 : remaining;
        remaining -= count;
        write_bytes(output, pixels, count);
        pixels += count;
    }
    free_block(original);
    free_block(converted);
    return 1;
}
