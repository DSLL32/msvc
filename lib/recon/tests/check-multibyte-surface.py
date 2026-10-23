import subprocess
import sys
binary=sys.argv[1]
subprocess.run([binary],check=True)
for name in ('compare','advance','copy','space','find','widecopy','leadbyte','fullcompare','wideclass','wideupper','wideformat','charlength','span'):
    subprocess.run([binary,name],check=True)
for name in ('upper','pbrk','substring','bytecopy','leadpage','leadhigh','widedup','widenumber','putenv'):
    result=subprocess.run([binary,name],capture_output=True)
    assert result.returncode==125,(name,result.returncode,result.stderr)
print('13 implemented ASCII/UTF16 imports compile/run;7 unvisited SDK imports and2 unknown lead-byte scopes explicitly unsupported')
