"""Decode distinct baseline analysis or candidate-bound masked clock responses."""
import datetime
import hashlib
import json
from pathlib import Path
import re
import struct
from binary import PE,u32
from dependency_packet import exports,vector
from import_residuals import qualified_slots
from link_response import decode as link_response
from mktemp_state import original_states,CRT_SHA

PROJECT=Path(__file__).resolve().parents[1];ROOT=PROJECT.parents[2]
digest=lambda raw:hashlib.sha256(raw).hexdigest()

def timeb(raw):
    assert len(raw)==12
    seconds,milliseconds,zone,dst,padding=struct.unpack('<iHhh2s',raw)
    assert 0<=milliseconds<=999
    return {'time':seconds,'millitm':milliseconds,'timezone':zone,'dstflag':dst,'padding_hex':padding.hex(),'raw_hex':raw.hex()}

def decode(directory,response=False):
    directory=Path(directory);capture=json.loads((directory/'capture.json').read_bytes())
    if response:
        assert capture['baseline']is False and capture.get('clock_response')is True and not capture.get('clock_analysis')
    else:assert capture['baseline']is True and capture.get('clock_analysis')is True and not capture.get('clock_response')
    assert not capture.get('link_response')
    packet=link_response(directory,clock_analysis=not response,clock_response=response)
    config=json.loads((directory/'config.json').read_bytes())
    count=2 if not response or '_stop_timer'in config['accepted']else 1
    def read(name):
        raw=(directory/name).read_bytes();assert capture['files'][name]=={'bytes':len(raw),'sha256':digest(raw)};return raw
    assert read('clock-observer.txt')==(b'CL_CLOCK_RESPONSE_V1\n'if response else b'CL_CLOCK_OBSERVER_V1\n')
    helper=read('clock-observer.inc')
    if response:
        assert b'!clock_response_observation_mode(masked,dependency_link_response,'in helper
        assert b'return masked==1 && link_family==1 && analysis==0 && response==1;'in helper
    else:assert b'!clock_observation_mode(masked,dependency_link_response,dependency_clock_analysis)'in helper
    assert b'if (clock_calls!=(DWORD)point) ExitProcess(143);'in helper
    assert b'patch_byte(target_base+clock_sites[point],0xcc);'in helper
    target=PE(ROOT/'toolchains/5.10/bin/CL.EXE');provider=PE(directory/'loaded-msvcrt.bin')
    assert digest(target.data)==capture['target_sha256'] and digest(provider.data)==CRT_SHA
    slots=qualified_slots(target);assert slots[0x40a2a4]=={'dll':'msvcrt.dll','name':'_ftime'}
    assert all(value['name']not in ('_tzname','_timezone','_daylight')for value in slots.values())
    ftime=provider.base+exports(provider)['_ftime']['rva'];assert ftime==0x78002a41
    for address in (0x405402,0x405431):assert target.read(address,6)==b'\xff\x15'+struct.pack('<I',0x40a2a4)
    assert target.data.count(struct.pack('<I',0x40a2a4))==2,'additional target _ftime slot reference'
    phases=['clock-%d-%s'%(index,when)for index in range(1,count+1)for when in ('before','after')]
    events=read('observer/clock-events.txt').decode('ascii')
    parsed=re.findall(r'^ftime sequence=(\d) phase=(before|after) call=([0-9a-f]{8}) return=([0-9a-f]{8}) function=([0-9a-f]{8}) argument=([0-9a-f]{8}) esp=([0-9a-f]{8}) event=(\d+)$',events,re.M)
    assert [(int(row[0]),row[1])for row in parsed]==[(index,when)for index in range(1,count+1)for when in ('before','after')]
    errors=original_states(directory,phases,'observer/clock-events.txt');records=[];last_event=0;namespace=None;tz_identity=None
    for index in range(1,count+1):
        values={};contexts={}
        for when in ('before','after'):
            phase='clock-%d-%s'%(index,when);row=parsed[(index-1)*2+(when=='after')]
            call,returned,function,argument,esp,event=[int(v,16 if i<5 else 10)for i,v in enumerate(row[2:])]
            expected_call=(0x405402,0x405431)[index-1];expected_buffer=(0x409798,0x409688)[index-1]
            assert call==expected_call and returned==call+6 and function==ftime and argument==expected_buffer
            assert event>last_event;last_event=event
            context=read('observer/'+phase+'-context.bin');stack=read('observer/'+phase+'-stack.bin')
            assert len(context)==204 and len(stack)==64 and u32(context,196)==esp and u32(context,184)==(call if when=='before'else returned)+1
            assert u32(stack,0)==argument
            raw=timeb(read('observer/'+phase+'-timeb.bin'))
            clock_buffer=read('observer/'+phase+('-started.bin'if index==1 else'-stopped.bin'));assert clock_buffer.hex()==raw['raw_hex']
            assert errors[phase]=={'errno':0,'doserrno':2 if index==1 else 0,'last_error':0}
            words=list(struct.unpack('<9I',read('observer/'+phase+'-tz-words.bin')))
            assert words[0]==provider.base+exports(provider)['_timezone']['rva'] and words[1]==0
            assert words[2]==provider.base+exports(provider)['_daylight']['rva'] and words[3]==0
            assert words[4]==provider.base+exports(provider)['_tzname']['rva'] and words[7]==provider.base+exports(provider)['_environ']['rva']
            names=[];ownership=[]
            for number,address in enumerate(words[5:7]):
                text=read('observer/'+phase+'-tz-name-%d.bin'%number);assert text.endswith(b'\0')and b'\0'not in text[:-1]
                section=next(s for s in provider.sections if provider.base+s['rva']<=address and address+len(text)<=provider.base+s['rva']+max(s['size'],s['virtual_size']))
                assert section['flags']&0x80000000 and not section['flags']&0x20
                names.append(text[:-1].decode('ascii'));ownership.append({'rva':address-provider.base,'section':section['name'],'observed_bytes':len(text),'static_module_storage':True})
            assert names==['Greenwich Standard Time','Greenwich Daylight Time']
            env_address,env=vector(read('observer/'+phase+'-environment.bin'));assert env_address==words[8]
            strings=[text.decode('ascii')for pointer,text in env];assert strings==packet['parent']['CRT_environment']
            assert not any(text.split('=',1)[0].upper()=='TZ'for text in strings)
            current_namespace={'vector':env_address,'pointers':[pointer for pointer,text in env],'strings':strings}
            current_tz={'words':words,'names':names,'ownership':ownership}
            if namespace is None:namespace=current_namespace;tz_identity=current_tz
            else:assert current_namespace==namespace and current_tz==tz_identity
            contexts[when]=context;values[when]={'phase':phase,'event':event,'call':call,'return':returned,'argument':argument,'esp':esp,
                'timeb':raw,'errors':errors[phase],'x87_control_word':u32(context,28)&0xffff,'FloatSave_hex':context[28:140].hex()}
        assert values['before']['esp']==values['after']['esp']
        assert values['before']['timeb']['raw_hex']=='00'*12
        assert contexts['before'][28:140]==contexts['after'][28:140],'_ftime changed x87 state'
        assert values['before']['x87_control_word']==values['after']['x87_control_word']==639
        assert values['after']['timeb']['timezone']==values['after']['timeb']['dstflag']==0
        assert values['after']['timeb']['padding_hex']=='0000'
        records.append(values)
    first=records[0]['after']['timeb']
    assert read('observer/clock-1-before-stopped.bin')==read('observer/clock-1-after-stopped.bin')==bytes(12)
    before,after=[packet['parent']['phases'][name]['event']for name in ('spawn-before','spawn-after')]
    assert records[0]['after']['event']<before<after
    delta=None
    if count==2:
        last=records[1]['after']['timeb'];start=bytes.fromhex(first['raw_hex'])
        assert read('observer/clock-2-before-started.bin')==read('observer/clock-2-after-started.bin')==start
        terminal=packet['parent']['phases']['exit-before']['event']
        assert after<records[1]['before']['event']<records[1]['after']['event']<terminal
        delta=(last['time']-first['time'])*1000+last['millitm']-first['millitm'];assert 0<=delta<2**31
        seconds,milliseconds=divmod(delta,1000)
        output=('time('+packet['child']['path']+')=%d.%03ds\r\n'%(seconds,milliseconds)).encode('ascii')
        assert read('stdout.bin')==output and read('stderr.bin')==b''
    else:
        assert not any(name.startswith('observer/clock-2-')for name in capture['files'])
        assert read('stdout.bin')==read('stderr.bin')==b''
        from frontier_state import decode as frontier
        stopped=frontier(directory,'original')['fields']
        assert stopped['frontier']=='stop_timer'and stopped['parameters']=={'name':packet['child']['path']}
        assert stopped['timer_started']=={key:first[key]for key in('time','millitm','timezone','dstflag','padding_hex')}
    calendar=datetime.datetime.fromtimestamp(first['time'],datetime.timezone.utc)
    configuration=calendar.strftime('%Y-%m-%dT%H:%M:%S')+'.%03d/%d'%(first['millitm'],delta if delta is not None else 0)
    return {'kind':('fresh masked clock/LINK response; acceptance still held'if response else'baseline-only original clock/LINK analysis; never acceptance'),'capture_sha256':digest((directory/'capture.json').read_bytes()),
        'LINK':packet,'clock':{'qualified_import':slots[0x40a2a4],'provider_sha256':CRT_SHA,'function_rva':ftime-provider.base,'calls':records,
            'clock_input':configuration,'expected_calls':count,'delta_milliseconds':delta,'CRT_environment':namespace,'timezone':tz_identity,
            'defined_timeb_fields_strict':True,'padding_observed_preserved':True,'native_timezone_names_matched':False,
            'timezone_name_scope':'Original Greenwich names retained; native PST/PDT labels are not matched. CL has no timezone-name/data import and its whole timer/caller bodies consume time/millitm, errors and CW only. No target/host table reseed.'}}

if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);parser.add_argument('--output',type=Path);parser.add_argument('--response',action='store_true');args=parser.parse_args();payload=json.dumps(decode(args.directory,args.response),indent=2)+'\n'
    if args.output:args.output.write_text(payload)
    else:print(payload,end='')
