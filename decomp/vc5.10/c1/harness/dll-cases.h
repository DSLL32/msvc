#ifndef C1_DLL_CASES_H
#define C1_DLL_CASES_H
#include <recon.h>
#ifdef __cplusplus
extern "C" {
#endif
int c1_lifecycle_invoke(int, char **);
int c1_minimal_setup(void);
int c1_messages_setup(void);
int c1_named_messages_setup(const char *);
int c1_minimal_invoke(int, char **);
#ifdef __cplusplus
}
#endif
#endif
