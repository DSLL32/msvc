"""Native-only canonical strcmp contract checks; no original execution."""
import subprocess,sys
binary=sys.argv[1];default=len(sys.argv)>2 and sys.argv[2]=='default'
for mode in ('','live','null-left','null-right','null-both')+ (('explicit-default',)if default else ()):
 p=subprocess.run([binary]+([mode]if mode else []),capture_output=True)
 expected=0 if mode in ('','live')else 125
 assert p.returncode==expected,(mode,p.returncode,p.stdout,p.stderr)
 if expected:assert b'recon: unsupported native operation:'in p.stderr
 else:assert p.stdout==b'65536 unsigned pairs +3072 prefix/alignment cases +actual callback/guardpage/errors/full108FP PASS\r\n' and not p.stderr
print(('default hoststrcmp retained +4 explicitAPI denials'if default else 'explicit510 normalizedstrcmp +3NULLdenials')+'; empty/live FP PASS')
