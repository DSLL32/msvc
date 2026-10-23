#include "rc.h"
#include <recon.h>

int rc_main(int argc, char **argv, char **environment)
{
    int result;

    SetConsoleCtrlHandler(Handler, TRUE);
    result = RC(NULL, 0, rc_diagnostic, NULL, argc, argv);
    SetConsoleCtrlHandler(Handler, FALSE);
    exit(result);
    return 0;
}

int WINAPI rc_diagnostic(HWND window, unsigned code, char *message)
{
    int length, percent_count;
    char *percent, *source, *escaped, *destination;

    length = strlen(message);
    percent = strchr(message, '%');
    if (length >= 2 &&
        ((message[length - 2] == '\r' && message[length - 1] == '\n') ||
         (message[length - 2] == '\n' && message[length - 1] == '\r')))
        message[length - 2] = 0;
    if (*message) {
        if (percent) {
            percent_count = 0;
            for (source = message; *source; ++source)
                if (*source == '%')
                    ++percent_count;
            escaped = malloc(strlen(message) + percent_count + 1);
            if (escaped) {
                destination = escaped;
                for (source = message; *source; ++source) {
                    if (*source == '%')
                        *destination++ = *source;
                    *destination++ = *source;
                }
                *destination = 0;
                /* The original retains this temporary allocation. */
                printf(escaped);
            } else {
                while (strchr(message, '%'))
                    *strchr(message, '%') = ' ';
                printf(message);
            }
        } else
            printf(message);
    }
    printf("\n");
    return 0;
}
