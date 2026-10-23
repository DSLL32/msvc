"""Read-only native debugger observations of the reached type buffer lifetime."""
def install(source,line):
    import gdb
    gdb.cvpack_buffer=dict(entries=[],returns=[],allocations=[])
    def memory(address,size):return bytes(gdb.selected_inferior().read_memory(address,size)).hex()
    class Returned(gdb.FinishBreakpoint):
        def __init__(self,frame):super().__init__(frame,internal=True)
        def stop(self):
            try:
                p=gdb.cvpack_buffer;a=p['allocations'][0]
                p['returns'].append(dict(sequence=len(p['returns'])+1,result=int(self.return_value),root=memory(a['buffer'],24),node=memory(a['node'],12),payload=memory(a['base'],a['size'])))
                return False
            except Exception as error:
                import traceback
                gdb.write(traceback.format_exc());gdb.cvpack_buffer['error']=str(error);return True
    class Entry(gdb.Breakpoint):
        def __init__(self):
            from pathlib import Path
            site=next(i for i,text in enumerate(Path(source).read_text().splitlines(),1)if text.strip()=='if (buffer->block_size < size) return NULL;')
            super().__init__(source+':'+str(site),internal=True)
        def stop(self):
            try:
                f=gdb.newest_frame();caller=f.older();assert f.name()=='append_buffer'and caller.name()=='save_type'
                p=gdb.cvpack_buffer;sequence=len(p['entries'])+1;assert sequence<=256 and len(p['returns'])==sequence-1
                buffer,data,size=[int(f.read_var(n))for n in('buffer','data','size')]
                assert buffer==int(gdb.parse_and_eval('&type_symbol_storage'))and 0<size<=4096
                owner=int(caller.read_var('entry'));assert data==int(caller.read_var('original'))
                p['entries'].append(dict(sequence=sequence,buffer=buffer,source=data,size=size,owner=owner,source_bytes=memory(data,size),root=memory(buffer,24),index=int(gdb.parse_and_eval('next_type_index')),page=int(caller.read_var('page')),slot=int(caller.read_var('slot'))))
                Returned(f);return False
            except Exception as error:
                import traceback
                gdb.write(traceback.format_exc());gdb.cvpack_buffer['error']=str(error);return True
    class Allocated(gdb.Breakpoint):
        def __init__(self):super().__init__(source+':'+str(line),internal=True)
        def stop(self):
            try:
                p=gdb.cvpack_buffer;f=gdb.newest_frame();assert f.name()=='append_buffer'and not p['allocations']and len(p['entries'])==1
                base=int(f.read_var('payload'));node=int(f.read_var('block'));buffer=int(f.read_var('buffer'));size=int(gdb.parse_and_eval('type_symbol_storage.block_size'))
                assert base and node and size==0x7f00
                p['allocations'].append(dict(base=base,node=node,buffer=buffer,size=size,before=memory(base,size),node_before=memory(node,12),root_before=memory(buffer,24)))
                return False
            except Exception as error:
                import traceback
                gdb.write(traceback.format_exc());gdb.cvpack_buffer['error']=str(error);return True
    Entry();Allocated()


def capture():
    import gdb
    p=getattr(gdb,'cvpack_buffer',None)
    if p is None:return None
    if p.get('error'):return p
    def memory(address,size):return bytes(gdb.selected_inferior().read_memory(address,size)).hex()
    assert gdb.newest_frame().name()in('finish_recursive_types','find_named_type','find_global_type','find_local_type','record_derived_types','convert_global_data32','get_type_record','add_recursive_offset','intern_type','hash_recursive_type','commit_recursive_type','publish_recursive_type','hash_referenced_type','publish_packed_type','convert_procedure32','address_in_module','convert_bp_relative32','convert_register_symbol','end_symbol_block','link_symbol_scopes')and len(p['allocations'])==1
    a=p['allocations'][0];p['frontier']=dict(name=gdb.newest_frame().name(),root=memory(a['buffer'],24),node=memory(a['node'],12),payload=memory(a['base'],a['size']))
    p['hash_nodes']={};seen=set();heads=gdb.parse_and_eval('type_hash_buckets')
    for bucket in range(4096):
        pointer=int(heads[bucket])
        while pointer:
            assert pointer not in seen and len(seen)<256;seen.add(pointer)
            raw=bytes.fromhex(memory(pointer,12));index=int.from_bytes(raw[:2],'little')
            p['hash_nodes']['hash-%d-%d'%(bucket,index)]=dict(address=pointer,bytes=raw.hex())
            pointer=int.from_bytes(raw[8:12],'little')
    assert len(seen)==len(p['returns'])-({'publish_recursive_type':1,'hash_referenced_type':2,'publish_packed_type':3}.get(gdb.newest_frame().name(),0))
    return p
