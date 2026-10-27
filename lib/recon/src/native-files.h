#ifndef RECON_NATIVE_FILES_H
#define RECON_NATIVE_FILES_H
/* Internal process-wide registered path service; callers retain CRT policy. */
int recon_native_path_probe(const char *,struct stat64 *);
int recon_native_file_open(const char *,int *);
/* Existing DOS wildcard grammar; independent of the CRT singleton search. */
int recon_native_find_match(const char *,const char *);
#endif
