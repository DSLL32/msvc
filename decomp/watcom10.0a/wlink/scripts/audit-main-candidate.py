#!/usr/bin/env python3
"""Static candidate graph check only; does not accept main or run either EXE."""
import argparse,hashlib,json,re,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
REPO=ROOT.parents[2]
OUT=Path('/root/.ai-tmp/msvc2/watcom10.0a/wlink/main-frontier-01')
CALLEES={
'Spawn':0x416d3c, 'DoLinkFromCommandLine':0x4243c8,
'QClose':0x424e10, 'QDelete':0x424ee8, '_LnkFree':0x433344,
'BurnSystemList':0x412670,'FreeList':0x41c5c0,'CloseSpillFile':0x42539c,
'CleanTraces':0x41383c,'FreePaths':0x418300,'FreeUndefs':0x41c2e8,
'CleanLoadFile':0x413acc,'CleanLinkStruct':0x41c398,'FreeFormatStuff':0x41c1d0,
'FreeObjInfo':0x42545c,'FreeVirtMem':0x423564,
'FiniMsg':0x424b0c, 'LnkMemFini':0x433300,'exit':0x433f96}
def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument("--check-return",action="store_true",help="Also run strict RET closure, currently expected HOLD on genuine CRT exit tail")
 args=parser.parse_args()
 OUT.mkdir(exist_ok=True)
 def put(n,v): (OUT/n).write_text(json.dumps(v,indent=2)+'\n')
 put('functions-candidate.json',{'target':{'path':'toolchains/watcom10.0a/binnt/WLINK.EXE','sha256':hashlib.sha256((REPO/'toolchains/watcom10.0a/binnt/WLINK.EXE').read_bytes()).hexdigest()},'driver':[{'name':'main','symbol':'main_','start':0x4242e0,'end':0x4243c8}],'accepted':['main'],'analysis_only':'Select source candidate solely for static audit; no runtime/app acceptance.'})
 mapping={}
 for line in (ROOT/'build/original/wlink.map').read_text().splitlines():
  m=re.match(r'0001:([0-9a-fA-F]+)\s*[+*]?\s*(\S+)',line)
  if m: mapping[m[2]]=0x410000+int(m[1],16)
 put('runtime-candidate.json',[{'name':n,'start':va,'end':va,'pending_extent':True} for n,va in CALLEES.items()])
 put('rebuilt-runtime.json',[{'name':n,'start':mapping[n+'_'],'end':mapping[n+'_'],'pending_extent':True} for n in CALLEES])
 argv=[str(REPO/'tooling/graph-audit/target/release/graph-audit'),'--repo',str(REPO),'--project',str(ROOT),'--profile','cl','--discovery','cfg','--map-format','watcom10','--config',str(OUT/'functions-candidate.json'),'--runtime',str(OUT/'runtime-candidate.json'),'--rebuilt-runtime',str(OUT/'rebuilt-runtime.json'),'--rebuilt',str(ROOT/'build/original/WLINK.EXE'),'--map',str(ROOT/'build/original/wlink.map'),'--check-references','--check-ownership','--check-extents']
 if args.check_return: argv.append("--check-ret")
 start=time.monotonic();p=subprocess.run(argv,capture_output=True);elapsed=time.monotonic()-start
 stem='graph-ret' if args.check_return else 'graph'
 (OUT/(stem+'.stdout.txt')).write_bytes(p.stdout);(OUT/(stem+'.stderr.txt')).write_bytes(p.stderr)
 put(stem+'-receipt.json',{'argv':argv,'status':p.returncode,'elapsed_seconds':elapsed,'application_acceptance':False})
 print(p.stdout.decode());print(p.stderr.decode());print('audit status',p.returncode,'seconds',elapsed)
 return p.returncode
if __name__=='__main__': raise SystemExit(main())
