#ifndef LINK_DEFINITION_FILE_H
#define LINK_DEFINITION_FILE_H
#include "import-library.h"
#include <recon.h>
enum DefinitionKeyword {
    DEF_NAME,DEF_LIBRARY,DEF_DESCRIPTION,DEF_HEAPSIZE,DEF_STACKSIZE,DEF_PROTMODE,
    DEF_CODE,DEF_DATA,DEF_SEGMENTS,DEF_EXPORTS,DEF_IMPORTS,DEF_STUB,DEF_OLD,
    DEF_APPLOADER,DEF_EXETYPE,DEF_REALMODE,DEF_FUNCTIONS,DEF_INCLUDE,
    DEF_SECTIONS,DEF_OBJECTS,DEF_VERSION
};
char * read_definition_line(void);
unsigned short find_definition_keyword(const char *text);
unsigned short read_definition_name(unsigned char library);
unsigned short next_definition_keyword(void);
unsigned short read_definition_description(char *text);
unsigned short read_definition_sizes(const char *kind,char *text,unsigned long *reserve,unsigned long *commit);
unsigned short find_definition_attribute(const char *text);
unsigned short skip_definition_code(void);
unsigned short skip_definition_data(void);
unsigned short read_definition_sections(const char *kind,char *text);
unsigned short read_definition_exports(Image *image,char *text);
unsigned short read_definition_imports(char *text);
unsigned short parse_definition_export(Image *,char *,const char *);
char *read_definition_ordinal(char *,char *,const char *,char *,unsigned long *,unsigned char *);
#endif
