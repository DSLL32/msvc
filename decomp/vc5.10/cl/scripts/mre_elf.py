"""Authenticate recorded native MRE PCs against the exact executed ELF bytes."""
import hashlib,re,struct,subprocess
from pathlib import Path


class Layout:
    def __init__(self,path,expected_hash):
        self.path=Path(path);self.raw=self.path.read_bytes()
        assert hashlib.sha256(self.raw).hexdigest()==expected_hash,'different executed native ELF'
        assert self.raw[:7]==b'\x7fELF\x01\x01\x01'
        assert struct.unpack_from('<HH',self.raw,16)==(3,3),'expected native i386 PIE'
        phoff=struct.unpack_from('<I',self.raw,28)[0];entsize,count=struct.unpack_from('<HH',self.raw,42)
        assert entsize==32 and count<=64 and phoff+count*32<=len(self.raw)
        self.segments=[]
        for i in range(count):
            kind,offset,address,physical,filesize,memorysize,flags,align=struct.unpack_from('<8I',self.raw,phoff+32*i)
            if kind==1:
                assert offset+filesize<=len(self.raw)and filesize<=memorysize
                self.segments.append((address,offset,filesize))
        symbols=subprocess.check_output(['/usr/bin/nm','-n','-S','--defined-only',str(self.path)],text=True)
        self.symbols={}
        for line in symbols.splitlines():
            match=re.fullmatch(r'([0-9a-f]+) ([0-9a-f]+) ([A-Za-z]) (\S+)',line)
            if match:
                address,size,kind,name=match.groups()
                self.symbols.setdefault(name,[]).append((int(address,16),int(size,16),kind))
        text=subprocess.check_output(['/usr/bin/objdump','-d','-Mintel',str(self.path)],text=True)
        self.instructions={};self.owners={};owner=None
        for line in text.splitlines():
            label=re.fullmatch(r'([0-9a-f]+) <([^>]+)>:',line.strip())
            if label:owner=label[2];continue
            row=re.match(r'^\s*([0-9a-f]+):\s*((?:[0-9a-f]{2} )*[0-9a-f]{2})\s*\t(.*)$',line)
            if row:
                address=int(row[1],16);raw=bytes.fromhex(row[2])
                assert self.read(address,len(raw))==raw
                self.instructions[address]=raw;self.owners[address]=owner
        self.bias=None

    def symbol(self,name,data=False):
        matches=[row for row in self.symbols[name]if not data or row[2]in'BbDdRrVv'];assert len(matches)==1,'ambiguous ELF symbol '+name
        return matches[0][0]

    def read(self,address,count):
        matches=[self.raw[offset+address-base:offset+address-base+count]for base,offset,size in self.segments if base<=address and address+count<=base+size]
        assert len(matches)==1 and len(matches[0])==count,'unbacked ELF instruction range'
        return matches[0]

    def bind(self,record):
        # Derive relocation bias from independent DATA symbols, not the PCs
        # whose authenticity is being checked. Every addressed symbol agrees.
        biases={address-self.symbol(name,data=True)for name,address in record['data_addresses'].items()}
        biases|={address-self.symbol(name,data=True)for name,address in record['symbols'].items()if name!='errno'}
        assert len(biases)==1,'inconsistent ELF data relocation bias'
        bias=biases.pop();assert bias>0 and not bias%4096
        if self.bias is None:self.bias=bias
        else:assert bias==self.bias,'changed module relocation bias'
        return bias

    def call(self,return_address,callee,caller='prepare_jobs'):
        address=return_address-self.bias-5
        instruction=self.instructions[address]
        assert self.owners[address]==caller and len(instruction)==5 and instruction[:1]==b'\xe8','return is not an authentic prepare_jobs direct call'
        target=address+5+struct.unpack_from('<i',instruction,1)[0]
        assert target==self.symbol(callee),'wrong actual native call target'

    def entry(self,record,index):
        names=('MREFOpenByName','MREQueryMreDrv','MREDrvOneTimeInit')
        self.bind(record)
        assert record['general']['eip']==self.bias+self.symbol(names[index]),'wrong actual native MRE entry PC'
        self.call(record['arguments']['return'],names[index])

    def frontier(self,record,signature_candidate=False):
        self.bind(record);address=record['general']['eip']-self.bias-1
        if signature_candidate:
            assert self.instructions[address]==b'\xcc'and self.owners[address]=='option_matching_selector','not the authentic selector INT3 PC'
            assert record['arguments']['caller']=='option_signature'
            return {'native_module_bias':self.bias,'caller_chain':['prepare_jobs','option_signature','option_matching_selector'],'saved_return_word':'Both actual saved caller frames independently dumped; checked by addressed state decoder.'}
        assert self.instructions[address]==b'\xcc'and self.owners[address]=='option_signature','not the authentic option_signature INT3 PC'
        assert record['arguments']['slot']==record['general']['ebp']+8,'wrong actual cdecl formal slot'
        calls=[]
        for address,instruction in self.instructions.items():
            if len(instruction)==5 and instruction[0]==0xe8 and address+5+struct.unpack_from('<i',instruction,1)[0]==self.symbol('option_signature'):
                calls.append((self.owners[address],address))
        assert len(calls)==1 and calls[0][0]=='prepare_jobs','unexpected option_signature caller graph'
        assert record['arguments']['caller']=='prepare_jobs'
        return {'native_module_bias':self.bias,'unique_static_caller':'prepare_jobs','static_return':self.bias+calls[0][1]+5,
            'saved_return_word':'Not independently dumped in initial prefix; actual older-frame assertion is capture-source authenticated. Next required candidate native capture must include saved EBP/return bytes.'}
