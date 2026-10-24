import subprocess,sys
subprocess.run([sys.argv[1]],check=True)
for mode in ('null','empty','ascii','nonascii'):
 result=subprocess.run([sys.argv[1],mode],capture_output=True)
 assert result.returncode==125,(mode,result.returncode,result.stdout,result.stderr)
 assert b'unreviewed _mbslwr'in result.stderr
print('SDK unsignedchar* cdecl(unsignedchar*) link surface,4input scopes deny125')
