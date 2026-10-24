"""Actual parser helper inputs and typed grammar objects, without provider calls."""
from pathlib import Path
import re,struct,subprocess
from scaler_probe import symbols
from options_projection import SPECS


def elf_read(binary,address,length):
    raw=binary.read_bytes();assert raw[:7]==b'\x7fELF\x01\x01\x01'and struct.unpack_from('<HH',raw,16)==(2,3)
    table=struct.unpack_from('<I',raw,28)[0];stride,count=struct.unpack_from('<HH',raw,42);assert stride==32
    rows=[struct.unpack_from('<8I',raw,table+i*stride)for i in range(count)]
    found=[r for r in rows if r[0]==1 and r[2]<=address and address+length<=r[2]+r[4]];assert len(found)==1
    row=found[0];return raw[row[1]+address-row[2]:row[1]+address-row[2]+length]


def literal_defaults(binary):
    # Immutable ELF initializer contents supply identities even when an
    # explicit restore auxiliary later overwrites the live pointer fields.
    names=subprocess.check_output(['nm','-S','--defined-only',str(binary)],text=True)
    base=int(re.search(r'^([0-9a-f]+) [0-9a-f]+ [Dd] frontend_data$',names,re.M)[1],16);offset=200;result={}
    wanted={'path_separators':b'\\:/\0','il_prefix':b'\0','database_file':b'vc50.pdb\0'}
    for name,rva,typ,width,kind in SPECS:
        if kind!='initialized':continue
        if name in wanted:
            address=struct.unpack('<I',elf_read(binary,base+offset,4))[0];raw=wanted[name];assert elf_read(binary,address,len(raw))==raw
            result[name]={'address':address,'bytes':raw.hex()}
        offset+=width
    assert set(result)==set(wanted);return result


def setup(output,binary):
    (output/'options-probe-source.py').write_bytes(Path(__file__).read_bytes())
    names=symbols(binary)
    code='''python
options_helper_entries=[]
options_helper_returns=[]
options_next_entries=[]
class OptionsHelperReturn(gdb.Breakpoint):
    def __init__(self,address,entry_esp):
        super(OptionsHelperReturn,self).__init__('*0x%x'%address,internal=True);self.entry_esp=entry_esp
    def stop(self):
        row=scaler_context();row.update(entry_esp=self.entry_esp,guard=scaler_guard());options_helper_returns.append(row);self.enabled=False;return False
class OptionsNextEntry(gdb.Breakpoint):
    def stop(self):
        row=scaler_context();row.update(return_pc=struct.unpack('<I',gdb.selected_inferior().read_memory(row['esp'],4))[0],guard=scaler_guard())
        block=gdb.block_for_pc(row['pc'])
        while block and not block.function:block=block.superblock
        row['formal_inventory']=[{'name':item.name,'type':str(item.type)}for item in block if item.is_argument];options_next_entries.append(row);return False
class OptionsHelperEntry(gdb.Breakpoint):
    def stop(self):
        if @@LANGUAGE_AUX@@:
            obj=gdb.parse_and_eval('frontend_bss.language_contexts');gdb.selected_inferior().write_memory(int(obj.address),struct.pack('<10i',*[1+(i&1)for i in range(10)]))
        row=scaler_context();words=list(struct.unpack('<2I',gdb.selected_inferior().read_memory(row['esp'],8)))
        row.update(return_pc=words[0],args=words[1:],guard=scaler_guard())
        block=gdb.block_for_pc(row['pc'])
        while block and not block.function:block=block.superblock
        row['formal_inventory']=[{'name':item.name,'type':str(item.type)}for item in block if item.is_argument]
        if @@LANGUAGE@@:OptionsHelperReturn(words[0],row['esp'])
        options_helper_entries.append(row)
        if len(options_helper_entries)!=1:raise RuntimeError('unexpected extra parser helper entry')
        layout={}
        for name,kind in [('option_rules','CompilerOptionRule'),('letter_options','CompilerZRule'),('warning_defaults','WarningDefault')]:
            obj=gdb.parse_and_eval(name);address=int(obj.address);width=int(obj.type.sizeof);typ=gdb.lookup_type(kind)
            layout[name]={'address':address,'bytes':width,'record_size':int(typ.sizeof),'fields':{f.name:[f.bitpos//8,int(f.type.sizeof),str(f.type)]for f in typ.fields()}}
            open(@@OUTPUT@@+'/'+name+'-native.bin','wb').write(gdb.selected_inferior().read_memory(address,width).tobytes())
        open(@@OUTPUT@@+'/options-grammar-layout.json','w').write(json.dumps(layout,indent=2)+'\\n')
        rule_bytes=gdb.selected_inferior().read_memory(layout['option_rules']['address'],1212).tobytes();patterns=[]
        for index in range(101):
            pointer=struct.unpack_from('<I',rule_bytes,index*12)[0]
            text=gdb.Value(pointer).cast(gdb.lookup_type('char').pointer()).string(encoding='latin1')if pointer else None
            if text is not None and len(text)>=80:raise RuntimeError('unbounded grammar literal')
            patterns.append(text)
        open(@@OUTPUT@@+'/options-native-patterns.json','w').write(json.dumps(patterns)+'\\n')
        literals=@@LITERALS@@
        for literal in literals.values():
            actual=gdb.selected_inferior().read_memory(literal['address'],len(bytes.fromhex(literal['bytes']))).tobytes().hex()
            if actual!=literal['bytes']:raise RuntimeError('actual default literal object differs from compiled initializer')
            literal['actual_bytes']=actual
        open(@@OUTPUT@@+'/options-native-literals.json','w').write(json.dumps(literals,indent=2)+'\\n')
        return False
OptionsHelperEntry('*0x%x'%@@ENTRY@@,internal=True)
if @@LANGUAGE@@:
    OptionsNextEntry('*0x%x'%@@NEXT_ALIAS@@,internal=True);OptionsNextEntry('*0x%x'%@@NEXT_CHECKSUM@@,internal=True)
end
'''.replace('@@OUTPUT@@',repr(str(output))).replace('@@ENTRY@@',str(names['push_declaration_context'][0])).replace('@@LITERALS@@',repr(literal_defaults(binary))).replace('@@LANGUAGE@@',repr('_push_declaration_context'in __import__('json').loads((output/'trial.json').read_text())['accepted'])).replace('@@NEXT_ALIAS@@',str(names['initialize_include_aliases'][0])).replace('@@NEXT_CHECKSUM@@',str(names['initialize_checksums'][0])).replace('@@LANGUAGE_AUX@@',repr(bool(__import__('json').loads((output/'trial.json').read_text()).get('language_context_observers'))and bool(__import__('json').loads((output/'trial.json').read_text()).get('test_seed'))))
    finish='python\nopen('+repr(str(output/'options-helper-entries.json'))+',"w").write(json.dumps(options_helper_entries,indent=2)+"\\n")\nend\n'
    finish+="python\nopen("+repr(str(output/'options-helper-returns.json'))+",'w').write(json.dumps(options_helper_returns,indent=2)+'\\n')\nopen("+repr(str(output/'options-next-entries.json'))+",'w').write(json.dumps(options_next_entries,indent=2)+'\\n')\nend\n"
    return code,finish
