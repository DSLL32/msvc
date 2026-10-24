#!/usr/bin/env python3
"""Offline actual-C formatter parity; no Wine or target process execution."""
from pathlib import Path
import argparse,hashlib,json,os,shutil,struct,subprocess,time

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--proposal',type=Path,required=True)
parser.add_argument('--original',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
P=args.output.resolve();P.mkdir(parents=True,exist_ok=False)
O=args.original.resolve();proposal=args.proposal.resolve()
for name in ('dll-capture.proposed.c','options-fields.h','proposal.diff'):
    shutil.copy2(proposal/name,P/name)
original_receipt=json.loads((O/'receipt.json').read_text())
assert (original_receipt['kind'],original_receipt['mode'],original_receipt['exit'])==('explicit-original-capture','masked',125)
used_raw=set()
def authenticated_raw(name):
    row=original_receipt['artifacts'][name];data=(O/name).read_bytes()
    assert len(data)==row['size']and hashlib.sha256(data).hexdigest()==row['sha256'],name
    used_raw.add(name);return data
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
source=(P/'dll-capture.proposed.c').read_text()
a=source.index('/* Original-only observation views;');b=source.index('\nstatic void save_aux_state(',a)
body=source[a:b]
a=source.index('static void readmem(');b=source.index('\nstatic void write_mem(',a)
readmem=source[a:b]
prelude=r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint32_t DWORD;
typedef unsigned char BYTE;
typedef int BOOL;
typedef void *HANDLE;
static DWORD base=0x10400000;
static HANDLE child=(void *)1;
static const char *directory,*suffix;
static BYTE bss[0xf69c],initialized[8520],saved[8520],pdb1[28],pdb2[28],descriptors[180];
static DWORD restore_mode,saved_address,rpm_calls;
static int injection;
#include "options-fields.h"
static void fail(const char *why) {fprintf(stderr,"failure:%s RPM=%u\n",why,rpm_calls);exit(90);}
static void load(const char *name,void *buffer,DWORD length) {
 char file[2048];FILE *f;
 snprintf(file,sizeof(file),"%s/%s",directory,name);f=fopen(file,"rb");
 if(!f || fread(buffer,1,length,f)!=length || fgetc(f)!=EOF || fclose(f))fail("offline raw file");
}
static void stage(const char *stem,void *buffer,DWORD length) {
 char name[512];snprintf(name,sizeof(name),"%s-%s.bin",stem,suffix);load(name,buffer,length);
}
static BOOL ReadProcessMemory(HANDLE unused,void *pointer,void *buffer,DWORD bytes,DWORD *done) {
 DWORD address=(DWORD)(uintptr_t)pointer,offset;BYTE *from=NULL;
 (void)unused;++rpm_calls;
 if(injection==1 && rpm_calls==1){*done=0;return 0;}
 if((injection==2 && rpm_calls==1)||(injection==3 && rpm_calls==2)){*done=bytes-1;return 1;}
 #define REGION(start,array) if(!from && address>=(start) && bytes<=sizeof(array) && (offset=address-(start))<=sizeof(array)-bytes)from=(array)+offset
 REGION(base+0xb9000,bss);REGION(base+0xcc000,initialized);REGION(base+0xcb030,pdb1);REGION(base+0xcb050,pdb2);REGION(base+0xd5af8,descriptors);REGION(saved_address,saved);
 if(!from && address==base+0xcb06c && bytes==4)from=(BYTE *)&restore_mode;
 #undef REGION
 if(!from)fail("unmodeled offline RPM");
 memcpy(buffer,from,bytes);*done=bytes;return 1;
}
static void save_blob(const char *name,const void *data,unsigned bytes) {
 DWORD length=(DWORD)strlen(name);fwrite(&length,4,1,stdout);fwrite(&bytes,4,1,stdout);fwrite(name,1,length,stdout);fwrite(data,1,bytes,stdout);
}
'''
postlude=r'''
int main(int argc,char **argv) {
 DWORD values[36],address,bytes;FrontendReadView view;BYTE output[0xf69c];
 if(argc!=5)return 89;
 directory=argv[1];suffix=argv[2];frontend_batch_reads=atoi(argv[3]);injection=atoi(argv[4]);
 load("locale-placeholder",output,0); /* removed by generator; never executed */
 {char name[512];snprintf(name,sizeof(name),"%s-target-BSS.bin",suffix);load(name,bss,sizeof(bss));}
 stage("frontend-data",initialized,sizeof(initialized));stage("frontend-snapshot",saved,sizeof(saved));stage("pdb-context-01",pdb1,sizeof(pdb1));stage("pdb-context-02",pdb2,sizeof(pdb2));stage("arena-descriptors",descriptors,sizeof(descriptors));stage("frontend-state",values,sizeof(values));
 restore_mode=values[3];memcpy(&saved_address,bss+0xc4d50-0xb9000,4);
 if(injection>=10 && injection<30) {
  view.bss=bss;view.initialized=initialized;address=base+0xb9000;bytes=1;
  switch(injection) {
  case 10:address=base+0xb9000;bytes=sizeof(bss);break;
  case 11:address=base+0xc869b;break;
  case 12:address=base+0xcc000;bytes=8520;break;
  case 13:address=base+0xce147;break;
  case 14:address=base+0xcb06c;bytes=4;break;
  case 15:address=base+0xd5af8;bytes=180;break;
  case 16:address=saved_address;bytes=8520;break;
  case 20:address=base+0xb8fff;bytes=2;break;
  case 21:address=base+0xc869b;bytes=2;break;
  case 22:address=base+0xcbfff;bytes=2;break;
  case 23:address=base+0xce147;bytes=2;break;
  case 24:bytes=0;break;
  case 25:address=0xfffffffe;bytes=4;break;
  }
  read_frontend_view(&view,address,output,bytes);fprintf(stderr,"PASS RPM=%u\n",rpm_calls);return 0;
 }
 if(injection==4)base=0xfffffff0;
 save_frontend_state(suffix);
 if(injection==5) {
  bss[0xbe96c-0xb9000]^=4;initialized[4252]^=1;restore_mode^=1;saved[17]^=2;descriptors[0]^=1;
  save_frontend_state(suffix);
 }
 fprintf(stderr,"PASS RPM=%u\n",rpm_calls);return 0;
}
'''.replace(' load("locale-placeholder",output,0); /* removed by generator; never executed */\n','')
program=prelude+readmem+body+postlude
(P/'offline.c').write_text(program)
started=time.perf_counter()
run=subprocess.run(['gcc','-m32','-std=gnu89','-O0','-Wall','-Wextra',str(P/'offline.c'),'-o',str(P/'offline')],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
(P/'offline-build.stdout').write_bytes(run.stdout);(P/'offline-build.stderr').write_bytes(run.stderr)
if run.returncode:raise RuntimeError(run.stderr.decode())
env=os.environ.copy()
for key in ('MATCHER','OPTIONS','LANGUAGE_CONTEXT','CHECKSUM_TABLES'):env['C1XX_OBSERVE_'+key]='1'
def execute(suffix,mode=1,injection=0,binary=None):return subprocess.run([str(binary or P/'offline'),str(O),suffix,str(mode),str(injection)],env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
def records(blob):
 at=0;result=[]
 while at<len(blob):
  length,size=struct.unpack_from('<2I',blob,at);at+=8;name=blob[at:at+length].decode();at+=length;data=blob[at:at+size];at+=size;result.append((name,data))
 if at!=len(blob):raise ValueError('offline record framing')
 return result
phases=sorted(x.name[:-len('-target-BSS.bin')]for x in O.glob('locale-*-target-BSS.bin'))
rows=[];matched=0;before_reads=after_reads=0
for suffix in phases:
 authenticated_raw(suffix+'-target-BSS.bin')
 expected=None;counts=[]
 for mode in (0,1):
  r=execute(suffix,mode);assert r.returncode==0,(suffix,mode,r.stderr)
  actual=records(r.stdout)
  for name,data in actual:assert data==authenticated_raw(name),(suffix,mode,name)
  if expected is not None:assert actual==expected,suffix
  expected=actual;counts.append(int(r.stderr.decode().split('RPM=')[1]));matched+=len(actual)
 rows.append({'phase':suffix,'artifacts':len(expected),'before_RPM':counts[0],'after_RPM':counts[1]});before_reads+=counts[0];after_reads+=counts[1]
negative=[];suffix=phases[-1]
for injection in (1,2,3,4,20,21,22,23,24,25):
 r=execute(suffix,injection=injection);assert r.returncode==90,(injection,r.stderr)
 if injection in (1,2,3,4):assert not r.stdout,(injection,'partial views leaked before successful initial reads')
 if injection==4:assert b'RPM=0'in r.stderr
 negative.append({'case':injection,'rejection':r.stderr.decode().strip()})
for injection in range(10,17):
 r=execute(suffix,injection=injection);assert r.returncode==0,(injection,r.stderr)
 expected_calls=0 if injection<14 else 1;assert ('RPM=%d'%expected_calls).encode()in r.stderr
# Two calls in one process must reread all state and all direct fallbacks.
r=execute(suffix,injection=5);assert r.returncode==0,r.stderr;allrows=records(r.stdout);count=len(rows)and rows[-1]['artifacts'];assert len(allrows)==count*2
first,second=allrows[:count],allrows[count:];changed=[]
for (name,a),(other,b)in zip(first,second):
 assert name==other
 expected=bytearray(a)
 if name.startswith('current-argument-'):expected[0]^=4
 if name.startswith('frontend-data-'):expected[4252]^=1
 if name.startswith('frontend-snapshot-'):expected[17]^=2
 if name.startswith('frontend-state-'):expected[12]^=1
 if name.startswith('arena-descriptors-'):expected[0]^=1
 assert b==expected,(name,'same-process mutable refresh')
 if a!=b:changed.append(name)
assert len(changed)==5,changed
# Real extracted-C wrong-window/offset mutants must fail exact captured parity.
mutants=[]
for name,old,new in [('window','start=base+(part?0xcc000:0xb9000);','start=base+(part?0xcc000:0xb9004);'),('offset','memcpy(buffer,data+offset,bytes);return;','memcpy(buffer,data+offset+(bytes==4?1:0),bytes);return;')]:
 assert program.count(old)==1
 src=P/('offline-mutant-'+name+'.c');binary=P/('offline-mutant-'+name);src.write_text(program.replace(old,new));subprocess.run(['gcc','-m32','-std=gnu89','-O0',str(src),'-o',str(binary)],check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
 r=execute(suffix,binary=binary)
 mismatch=r.returncode!=0 or any(data!=authenticated_raw(key)for key,data in records(r.stdout))
 assert mismatch,'mutation accepted '+name;mutants.append({'case':name,'exit':r.returncode,'rejection':r.stderr.decode().strip(),'exact_artifact_mismatch':r.returncode==0})
for name in sorted(used_raw):authenticated_raw(name)
receipt={'authenticated_raw_files':len(used_raw),'original_artifacts_immutable_at_end':True,'kind':'offline actual extracted proposed-C stopped-span parity; no target/Wine execution','original':str(O),'original_receipt_sha256':sha(O/'receipt.json'),'proposal_source_sha256':sha(P/'dll-capture.proposed.c'),'proposal_diff_sha256':sha(P/'proposal.diff'),'verifier_sha256':sha(Path(__file__)),'actual_extracted_C_sha256':sha(P/'offline.c'),'native_driver_sha256':sha(P/'offline'),'available_full_span_points':len(phases),'compared_artifact_instances_both_modes':matched,'before_remote_read_count':before_reads,'after_remote_read_count':after_reads,'field_files_unchanged':True,'scope':'270 available complete locale BSS/data/saved points only; not all442 save_state points','per_point':rows,'negative_controls':negative,'valid_boundaries':7,'same_process_refresh_changed_files':changed,'wrong_window_offset_mutants':mutants,'seconds':time.perf_counter()-started}
(P/'offline-validation.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k]for k in ('available_full_span_points','compared_artifact_instances_both_modes','before_remote_read_count','after_remote_read_count','seconds')}))
