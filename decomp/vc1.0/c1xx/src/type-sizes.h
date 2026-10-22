#ifndef TYPE_SIZES_H
#define TYPE_SIZES_H
#include "class-members.h"

unsigned __stdcall base_type_size(unsigned short flags, CppSymbol *tag);
unsigned __stdcall inner_type_size(unsigned short flags, CppSymbol *tag, TypeShape *shape);
unsigned __stdcall data_member_pointer_size(TypeShape *shape);
unsigned __stdcall member_function_pointer_size(TypeShape *shape);
unsigned __stdcall pointer_kind_size(unsigned short attributes);
#endif
