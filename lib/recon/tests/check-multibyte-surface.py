import subprocess
import sys
binary=sys.argv[1]
subprocess.run([binary],check=True)
for name in ('compare','advance','copy','space','find','widecopy','leadbyte','fullcompare','wideclass','wideupper','wideformat','charlength','span','widenumber','pbrk'):
    subprocess.run([binary,name],check=True)
for name in ('upper','substring','bytecopy','leadpage','leadhigh','widedup','putenv','searchenv'):
    result=subprocess.run([binary,name],capture_output=True)
    assert result.returncode==125,(name,result.returncode,result.stderr)
print('15 implemented ASCII/UTF16 imports compile/run;6 unvisited SDK imports and2 unknown lead-byte scopes explicitly unsupported')
