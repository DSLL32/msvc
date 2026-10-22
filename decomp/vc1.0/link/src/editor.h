#ifndef LINK_EDITOR_H
#define LINK_EDITOR_H
#include "library.h"
extern Image *edit_image;
extern PIMAGE_SECTION_HEADER edit_section_headers;
extern unsigned char edit_optional_changed;
void prepare_edit_input(Argument *);
void apply_edit_options(void);
int rebase_main(int,char **,char **);
int bind_main(int,char **,char **);
#endif
