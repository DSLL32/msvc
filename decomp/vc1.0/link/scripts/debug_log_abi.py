"""Defined diagnostic-log inputs/flags/FILE state; offline actual packets only."""
import hashlib,re
from pathlib import Path
COUNTS=dict(debug_initialize=2,open_debug_log=1,debug_printf=1,set_debug_flag=2,fopen=2,fclose=1,setbuf=2,perror=1)
def compare(left,right,complete=False):
    left=[e for e in left if e.get('event')in['entry','return','frontier']and e.get('name')in COUNTS];right=right['events'];assert left and len(left)==len(right),('inventory',len(left),len(right));frames=[[],[]]
    for a,b in zip(left,right):
        assert(a['event'],a['phase'],a['name'])==(b['event'],b['phase'],b['name']);name=a['name'];phase=a['phase']
        assert a['debug_flags_address']==0x5c008,('wrongoriginalarray',a['debug_flags_address'])
        if name in ['fopen','fclose','setbuf','perror']:
            assert a['caller']==dict(fopen=0x3a931,fclose=0x3a910,setbuf=0x3a95d,perror=0x3a943)[name],('originalpubliccaller',name,a.get('caller'))
            assert b['actual_caller']=='open_debug_log',('nativepubliccaller',name,b.get('actual_caller'))
        elif name=='open_debug_log'and phase==2:
            assert a['caller']==0x3a8d6 and b['actual_caller']=='debug_printf',('innerlogcaller',a.get('caller'),b.get('actual_caller'))
        for side,e in enumerate([a,b]):
            args=e['arguments'];assert len(args)==COUNTS[name]and e['names_address']and e['iob_address']
            if name=='debug_initialize':assert args==[e['names_address'],e['names_address']+32]
            elif name=='set_debug_flag':assert args[0]==2 and args[1]&255==(1 if phase==4 else 0)
            elif name=='debug_printf':assert args==[e['names_address']+{2:96,5:160,8:192}[phase]]and e['variadic_words_consumed']==0
            elif name in ['open_debug_log','perror']:
                if phase in[1,3,7]:assert args==[e['names_address']+{1:64,3:128}[phase]if phase!=7 else 0]
                else:assert phase==2 and bytes.fromhex(e['argument_strings'][0])==b'db.log\0'
            elif name=='fclose':assert args==[e['iob_address']+96]
            elif name=='setbuf':assert args==[e['iob_address']+96,0]
            elif name=='fopen':
                assert bytes.fromhex(e['argument_strings'][1])==b'w\0'
                if phase in[1,3]:assert args[0]==e['names_address']+{1:64,3:128}[phase]
                else:assert phase==2 and bytes.fromhex(e['argument_strings'][0])==b'db.log\0'
            assert e['fp_control']&65535==0x27f and e['fp_status']&65535==0 and e['fp_tag']&65535==0xffff
            if side==0:assert bytes.fromhex(e['fp_raw'])[28:108]==bytes(80)
            else:assert len(re.findall(r'Empty\s+0x00000000000000000000',e['fp_raw']))==8
            if e['event']=='entry':frames[side].append(e)
            elif e['event']=='return':
                before=frames[side].pop();assert before['name']==name and e['esp']==before['esp']+4 and all(e[r]==before[r]for r in['ebx','esi','edi','ebp'])
        assert a['debug_flags']==b['debug_flags']and len(bytes.fromhex(a['debug_flags']))==100
        expected=bytearray(100)
        if not(phase==0 and a['event']=='entry')and not(phase>7 or phase==7 and name=='open_debug_log'and a['event']=='return'):expected[1]=1
        if phase==4 and a['event']=='return' or phase==5 or phase==6 and a['event']=='entry':expected[2]=1
        assert bytes.fromhex(a['debug_flags'])==expected,('flagtransition',phase,name,a['event'])
        assert a['names']==b['names']and bytes.fromhex(a['names'])[-16:]==b'\xa5'*16
        if 'argument_strings'in a:assert a['argument_strings']==b['argument_strings']
        assert a['errors']==b['errors'],('errors',phase,name,a['event'],a['errors'],b['errors'])
        errors=[2,3,0]
        if phase==0 or phase==1 and a['event']in ['entry','frontier']and name in ['open_debug_log','fopen']:errors=[0,0,0]
        elif phase==1 or phase==2 and a['event']in ['entry','frontier']and name in ['debug_printf','open_debug_log','fopen']:errors=[2,3,3]
        assert a['errors']==errors,('errortransition',phase,name,a['event'],a['errors'],errors)
        for e in[a,b]:assert e['debug_log']in[0,e['iob_address']+96]
        assert bool(a['debug_log'])==bool(b['debug_log'])
        assert len(a['streams'])==len(b['streams'])==3
        for u,v in zip(a['streams'],b['streams']):
            assert u['index']==v['index']
            assert u['pending']==v['pending']
            assert u['raw'][1:2]+u['raw'][3:]==v['raw'][1:2]+v['raw'][3:],('FILEscalars',phase,name,a['event'],u,v)
            shapes=[]
            for e,s in[(a,u),(b,v)]:
                raw=s['raw'];base=raw[2];pointer=raw[0]
                if not base:assert not pointer;shapes.append(['null'])
                elif base==e['iob_address']+32*s['index']+20:shapes.append(['charbuf',pointer-base])
                else:assert raw[6]and 0<=pointer-base<=raw[6];shapes.append(['ownedbuffer',pointer-base,raw[6]])
            assert shapes[0]==shapes[1],('FILEowner',phase,name,a['event'],shapes)
        if a['event']=='return':
            if name=='fopen':
                if phase==1:assert a['eax']==b['eax']==0
                else:assert a['eax']==a['iob_address']+96 and b['eax']==b['iob_address']+96
            elif name in ['open_debug_log','debug_printf','fclose']:assert a['eax']==b['eax']
    if complete:assert len(left)==36 and not frames[0]and not frames[1]and right[-1]['phase']==8
    return dict(status='pass',matched_observations=len(left),complete=complete,scope='Actualtypedformals/zeroactualvariadicwords/100Bflags5c008/allotherbytes/ownedstringsguards/FILEdescriptor+bufferrelations+pending/errors/CW+SW+tags+emptyregisters/returnESP+nonvolatiles. Voidresults unconsumed; closedglobal sampledraw withoutdereference. No qualification.')

def compare_effects(original,native,complete=False):
    """Exact current target files and physical streams, including partial runs."""
    original,native=Path(original),Path(native)
    result={}
    for name in ['stdout.bin','stderr.bin']:
        left=(original/name).read_bytes();right=(native/name).read_bytes()
        assert left==right,('physicalstream',name,left,right)
        result[name]=dict(bytes=len(left),sha256=hashlib.sha256(left).hexdigest())
    work=native/'vc1.0/link/direct-debug-log'
    def files(root):
        assert root.is_dir()
        return {str(p.relative_to(root)):p.read_bytes()for p in root.rglob('*')if p.is_file()}
    left,right=files(original/'files'),files(work)
    assert left==right,('targetfiles',sorted(left),sorted(right))
    result['files']={name:dict(bytes=len(data),sha256=hashlib.sha256(data).hexdigest())for name,data in left.items()}
    if complete:
        left=(original/'results.bin').read_bytes();right=(native/'results.bin').read_bytes()
        assert left==right and len(left)==1384
        result['external_report']=dict(bytes=len(left),sha256=hashlib.sha256(left).hexdigest())
    return result
