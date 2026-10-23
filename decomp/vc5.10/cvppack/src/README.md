Win32 C for the completed `/help` case: main, option parsing, diagnostics and
resource messages. Future application callees retain named, typed INT3 stubs.
`cvpack.rc` contains only the three resource strings encountered by this case.
Host setup belongs in `scripts/native-driver.c`; shared compatibility lives in
`lib/recon/`. Both original 5.10 and native i386 builds are checked.
