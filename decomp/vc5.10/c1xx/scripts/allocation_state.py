"""Actual reserve/commit provider ABI, memory ownership and known continuation."""
import copy
import json
import re
import struct

import allocation_imports


def check(s,d,original,native,trial,text,state,final,symbols,traps):
    def require(value,message):s.require(value,'allocation: '+message)
    def read(name):return json.loads((native/name).read_text())
    def symbol(name):return tuple(int(v,16) for v in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] '+name+'$',symbols,re.M).groups())
    def call(return_pc,destination):
        code=bytes(d.elf_byte(native,return_pc-5+i) for i in range(5));require(code[0]==0xe8 and return_pc+struct.unpack_from('<i',code,1)[0]==destination,'not actual native caller CALL')
    packet=allocation_imports.check_static()
    for row in packet['providers']:
        loaded=original/('loaded-'+row['name'][:-4]+'.bin');require(loaded.stat().st_size==row['bytes'] and allocation_imports.sha(loaded)==row['sha256'] and allocation_imports.export_rva(loaded,'VirtualAlloc')==row['rva'],'loaded hFile provider/export differs')
    kernel=packet['providers'][0];kernel_base=int(re.search(r'^module name=KERNEL32.dll actual=([0-9a-f]+) ',text,re.M)[1],16);provider=int(re.search(r'^allocation-provider iat=000c9204 resolved=([0-9a-f]+)$',text,re.M)[1],16);require(provider==kernel_base+kernel['rva'],'actual target IAT provider differs')
    require(trial['allocation_observers']=={'owner':'_reserve_work_buffer','before':[0x38cdf,0x38cf8],'after':[0x38ce1,0x38cfa],'helper_return':'actual captured stack return, only two declared callers','provider_iat':0xc9204,'final_bss_rva':0x38d07,'scope':'validation only; no extra unmask, stub-return or API mock'},'undeclared observer transport')
    he=read('allocation-helper-entries.json');hr=read('allocation-helper-returns.json');ae=read('allocation-api-entries.json');ar=read('allocation-api-returns.json')
    oe=re.findall(r'^allocation-helper-entry index=(\d+) seq=(\d+) pc=(\w+) esp=(\w+) edi=(\w+) esi=(\w+) return=(\w+) reserve=(\w+) commit=(\w+)$',text,re.M)
    old_returns=re.findall(r'^allocation-helper-return index=(\d+) pc=(\w+) esp=(\w+) eax=(\w+) edi=(\w+) esi=(\w+)$',text,re.M)
    before=re.findall(r'^allocation-before index=(\d+) pc=(\w+) esp=(\w+) edi=(\w+) args=([0-9a-f,]+) error=(\d+) fpu=(\w+)$',text,re.M)
    after=re.findall(r'^allocation-after index=(\d+) pc=(\w+) esp=(\w+) eax=(\w+) edi=(\w+) error=(\d+) fpu=(\w+)$',text,re.M)
    require(len(he)==len(hr)==len(oe)==len(old_returns)==2 and len(ae)==len(ar)==len(before)==len(after)==4,'missing/extra helper/API observations')
    old_bases=[int(row[3],16) for row in old_returns];new_bases=[row['eax'] for row in hr];work_entry=read('work-entry.json');work_pc,work_size=symbol('initialize_work_buffers');reserve_pc,reserve_size=symbol('reserve_work_buffer');api_pc,_=symbol('recon_VirtualAlloc')
    for bases in (old_bases,new_bases):require(all(bases) and (bases[0]+0x500000<=bases[1] or bases[1]+0x500000<=bases[0]),'allocation identities overlap or are NULL')
    for index,(entry,result,left,right) in enumerate(zip(he,hr,oe,old_returns)):
        require(entry['index']==result['index']==int(left[0])==int(right[0])==index,'helper order differs')
        require(entry['pc']==reserve_pc and entry['args']==[0x500000,0x4000] and entry['formal_inventory']==[{'name':'reserve_bytes','type':'unsigned int'},{'name':'commit_bytes','type':'unsigned int'}],'actual helper formals/inputs differ')
        require(work_pc<entry['return_pc']<work_pc+work_size,'helper caller outside real work initializer');call(entry['return_pc'],reserve_pc)
        require(result['pc']==entry['return_pc'] and result['esp']==entry['esp']+4 and all(result[r]==entry[r] for r in ('edi','esi','ebx')),'native helper RET0/register preservation differs')
        require(int(left[2],16)==0x10438cc9 and [int(v,16) for v in left[6:]]==[0x10438cb6 if index else 0x10438c95,0x500000,0x4000] and int(right[1],16)==int(left[6],16) and int(right[2],16)==int(left[3],16)+4 and right[4:]==left[4:6],'original helper actual caller/ABI differs')
        wanted=copy.deepcopy(work_entry['frontend'])
        if index:wanted['work_buffers'][0]=[new_bases[0],new_bases[0]+0x3ffb]
        observed=read('reserve-entry-%02d.json'%(index+1));returned=read('reserve-return-%02d.json'%(index+1));require(observed['frontend']==returned['frontend']==wanted and observed['raw_entry_abi']['stack_arguments']==[0x500000,0x4000],'helper changed modeled state before caller stores')
        require(all(row['last_error']==203 and row['fpu']==0x27f for row in (entry,result)),'helper changed caller error/FPU state')
    for index,(entry,result,left,right) in enumerate(zip(ae,ar,before,after)):
        commit=index%2;allocation=index//2
        require(entry['index']==result['index']==int(left[0])==int(right[0])==index,'API order differs')
        args=[old_bases[allocation] if commit else 0,0x4000 if commit else 0x500000,0x1000 if commit else 0x2000,4];native_args=[new_bases[allocation] if commit else 0]+args[1:]
        require([int(v,16) for v in left[4].split(',')]==args and entry['args']==native_args and entry['pc']==api_pc and entry['formal_inventory']==[{'name':'address','type':'LPVOID'},{'name':'bytes','type':'DWORD'},{'name':'kind','type':'DWORD'},{'name':'protection','type':'DWORD'}],'actual VirtualAlloc formals/arguments differ');call(entry['return_pc'],api_pc)
        require(reserve_pc<entry['return_pc']<reserve_pc+reserve_size and result['pc']==entry['return_pc'] and result['esp']==entry['esp']+20 and all(result[r]==entry[r] for r in ('edi','esi','ebx')),'native API RET16/caller/register preservation differs')
        require(int(left[1],16)==(0x10438cf8 if commit else 0x10438cdf) and int(right[1],16)==(0x10438cfa if commit else 0x10438ce1) and int(right[2],16)==int(left[2],16)+16 and int(left[3],16)==int(right[4],16)==provider,'original API location/RET16/EDI identity differs')
        require(int(right[3],16)==old_bases[allocation] and result['eax']==new_bases[allocation],'API result allocation identity differs')
        require(int(left[5])==int(right[5])==entry['last_error']==result['last_error']==203 and int(left[6],16)&0xffff==int(right[6],16)&0xffff==entry['fpu']==result['fpu']==0x27f,'API LastError/FPU state differs')
        regions=re.findall(r'^allocation-region index=%d part=(\d+) base=(\w+) allocation=(\w+) allocation-protect=(\w+) bytes=(\w+) state=(\w+) protect=(\w+) type=(\w+)$'%index,text,re.M);base=old_bases[allocation]
        expected=[(0,base,base,4,0x4000,0x1000,4,0x20000),(1,base+0x4000,base,4,0x4fc000,0x2000,0,0x20000)] if commit else [(0,base,base,4,0x500000,0x2000,0,0x20000)]
        require([tuple(int(x,10 if i==0 else 16) for i,x in enumerate(row)) for row in regions]==expected,'original memory region state differs')
        base=new_bases[allocation];mapped=[]
        for line in result['maps'].splitlines():
            m=re.match(r'^([0-9a-f]+)-([0-9a-f]+) ([-rwxps]+) ',line)
            if m:
                a,b=int(m[1],16),int(m[2],16)
                if a<base+0x500000 and b>base:mapped.append((max(a,base),min(b,base+0x500000),m[3]))
        require(mapped==([(base,base+0x4000,'rw-p'),(base+0x4000,base+0x500000,'---p')] if commit else [(base,base+0x500000,'---p')]),'native reservation/commit maps differ')
        if commit:require((original/('work-committed-%02d.bin'%(allocation+1))).read_bytes()==(native/('work-committed-%02d.bin'%(allocation+1))).read_bytes()==bytes(0x4000),'committed16KiB are not actually zero')
    wanted=copy.deepcopy(work_entry['frontend']);wanted['work_buffers']=[[base,base+0x3ffb] for base in new_bases];wanted['command_argc']=26;wanted['command_argv']=state['invocation']['argv_address'];old_count=wanted['cleanup_count'];require(old_count==12,'unexpected prefix cleanup count');wanted['cleanup_count']+=1;wanted['cleanup_callbacks'][old_count]=symbol('cleanup_main')[0]
    require(state['frontend']==wanted and state['args']=={'argv':state['invocation']['argv_address']} and state['symbol']=='initialize_intermediate_files','known main continuation/state/next formal differs')
    before_bss=bytearray((original/'main-bss-frontier.bin').read_bytes());require(len(before_bss)==0xf69c,'missing original BSS guard')
    for (data,limit),base in zip([(0xba850,0xb9020),(0xba854,0xb9028)],old_bases):struct.pack_into('<I',before_bss,data-0xb9000,base);struct.pack_into('<I',before_bss,limit-0xb9000,base+0x3ffb)
    argv=struct.unpack_from('<I',(original/'invoke-entry-argv.bin').read_bytes())[0];struct.pack_into('<II',before_bss,0xc1708-0xb9000,26,argv);struct.pack_into('<I',before_bss,0xc4d54-0xb9000,13);struct.pack_into('<I',before_bss,0xc4d58-0xb9000+12*4,0x104346d3)
    require(bytes(before_bss)==(original/'allocation-final-bss.bin').read_bytes(),'unknown original BSS changed during allocator/main continuation')
    for name in ('frontend-data','frontend-snapshot'):
        entry=(original/('%s-%03d.bin'%(name,int(oe[0][1])))).read_bytes()
        require(len(entry)==8520 and entry==(original/('%s-%s.bin'%(name,final))).read_bytes(),'original initialized/saved bytes changed during allocator/main continuation')
    require([int(v,16) for v in traps[-1][2].split(',')][:2]==[0x10438c35,argv],'original next helper argv/caller differs')
