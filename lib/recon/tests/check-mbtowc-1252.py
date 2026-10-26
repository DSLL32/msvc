from pathlib import Path
import subprocess,sys
selected,generic=[str(Path(p).resolve()) for p in sys.argv[1:]]
for mode in ('','high','count','before-mb','mbcp','gate','max','lccp','handle','other-handle','mbtable','ctype','bound','calls','align','overlap'):
    p=subprocess.run([selected]+([mode] if mode else []),capture_output=True)
    assert p.returncode==0,(mode,p.returncode,p.stdout,p.stderr)
    if mode: assert b'recon: unsupported native operation:' in p.stderr
    else: assert not p.stderr
p=subprocess.run([generic],capture_output=True)
assert p.returncode==0 and b'recon: unsupported native operation:' in p.stderr,(p.returncode,p.stdout,p.stderr)
print('isolated1252 mbtowc ASCII and CP0 vectors, prewrite denials and profile isolation pass')
