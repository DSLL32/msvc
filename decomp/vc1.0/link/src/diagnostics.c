/* Native diagnostics and lazily opened external message catalog. */
#include "linker-types.h"
#include <io.h>
#include <recon.h>
char *lookup_diagnostic_text(unsigned);
const char *lookup_diagnostic_code(unsigned);
void open_diagnostic_file(const char *);
FILE *diagnostic_stream;
int diagnostic_initialized;
long diagnostic_offsets[138];
char diagnostic_line[256];
char diagnostic_path[264];

extern char *output_name;
void close_all_files(void);
void remove_temporary_inputs(void);

/* Fatal diagnostic and shared cleanup [000218d0,0002198b).
 * This wrapper forwards exactly one machine word from its variable arguments. */
void fatal_message(const char *file,int message,...)
{
    char *label,*format;
    unsigned long argument;
    va_list args;
    if (banner_pending) print_banner();
    label=lookup_diagnostic_text(0);
    printf("%s :",file);
    if (label) printf("%s ",label);
    message=(unsigned short)message;
    printf("%s:",lookup_diagnostic_code(message));
    format=lookup_diagnostic_text(message);
    if (format) {
        va_start(args,message);
        argument=va_arg(args,unsigned long);
        va_end(args);
        printf(format,argument);
    }
    fputc('\n',stdout);
    close_all_files();
    remove_temporary_inputs();
    if (output_name && *output_name) _unlink(output_name);
    exit(message);
}

void out_of_memory(void) { __asm { int 3 } }

/* Catalog text lookup [000288a0,00028980). */
char *lookup_diagnostic_text(unsigned index)
{
    long position,*next;
    char *text;
    unsigned length;
    if (!diagnostic_initialized) {
        diagnostic_initialized=1;
        open_diagnostic_file(_pgmptr);
        if (!diagnostic_stream) return NULL;
        position=ftell(diagnostic_stream);
        next=diagnostic_offsets;
        while (fgets(diagnostic_line,256,diagnostic_stream)) {
            *next++=position;
            position=ftell(diagnostic_stream);
        }
    }
    if (!diagnostic_stream) return NULL;
    fseek(diagnostic_stream,diagnostic_offsets[index],SEEK_SET);
    text=fgets(diagnostic_line,256,diagnostic_stream);
    length=0;
    while (text[length]) ++length;
    if (length) text[length-1]=0;
    return strchr(text,':')+1;
}

/* The native137-entry indexed code table [7c2d0,7c4f4) is shared by
 * warning/fatal reporting. Prefixes beginning // are real native entries;
 * message text remains in the external catalog rather than this table. */
const char *diagnostic_codes[137] = {
    "LNK000", "LNK001", "LNK1000", "LNK1100", "LNK1101", "LNK1102",
    "//LNK1103", "LNK1104", "LNK1105", "LNK1106", "LNK1107", "LNK1108",
    "LNK1109", "LNK1110", "LNK1111", "LNK1112", "LNK1113", "//LNK1114",
    "LNK1115", "LNK1116", "LNK1117", "LNK1118", "LNK1119", "LNK1120",
    "LNK1121", "LNK1122", "LNK1123", "//LNK1124", "LNK1125", "//LNK1126",
    "LNK1127", "LNK1128", "LNK1129", "LNK1130", "LNK1131", "LNK1132",
    "LNK1133", "LNK1134", "//LNK1135", "LNK1136", "LNK1137", "LNK1138",
    "LNK1139", "//LNK1140", "LNK1141", "LNK1142", "LNK1143", "LNK1144",
    "LNK1145", "LNK1146", "LNK1147", "LNK1148", "LNK1149", "LNK1150",
    "LNK1151", "LNK1152", "LNK1550", "LNK1551", "LNK1552", "LNK1553",
    "LNK1554", "LNK1555", "LNK1556", "LNK1557", "LNK1558", "LNK1559",
    "LNK1560", "LNK1561", "LNK1562", "LNK1563", "LNK1564", "LNK4000",
    "LNK4001", "//LNK4002", "LNK4003", "LNK4004", "LNK4005", "LNK4006",
    "LNK4007", "LNK4008", "LNK4009", "LNK4010", "LNK4011", "LNK4012",
    "LNK4013", "LNK4014", "LNK4015", "LNK4016", "LNK4017", "LNK4018",
    "LNK4019", "LNK4020", "LNK4021", "LNK4022", "LNK4023", "//LNK4024",
    "LNK4025", "LNK4026", "LNK4027", "LNK4028", "LNK4029", "LNK4030",
    "LNK4031", "//LNK4032", "LNK4033", "LNK4034", "//LNK4035", "LNK4036",
    "LNK4037", "LNK4038", "LNK4039", "LNK4040", "LNK4041", "LNK4042",
    "LNK4043", "LNK4044", "LNK4045", "LNK4046", "LNK4047", "LNK4048",
    "LNK4049", "LNK4050", "LNK4051", "LNK4052", "LNK4053", "//LNK4054",
    "LNK4055", "LNK4056", "LNK4057", "LNK4058", "LNK4059", "LNK4060",
    "LNK4550", "LNK4551", "LNK4552", "LNK4553", "LNK9999",
};
/* Native leaf table lookup [28980,28990). */
const char *lookup_diagnostic_code(unsigned index)
{
    return diagnostic_codes[index];
}

/* Warning wrapper [00021830,000218d0). */
void warning_message(const char *file,unsigned message,...)
{
    char *label,*format;
    unsigned long first,second;
    va_list args;
    if (banner_pending) print_banner();
    label=lookup_diagnostic_text(1);
    printf("%s :",file);
    if (label) printf("%s ",label);
    message=(unsigned short)message;
    printf("%s:",lookup_diagnostic_code(message));
    format=lookup_diagnostic_text(message);
    if (format) {
        va_start(args,message);
        first=va_arg(args,unsigned long);
        second=va_arg(args,unsigned long);
        va_end(args);
        printf(format,first,second);
    }
    fputc('\n',stdout);
    fflush(stdout);
}

/* Locate the companion catalog beside the actual program [287f0,288a0). */
void open_diagnostic_file(const char *path)
{
    char *directory=diagnostic_path;
    _splitpath(path,diagnostic_path,NULL,NULL,NULL);
    while (*directory) ++directory;
    _splitpath(path,NULL,directory,NULL,NULL);
    {
        char *end=diagnostic_path;
        const char *suffix="\\link.err";
        while (*end) ++end;
        while ((*end++=*suffix++)!=0) ;
    }
    diagnostic_stream=fopen(diagnostic_path,"rt");
    if (!diagnostic_stream)
        printf("%s : warning: file not found \"%s\"\n",program_name,diagnostic_path);
}
