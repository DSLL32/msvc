/* Argument/environment packing in RC's older CRT. */
#include "rc.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <process.h>
#include "lowio.h"
#include <recon.h>
#pragma intrinsic(strcpy, strlen, memcpy, memset)
extern unsigned long _doserrno;

/* Original 0x0100e6b0..0x0100e940. argv[0] is initially kept in its own
 * NUL-terminated field; _dospawn joins it to the remaining command text. */
int __cdecl _cenvarg(const char *const *arguments, const char *const *environment,
                    char **command_block, char **environment_block,
                    const char *program)
{
    const char *const *entry = arguments;
    unsigned int command_size = 2, environment_size;
    char *next;

    while (*entry && command_size <= 32767)
        command_size += strlen(*entry++) + 1;
    if (command_size >= 32767) {
        *environment_block = 0;
        errno = E2BIG;
        _doserrno = ERROR_BAD_ENVIRONMENT;
        return -1;
    }
    *command_block = malloc(command_size);
    if (!*command_block) {
        *environment_block = 0;
        errno = ENOMEM;
        _doserrno = ERROR_NOT_ENOUGH_MEMORY;
        return -1;
    }
    if (environment) {
        entry = environment;
        environment_size = 2;
        while (*entry && environment_size <= 32767)
            environment_size += strlen(*entry++) + 1;
        if (command_size + environment_size + strlen(program) > 32766) {
            free(*command_block);
            *environment_block = 0;
            errno = E2BIG;
            _doserrno = ERROR_BAD_ENVIRONMENT;
            return -1;
        }
        *environment_block = malloc(environment_size);
        if (!*environment_block) {
            free(*command_block);
            *environment_block = 0;
            errno = ENOMEM;
            _doserrno = ERROR_NOT_ENOUGH_MEMORY;
            return -1;
        }
    } else
        *environment_block = 0;
    entry = arguments;
    next = *command_block;
    if (*entry) {
        strcpy(next, *entry);
        next += strlen(*entry++) + 1;
    } else
        ++next;
    while (*entry) {
        strcpy(next, *entry);
        next += strlen(*entry++);
        *next++ = ' ';
    }
    next[-1] = 0;
    *next = 0;
    if (environment) {
        entry = environment;
        next = *environment_block;
        if (!*entry)
            *next++ = 0;
        while (*entry) {
            strcpy(next, *entry);
            next += strlen(*entry++) + 1;
        }
    }
    *next = 0;
    return 0;
}

int __cdecl _dospawn(int mode, const char *program, char *command, char *environment)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    char *next = command;
    unsigned char *packet;
    long *handles;
    int descriptors, index;
    BOOL created;
    DWORD flags = 0, error, result;

    if (mode < _P_WAIT || mode > _P_DETACH) {
        errno = EINVAL;
        _doserrno = 0;
        return -1;
    }
    while (*next) {
        while (*next)
            ++next;
        if (next[1])
            *next++ = ' ';
    }
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    descriptors = _nhandle;
    while (descriptors && !_osfile[descriptors - 1])
        --descriptors;
    startup.cbReserved2 = (WORD)(descriptors * 5 + sizeof(int));
    packet = calloc(startup.cbReserved2, 1);
    startup.lpReserved2 = packet;
    *(int *)packet = descriptors;
    memcpy(packet + sizeof(int), _osfile, descriptors);
    handles = (long *)(packet + sizeof(int) + descriptors);
    memcpy(handles, _osfhnd, descriptors * sizeof(long));
    if (mode == _P_DETACH) {
        for (index = 0; index < 3; ++index)
            packet[sizeof(int) + index] = 0;
        for (index = 0; index < 3; ++index)
            handles[index] = -1;
        flags = DETACHED_PROCESS;
    }
    errno = 0;
    _doserrno = 0;
    created = CreateProcessA(program, command, 0, 0, TRUE, flags,
                              environment, 0, &startup, &process);
    error = GetLastError();
    free(packet);
    if (!created) {
        _dosmaperr(error);
        return -1;
    }
    if (mode == _P_OVERLAY)
        _exit(0);
    else if (mode == _P_WAIT) {
        WaitForSingleObject(process.hProcess, INFINITE);
        GetExitCodeProcess(process.hProcess, &result);
        CloseHandle(process.hProcess);
    } else if (mode == _P_DETACH) {
        CloseHandle(process.hProcess);
        result = 0;
    } else
        result = (DWORD)process.hProcess;
    CloseHandle(process.hThread);
    return result;
}
