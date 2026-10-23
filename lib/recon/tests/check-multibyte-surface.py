import subprocess
import sys
binary=sys.argv[1]
subprocess.run([binary],check=True)
for name in ('compare','advance','copy','space','find','widecopy','leadbyte','fullcompare','wideclass','wideupper'):
    subprocess.run([binary,name],check=True)
for name in ('upper','pbrk','substring','bytecopy','wideformat','leadpage','leadhigh'):
    result=subprocess.run([binary,name],capture_output=True)
    assert result.returncode==125,(name,result.returncode,result.stderr)
print('10 implemented ASCII/UTF16 imports compile/run;5 unvisited SDK imports and2 unknown lead-byte scopes explicitly unsupported')
