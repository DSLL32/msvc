# Existing DLL startup support

These two historical implementations predate the rule excluding new handwritten
CRT reconstruction. They remain necessary to preserve this DLL's already
verified attach/export/detach contract in both Win32 and native builds; the
shipped MSVCRT import library supplies the public CRT calls, not these local
startup bodies. Neither file receives application reconstruction credit, and
this layout change does not authorize further CRT work.

- `startup.c`: the existing 191-byte _DllMainCRTStartup owner forwards the raw
  startup hook, CRT initialization and application DllMain with the original
  stdcall ABI and lifecycle order.
- `crt-startup.c`: the existing 364-byte _CRT_INIT owner maintains this DLL's
  attach count, initializer/exit table and historical platform/FDIV state around
  the imported CRT boundary. Replacing it with a generic host initializer would
  change the accepted lifecycle graph and state contract.
