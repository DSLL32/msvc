#ifndef RESOURCE_FEATURES_H
#define RESOURCE_FEATURES_H
#include "nested/values.h"
#define APP_TITLE "Resource reconstruction"
#define STRING_VALUE(name) #name
#define MAKE_ID(prefix, suffix) prefix##suffix
#define TWICE(value) ((value) * 2)
#define CONTINUED_STRING "Line \
continuation"
#define RESOURCE_SWITCH 1
#define TEMPORARY_SWITCH 1
#undef TEMPORARY_SWITCH
#ifdef TEMPORARY_SWITCH
#error Undefinition failed
#endif
#ifndef RC_INVOKED
#error This header must be processed by the resource compiler
#endif
#ifdef RESOURCE_SWITCH
#define ACTIVE_STRING "The ifdef branch"
#else
#error Wrong ifdef branch
#endif
#if defined(RESOURCE_SWITCH) && (TWICE(BASE_VALUE) == 42) && !defined(TEMPORARY_SWITCH)
#define CONDITIONAL_STRING "The arithmetic if branch"
#elif BASE_VALUE == 0
#error Wrong elif branch
#else
#error Wrong else branch
#endif
#if 0
#include "deliberately-absent.h"
#error Inactive directives must be skipped
#endif
#endif
