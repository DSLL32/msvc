/* Browse-information stream setup and record transport. */
#include "browser.h"
#include "initialization.h"
#include <recon.h>

/* Original 0x2bfe0..0x2c04f. Use the browser's separate temporary
   filename, with binary-default w+ access and ordinary CRT buffering. */
void begin_browser_output(void)
{
    char filename[256];
    format_browser_temporary_name(filename);
    browser_file = fopen(filename, "w+");
    if (!browser_file)
        report_fatal(83, message_text(340), filename);
    setvbuf(browser_file, 0, 0, 4096);
    begin_browser_records();
}
