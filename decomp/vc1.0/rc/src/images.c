/* ICO/CUR directory splitting into individual images and group resources. */
#include "resource-data.h"

#pragma pack(2)
struct ImageDirectory {
    WORD reserved, type, count;
};
struct ImageDirectoryEntry {
    BYTE width, height, colors, reserved;
    WORD planes_or_hotspot_x, bits_or_hotspot_y;
    DWORD bytes, offset;
};
struct ImageGroupEntry {
    union {
        struct { BYTE width, height, colors, reserved; } icon;
        struct { WORD width, height; } cursor;
    } dimensions;
    WORD planes, bits;
    DWORD bytes;
};
#pragma pack()
static WORD next_image_ordinal = 1;

/* Original 0x0100b140..0x0100b3ca. Image records use the global language and
 * version defaults; the group's own metadata remains on the parent record. */
void __stdcall split_icon_file(struct ResourceType *type, struct ResourceInfo *resource, int kind)
{
    struct ImageDirectory directory;
    struct ImageDirectoryEntry entry;
    struct ImageGroupEntry group;
    struct ResourceInfo *image;
    struct ResourceType *image_type;
    BITMAPINFOHEADER bitmap;
    WORD hotspot[2];
    DWORD zero = 0;
    unsigned int index, prefix, remainder;
    long directory_position;
    read_resource_file();
    read_file(resource_data_file, &directory, sizeof(directory));
    if (directory.reserved || (directory.type != 1 && directory.type != 2))
        resource_data_error("Resource file %s is not in 3.00 format.", current_token.spelling);
    put_bytes(&directory, sizeof(directory));
    for (index = 0; index < directory.count; ++index) {
        read_file(resource_data_file, &entry, sizeof(entry));
        directory_position = seek_file(resource_data_file, 0, SEEK_CUR);
        seek_file(resource_data_file, entry.offset, SEEK_SET);
        read_file(resource_data_file, &bitmap, sizeof(bitmap));
        if (bitmap.biSize != 40)
            resource_data_error("Old DIB in %s.  Pass it through SDKPAINT.", current_token.spelling);
        group.bits = bitmap.biBitCount;
        group.planes = bitmap.biPlanes;
        prefix = 0;
        seek_file(resource_data_file, entry.offset, SEEK_SET);
        if (directory.type == 1) {
            group.dimensions.icon.width = entry.width;
            group.dimensions.icon.height = entry.height;
            group.dimensions.icon.colors = entry.colors;
            group.dimensions.icon.reserved = entry.reserved;
        } else if (directory.type == 2) {
            hotspot[0] = entry.planes_or_hotspot_x;
            hotspot[1] = entry.bits_or_hotspot_y;
            prefix = 4;
            group.dimensions.cursor.width = (WORD)bitmap.biWidth;
            group.dimensions.cursor.height = (WORD)bitmap.biHeight;
        }
        group.bytes = entry.bytes + prefix;
        image = resource_alloc(sizeof(*image));
        image->language = (WORD)default_language;
        image->version = default_resource_version;
        image->characteristics = default_characteristics;
        image->name = 0;
        image->ordinal = next_image_ordinal++;
        image->memory_flags = resource->memory_flags;
        image->data_size = entry.bytes + prefix;
        image_type = find_resource_type(0, (WORD)kind);
        put_bytes(&group, sizeof(group));
        put_word(image->ordinal);
        remainder = seek_file(resource_output, 0, SEEK_CUR) & 3;
        if (remainder)
            write_file(resource_output, &zero, 4 - remainder);
        record_resource(image_type, image, hotspot, prefix, entry.bytes);
        seek_file(resource_data_file, directory_position, SEEK_SET);
    }
    resource->data_size = directory.count * 14 + 6;
    if (!(resource->memory_flags & 0x40))
        resource->memory_flags = 0x1030;
    fclose(resource_data_file);
    resource_data_file = 0;
    write_resource(type, resource);
}
