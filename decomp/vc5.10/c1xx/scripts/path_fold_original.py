"""Own C1XX filename-fold/public CRT witness, no native success implied.

Record transport patterned on C1; all addresses, case inputs and expected state
below are independently bound to this target's actual capture.
"""
import hashlib,importlib.util,json,re,struct,time
from pathlib import Path
from allocation_imports import pe_read,export_rva


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def need(condition,message):
    if not condition:raise ValueError('original filename fold: '+message)


def record(path):
    raw=path.read_bytes();need(len(raw)==2637,'serialized record extent')
    sizes=[('meta',80),('state',72),('fp',112),('mb',257),('lower',256),('ctype',514),('old_ctype',514),('result',64),('fold',512),('classes',256)]
    result={};at=0
    for name,size in sizes:result[name]=raw[at:at+size];at+=size
    result['meta']=list(struct.unpack('<20I',result['meta']));result['state']=list(struct.unpack('<18I',result['state']));result['fold']=list(struct.unpack('<256H',result['fold']))
    return result


def check(directory):
    started=time.perf_counter();directory=directory.resolve()
    spec=importlib.util.spec_from_file_location('fold_prerequisite',Path(__file__).with_name('check-isolated-multibyte.py'));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);prior=m.check(directory)
    trial=json.loads((directory/'trial.json').read_text());need(trial['trial']=='_initialize_lowercase_table','sole actual candidate')
    plan={'entry_rva':0x393ca,'return_rva':0x38fee,'IAT_rva':0xc9054,'public_entry_rva':0xa5d3,'provider_hot_bytes':54,'provider_case_table_rva':0x3f7f8,'provider_case_table_bytes':256,'target_map_rva':0xc13f8,'target_map_bytes':512,'next_rva':0x39413,'scope':'all actual unsigned-byte public calls/ABI/errors/fullFP and fullmap output; provider success unclaimed until reviewed'}
    need(trial.get('lowercase_observers')==plan,'exact observer contract')
    target=directory/'loaded-target.bin';provider=directory/'loaded-msvcrt.bin';text=(directory/'events.txt').read_text()
    need(export_rva(provider,'_mbctolower')==0xa5d3 and (directory/'mbctolower-provider.bin').read_bytes()==pe_read(provider,0xa5d3,54),'actual public body/export identity')
    need(pe_read(target,0x393cc,6)==b'\x8b\x2d'+struct.pack('<I',0x104c9054)and pe_read(target,0x393f1,2)==b'\xff\xd5','own cached EBP call/IAT')
    need(text.count('path-fold-observer entry=000393ca iat=000c9054 provider=0000a5d3 table=000c13f8 bytes=512 no-query-calls=1\n')==1,'actual observer inventory')
    need(re.findall(r'^frontier rva=(\w+)$',text,re.M)==['00039413'],'actual final named frontier')
    fp=(directory/'locale-mb-next-fp.bin').read_bytes();ref=list(struct.unpack('<18I',(directory/'locale-mb-next-words.bin').read_bytes()));crt=ref[0];module=0x10400000;offset=0xc13f8-0xb9000
    entered=(directory/'locale-mb-next-target-BSS.bin').read_bytes();completed=(directory/'locale-fold-complete-target-BSS.bin').read_bytes();final=(directory/'locale-fold-next-target-BSS.bin').read_bytes()
    classes=(directory/'frontend-data-locale-mb-next.bin').read_bytes()[8:264];arguments=[i for i in range(256)if classes[i]!=35];seed=bool(trial.get('test_seed'))
    need(len(arguments)==(255 if seed else 256),'full actual byte-domain/skippedlead inventory')
    words=lambda raw:list(struct.unpack('<256H',raw[offset:offset+512]));working=words(entered)
    need(working==([(0x4303+37*i)&65535 for i in range(256)]if seed else[0]*256),'real helper input/labeled nonzero seed')
    need((directory/'filename-case-map-000.bin').read_bytes()==bytes(512),'own pre-entry zeroBSS defaults')
    if seed:
        need((directory/'path-fold-helper-before-seed.bin').read_bytes()==bytes(512),'real reset before helper seed')
        need(text.count('path-fold-seed phase=reset bytes=512 first=8101 step=37\n')==text.count('path-fold-seed phase=helper bytes=512 first=4303 step=37\n')==1,'declared data-only auxiliaries')
        need((directory/'filename-case-map-006.bin').read_bytes()==struct.pack('<256H',*[(0x8101+37*i)&65535 for i in range(256)]),'nonzero initializer input')
    else:need('path-fold-seed 'not in text,'no natural input mock')
    need((directory/'filename-case-map-phase.bin').read_bytes()==bytes(512),'all512 bytes participate BSSclear')
    need(len(list(directory.glob('path-fold-*-entry.bin')))==len(list(directory.glob('path-fold-*-return.bin')))==len(arguments),'complete actual public call inventory')
    results=[];previous=-1;lower_reference=None
    for index,character in enumerate(arguments):
        for skipped in range(previous+1,character):need(classes[skipped]==35,'only actual lead skip');working[skipped]=65535
        before=record(directory/('path-fold-%03d-entry.bin'%index));after=record(directory/('path-fold-%03d-return.bin'%index));a,b=before['meta'],after['meta']
        need(a[:2]==[0,index]and b[:2]==[1,index]and a[2]==b[2]and a[3]==crt+0xa5d3 and b[3]==module+0x393f3,'actual entry/return PCs/index')
        need(a[4]+4==b[4]and a[4]==a[17]==b[17]and a[10]==b[10]==module+0x393f3 and a[11]==b[11]==character,'actual cdecl4 argument/RET0')
        need(a[6:10]==b[6:10]and a[9]==crt+0xa5d3,'nonvolatile/cached EBP preservation')
        need(a[12:15]==b[12:15]==[0,0,203]and a[15:]==b[15:]and a[15:17]==[crt,module]and a[18]and a[19],'own errors/provider/TEB/PTD identity')
        need(before['state']==after['state']==ref and ref[1:11]==[1,1252,0,0,1033,0,0,0,0,1252],'full postselection locale/MB state')
        need(before['fp']==after['fp']==fp,'full112 FP preservation')
        for key,suffix in [('mb','mbctype'),('ctype','ctype'),('old_ctype','old-ctype')]:need(before[key]==after[key]==(directory/('locale-mb-next-'+suffix+'.bin')).read_bytes(),'all table bytes '+key)
        result=(directory/'locale-mb-next-result.bin').read_bytes();need(before['result']==after['result']==result+bytes(64-len(result)),'result lifetime')
        need(before['lower']==after['lower']and before['classes']==after['classes']==classes,'fresh provider case map/target scanner')
        if lower_reference is None:lower_reference=before['lower']
        need(before['lower']==lower_reference,'provider complete lower-map lifetime')
        need(before['fold']==after['fold']==working,'actual partial target output; public API does not write it')
        expected=before['lower'][character]if before['mb'][character+1]&16 else character
        need(b[5]==expected,'full unsigned public result')
        working[character]=expected;results.append([character,expected]);previous=character
    for skipped in range(previous+1,256):need(classes[skipped]==35,'final lead skip');working[skipped]=65535
    working[47]=92;need(words(completed)==working,'complete512 output/forcedslash')
    expected_bss=bytearray(entered);expected_bss[offset:offset+512]=struct.pack('<256H',*working);need(bytes(expected_bss)==completed==final,'only owned512BSS writes/full neighbor/next frontier guard')
    for phase in ('fold-complete','fold-next'):
        for prefix in ('frontend-data-locale-','frontend-snapshot-locale-'):
            need((directory/(prefix+phase+'.bin')).read_bytes()==(directory/(prefix+'mb-next.bin')).read_bytes(),'full unknown initialized/snapshot preservation')
        for name in ('words','mbctype','ctype','old-ctype','result','fp','errors','CRT-argv','CRT-environment','Invoke-argv'):
            need((directory/('locale-'+phase+'-'+name+'.bin')).read_bytes()==(directory/('locale-mb-next-'+name+'.bin')).read_bytes(),'full provider/private-vector lifetime '+name)
    rows=re.findall(r'^path-fold-helper-return seq=(\d+) pc=(\w+) esp=(\w+) entry-esp=(\w+) calls=(\d+)$',text,re.M);need(len(rows)==1,'actual helper return inventory');seq,pc,esp,entryesp,calls=rows[0]
    need(int(pc,16)==module+0x38fee and int(esp,16)==int(entryesp,16)+4 and int(calls)==len(arguments),'helperRET0/caller/count')
    rows=re.findall(r'^trap seq=(\d+) rva=00039413 .*stack=([0-9a-f,]+)$',text,re.M);need(len(rows)==1 and [int(x,16)for x in rows[0][1].split(',')][:2]==[module+0x39021,0],'next actual read_env_arguments(NULL) formal/caller')
    return dict(kind='own original-only canonical unsigned-byte filename-fold witness; no native success/application acceptance',directory=str(directory),original_receipt_sha256=sha(directory/'receipt.json'),provider_sha256=sha(provider),target_sha256=sha(target),trial_sha256=sha(directory/'trial.json'),calls=len(arguments),results=results,unchanged_errors=[0,0,203],full112FP_preserved=True,full_provider_case_table_sha256=hashlib.sha256(lower_reference).hexdigest(),complete_target_table_bytes=512,restore_auxiliary=seed,prerequisite_checker_sha256=sha(Path(m.__file__)),checker_sha256=sha(Path(__file__)),seconds=time.perf_counter()-started)


if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('--output',type=Path);args=parser.parse_args();result=check(args.original)
    if args.output:
        need(not args.output.exists(),'refuse receipt overwrite');args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(status='PASS',calls=result['calls'],receipt=result['original_receipt_sha256'],seconds=result['seconds'])))
