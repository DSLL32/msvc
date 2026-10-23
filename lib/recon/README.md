
Canonical MSVCRT startup imports can use `recon_crt_bind_arguments(argc, argv,
 target_environment)` in the native host before target startup. Both vectors
must be mutable, NULL-terminated and live for the process lifetime. They are
borrowed unchanged; target environment is independent of Linux shim settings.
`__getmainargs` supports only the observed wildcard-zero path and leaves its
one-DWORD startup-info input unchanged. The fmode/commode/initenv accessors
return stable mutable state; imported app-type, newmode and matherr callback
state remain inspectable. `_fsopen` has its SDK ABI but deliberately fails closed
until its sharing behavior is exercised; `_tzset` uses the existing shim.
