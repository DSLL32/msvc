from pathlib import Path
import subprocess,sys
for index,binary in enumerate(sys.argv[1:]):
    binary=str(Path(binary).resolve())
    modes=('',) if index else ('','null','wrap','high','long','mbcp','gate','max','lccp','handle','other-handle','mbtable','ctype')
    for mode in modes:
        result=subprocess.run([binary]+([mode] if mode else []),capture_output=True)
        expected=125 if mode or index else 0
        assert result.returncode==expected and not result.stdout,(binary,mode,result)
        assert bool(result.stderr)==bool(expected),(mode,result.stderr)
        if expected: assert b'recon: unsupported native operation:' in result.stderr
print('Default/validated1252 boundedASCII lengths,2047/empty/readonly edge,state/errors/semanticFP and12domain+profile denials pass')
