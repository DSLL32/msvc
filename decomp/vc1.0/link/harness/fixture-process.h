#ifndef LINK_FIXTURE_PROCESS_H
#define LINK_FIXTURE_PROCESS_H
/* Native fixture policy helpers, never target application vertices. */
#include <stddef.h>
#include <recon-process.h>
#include <recon.h>
int link_fixture_process_environment(const ReconProcessRequest *,const char *,const char *);
int link_fixture_process_context(const ReconProcessRequest *,const char *,int);
int link_fixture_process_argv(const ReconProcessRequest *,const char *const *,unsigned);
int link_fixture_process_path(const ReconProcessRequest *,const char *,char *,size_t);
int link_fixture_cvpack(const char *,const char *,const char *,const unsigned (*)[3],unsigned);
int link_fixture_copy(const char *,const char *,const char *);
#endif
