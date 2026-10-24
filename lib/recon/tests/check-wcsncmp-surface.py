import subprocess,sys
for mode in ('null-zero','null-left','null-right','zero','equal','wide','maximum','empty'):
 p=subprocess.run([sys.argv[1],mode],capture_output=True)
 assert p.returncode==125 and not p.stdout and b'recon: unsupported native operation:'in p.stderr,(mode,p.returncode,p.stdout,p.stderr)
print('WCHAR16/cdecl/size_t32 typed public surface:8unvisited calls deny125 PASS')
