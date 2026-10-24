#!/usr/bin/env python3
"""One explicit original-only debugger transport probe; never an acceptance."""
import argparse,json,re,shutil,subprocess,time
from pathlib import Path
from provenance import freeze,sha
from wine_tool_support import PROJECT,TC,PREFIX,SCRATCH,environment,windows

def main():
 parser=argparse.ArgumentParser();parser.add_argument('--name',required=True);args=parser.parse_args()
 assert args.name.replace('-','').replace('_','').isalnum()
 output=SCRATCH/'vc5.10/c1'/args.name;output.mkdir(exist_ok=False)
 helper=PROJECT/'build/dll-capture.exe';shutil.copy2(helper,output/'dll-capture.exe')
 shutil.copy2(TC/'bin/MSVCRT.DLL',output/'MSVCRT.DLL')
 (output/'ranges.txt').write_text('');(output/'hfile-offset-probe.txt').write_text('actual debug-event hFile only; no target execution\n')
 freeze([PROJECT/'scripts/dll-capture.c',Path(__file__),PROJECT/'scripts/wine_tool_support.py'],output/'source-snapshot.json')
 command=['wine',windows(output/'dll-capture.exe'),'--debug',windows(TC/'bin/C1.DLL'),windows(TC/'bin/MSVCRT.DLL'),'masked','ranges.txt','minimal']
 started=time.perf_counter()
 result=subprocess.run(command,cwd=output,env=environment(),stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=20)
 seconds=time.perf_counter()-started
 (output/'stdout.bin').write_bytes(result.stdout);(output/'stderr.bin').write_bytes(result.stderr)
 (output/'execution.json').write_text(json.dumps(dict(kind='transport-only probe execution, NOT acceptance',command=command,exit=result.returncode,seconds=seconds,helper_sha256=sha(helper)),indent=2)+'\n')
 raw=(output/'events.txt').read_text();assert result.returncode==125 and 'failure 'not in raw
 assert 'hfile-probe-complete target-loaded=0 explicit-offset-identity=1\n'in raw
 assert 'name=c1n32_l.dll'not in raw and not(output/'loaded-target.bin').exists()
 canonical=PREFIX/'drive_c/windows/system32/kernel32.dll';size=canonical.stat().st_size
 assert sha(output/'loaded-kernel32.bin')==sha(canonical)and(output/'loaded-kernel32.bin').stat().st_size==size
 assert 'hfile-implicit actual-offset=00002000 requested-reference=00000000 bytes=32 matches=0\n'in raw
 rows=re.findall(r'^hfile-explicit index=(\d+) offset=([0-9a-f]+) before=([0-9a-f]+) after=([0-9a-f]+) requested=32 status=(\d+) error=(\d+) bytes=(\d+) matches=(\d+)$',raw,re.M);assert len(rows)==5
 expected=[(0,32),(8192,32),(65536,32),(size-13,13),(size,0)];reference=canonical.read_bytes();observed=[]
 for index,(row,(offset,count))in enumerate(zip(rows,expected)):
  i,at,before,after,status,error,got,matches=row;at,before,after=(int(x,16)for x in (at,before,after));status,error,got,matches=map(int,(status,error,got,matches))
  assert int(i)==index and at==offset and before==(0 if offset else 8192)and before!=offset
  assert got==count and matches==1 and(status==1 if count else status==1 or status==0 and error==38)
  assert after==offset+count
  assert(output/('hfile-sample-%d.bin'%index)).read_bytes()==reference[offset:offset+count]
  observed.append(dict(offset=offset,position_before=before,position_after=after,requested=32,returned=count,status=status,last_error=error))
 receipt=dict(schema=1,kind='original-only actual-hFile explicit-offset transport probe; NO TARGET ACCEPTANCE',command=command,exit=result.returncode,seconds=seconds,
  actual_hfile_kernel32=dict(bytes=size,sha256=sha(output/'loaded-kernel32.bin')),helper_sha256=sha(helper),target_executed=False,probe=observed,
  conclusions=['Actual debug hFile reads complete synchronously without ERROR_IO_PENDING','OVERLAPPED.Offset is honored despite deliberately wrong current file pointer','The synchronous file pointer DOES update; do not claim pointer immutability','Partial-tail and EOF returned counts/status are observed, not assumed','The complete actual same-hFile output has canonical size/SHA; no reopen or duplicate supplies actual bytes'],
  artifacts={p.name:dict(bytes=p.stat().st_size,sha256=sha(p))for p in output.iterdir()if p.is_file()})
 (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(output=str(output),seconds=seconds,probe=observed)))
if __name__=='__main__':main()
