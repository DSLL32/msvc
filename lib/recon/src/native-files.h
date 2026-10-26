#ifndef RECON_NATIVE_FILES_H
#define RECON_NATIVE_FILES_H
/* Internal process-wide registered path service; callers retain CRT policy. */
int recon_native_path_probe(const char *,struct stat64 *);
int recon_native_file_open(const char *,int *);
#endif
