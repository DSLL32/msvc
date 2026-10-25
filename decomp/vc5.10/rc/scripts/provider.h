/* RC-only native dependency adapter, outside reconstructed target source. */
#ifndef VC510_RC_PROVIDER_H
#define VC510_RC_PROVIDER_H
int rc_provider_setup(void);
int rc_provider_run(int,char **);
int rc_provider_denies_flags(void);
int rc_provider_denies_handler(void);
#endif
