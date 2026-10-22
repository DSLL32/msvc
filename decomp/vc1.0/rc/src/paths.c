/* Path helpers in RC's older CRT. */
#include <stdlib.h>
#include <string.h>

/* Original 0x0100ef90..0x0100f0e2. Each component is truncated to 255 bytes. */
void __cdecl _splitpath(const char *path, char *drive, char *directory,
                       char *filename, char *extension)
{
    const char *cursor, *last_slash = 0, *last_dot = 0;
    int length;

    if (path[1] == ':') {
        if (drive) {
            strncpy(drive, path, 2);
            drive[2] = 0;
        }
        path += 2;
    } else if (drive)
        *drive = 0;
    for (cursor = path; *cursor; ++cursor) {
        if (*cursor == '/' || *cursor == '\\')
            last_slash = cursor + 1;
        else if (*cursor == '.')
            last_dot = cursor;
    }
    if (last_slash) {
        if (directory) {
            length = last_slash - path;
            if (length >= 255)
                length = 255;
            strncpy(directory, path, length);
            directory[length] = 0;
        }
        path = last_slash;
    } else if (directory)
        *directory = 0;
    if (last_dot && last_dot >= path) {
        if (filename) {
            length = last_dot - path;
            if (length >= 255)
                length = 255;
            strncpy(filename, path, length);
            filename[length] = 0;
        }
        if (extension) {
            length = cursor - last_dot;
            if (length >= 255)
                length = 255;
            strncpy(extension, last_dot, length);
            extension[length] = 0;
        }
    } else {
        if (filename) {
            length = cursor - path;
            if (length >= 255)
                length = 255;
            strncpy(filename, path, length);
            filename[length] = 0;
        }
        if (extension)
            *extension = 0;
    }
}

/* Original 0x0100dfa0..0x0100e038. */
void __cdecl _makepath(char *path, const char *drive, const char *directory,
                      const char *filename, const char *extension)
{
    if (drive && *drive) {
        *path++ = *drive;
        *path++ = ':';
    }
    if (directory && *directory) {
        while (*directory)
            *path++ = *directory++;
        if (directory[-1] != '/' && directory[-1] != '\\')
            *path++ = '\\';
    }
    if (filename)
        while (*filename)
            *path++ = *filename++;
    if (extension) {
        if (*extension && *extension != '.')
            *path++ = '.';
        do {
            *path++ = *extension;
        } while (*extension++);
    } else
        *path = 0;
}
