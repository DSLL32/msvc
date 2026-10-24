"""Four-byte include-alias root suballocation; pointee layout remains opaque."""
import copy,json,re,struct
from scaler_probe import symbols
from scaler_state import fp,semantic


def check(s,d,original,native,trial,text,state,final,traps):
    def require(ok,message):s.require(ok,'include-alias root: '+message)
    def read(name):return json.loads((native/(name+'.json')).read_text())
    require(trial['trial']in ('_initialize_include_aliases','_initialize_checksums')and not trial.get('test_seed')and trial['alias_root_observers']=={'entry_rva':0x39282,'return_rva':0x38fcf,'request_bytes':4,'arena':0,'pointee_layout':'UNMODELED; bytes untouched','source_formals':[],'ABI':'cdecl0/RET0'},'wrong allocation observer/candidate/auxiliary scope')
    names=symbols(native/'native-executable.elf');entries=read('options-next-entries');returns=read('options-alias-returns');require(len(entries)==2 and len(returns)==1,'actual alias/checksum/return inventory differs');entry,next_entry=entries;returned=returns[0]
    storage=state['frontend']['storage'];arena_offset=storage['arenas'][0]-state['frontend']['bss_address'];root_offset=200+state['frontend']['options_data_layout']['fields']['include_aliases'][0]
    before_bss=bytearray.fromhex(entry['guard']['bss']);before_data=bytearray.fromhex(entry['guard']['data']);cursor,limit,base,committed=struct.unpack_from('<4I',before_bss,arena_offset);root=struct.unpack_from('<I',before_data,root_offset)[0]
    require(root==0 and cursor==base+40960 and limit==base+65536 and committed==65536 and cursor+4<=limit,'actual existing arena cursor/limit/root/committed state differs')
    alias_next_suffix='%03d'%int(next(v[0]for v in traps if v[1]=='000392a8'))
    oldrows=[v for v in traps if v[1]=='00039282'];require(len(oldrows)==1,'original alias entry absent');seq=int(oldrows[0][0]);stack=[int(v,16)for v in oldrows[0][2].split(',')]
    raw_bss=bytearray((original/('pool-bss-%03d.bin'%(seq+1))).read_bytes());raw_data=bytearray((original/('frontend-data-%03d.bin'%seq)).read_bytes());old_cursor,old_limit,old_base,old_committed=struct.unpack_from('<4I',raw_bss,0xc6500-0xb9000)
    require(stack[0]==0x10438fcf and old_cursor==old_base+40960 and old_limit==old_base+65536 and old_committed==65536 and struct.unpack_from('<I',raw_data,0x444)[0]==0,'original owned arena offset/caller/root differs')
    require(entry['pc']==names['initialize_include_aliases'][0] and entry['formal_inventory']==[] and next_entry['pc']==names['initialize_checksums'][0] and next_entry['formal_inventory']==[] and (trial['trial']!='_initialize_include_aliases'or state['symbol']=='initialize_checksums')and state['args']=={},'actual named entry/next noarg ABI differs')
    start,length=names['process_compiler_options']
    for row in entries:
        ret=row['return_pc'];code=bytes(d.elf_byte(native,ret-5+i)for i in range(5));require(start<ret<start+length and code[0]==0xe8 and ret+struct.unpack_from('<i',code,1)[0]==row['pc'],'native real CALL/ownership differs')
    struct.pack_into('<I',before_bss,arena_offset,cursor+4);struct.pack_into('<I',before_data,root_offset,cursor);expected=copy.deepcopy(entry['guard']);expected.update(bss=before_bss.hex(),data=before_data.hex())
    require(returned['guard']==next_entry['guard']==expected,'allocation changed native neighbor/padding/saved/external state beyond cursor/root writes')
    struct.pack_into('<I',raw_bss,0xc6500-0xb9000,old_cursor+4);struct.pack_into('<I',raw_data,0x444,old_cursor)
    require((original/'alias-return-bss.bin').read_bytes()==(original/('pool-bss-%03d.bin'%(int(alias_next_suffix)+1))).read_bytes()==bytes(raw_bss),'original full BSS changed beyond cursor+4')
    require((original/'frontend-data-alias-return.bin').read_bytes()==(original/('frontend-data-'+alias_next_suffix+'.bin')).read_bytes()==bytes(raw_data),'original initialized/unknown bytes changed beyond root pointer')
    saved=(original/('frontend-snapshot-%03d.bin'%seq)).read_bytes();require(saved==(original/'frontend-snapshot-alias-return.bin').read_bytes()==(original/('frontend-snapshot-'+final+'.bin')).read_bytes(),'original complete saved internal snapshot changed')
    require(state['frontend']['arenas'][0]==cursor+4 and state['frontend']['options_data']['include_aliases']==cursor and state['frontend']['saved_options_data']['include_aliases']==0,'serialized actual suballocation/root/snapshot identities differ')
    rows=re.findall(r'^alias-return pc=(\w+) esp=(\w+) eax=(\w+) ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+) error=(\d+)$',text,re.M);require(len(rows)==1,'original actual return absent');row=rows[0]
    esp=int(re.search(r'^trap seq=%d .*esp=(\w+)'%seq,text,re.M)[1],16);regs=re.findall(r'^pool-registers seq=%d ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+)$'%(seq+1),text,re.M)
    require(int(row[0],16)==0x10438fcf and int(row[1],16)==esp+4 and int(row[7])==203 and len(regs)==1 and tuple(row[3:7])==tuple(regs[0]),'original RET0/nonvolatile/error differs')
    require(returned['pc']==entry['return_pc'] and returned['entry_esp']==entry['esp'] and returned['esp']==entry['esp']+4 and all(returned[r]==entry[r]for r in ('ebx','esi','edi','ebp')) and entry['last_error']==returned['last_error']==next_entry['last_error']==203,'native actual return/RET0/nonvolatile/error differs')
    require(semantic(entry['fp'])==semantic(returned['fp'])==semantic(next_entry['fp'])==semantic(fp((original/('x87-context-%03d.bin'%seq)).read_bytes()))==semantic(fp((original/'alias-return-fp.bin').read_bytes())),'architectural FP state changed')
    nexttrap=next(v for v in traps if v[1]=='000392a8');require(int(nexttrap[2].split(',')[0],16)==0x10438fd4,'actual next checksum call/ABI differs')
    for name in ('pool-committed','pool-final-committed'):require((original/(name+'.bin')).read_bytes()==(native/(name+'.bin')).read_bytes()==bytes(65536),'owned committed bytes changed; root contents were not written')
    # Only allocation identity/untouched bytes are established. No root/node
    # layout, EAX semantic return, or natural cold-commit execution is claimed.
    return dict(code_bytes=53,allocation_bytes=4,arena=0,offset=40960,pointee_layout='UNMODELED',pointee_written=False,return_value_claimed=False,cold_CommitExtent_naturally_executed=False)
