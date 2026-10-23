"""Native97 actual frame-bound entry/reservation/return observations."""
def install(source,line,output):
    import gdb,json
    from pathlib import Path
    rows=[];active=[];gdb.cvpack_symbol_references=rows
    def save():Path(output).write_text(json.dumps(rows,indent=2)+'\n')
    def memory(address,size):return bytes(gdb.selected_inferior().read_memory(address,size)).hex()
    class Returned(gdb.FinishBreakpoint):
        def __init__(self,frame,row):super().__init__(frame,internal=True);self.row=row
        def stop(self):
            try:
                row=self.row;f=gdb.newest_frame();assert active==[row]and row.get('base')and gdb.selected_thread().global_num==row['thread']
                assert f.name()=='record_symbol_offsets'and int(f.pc())==row['caller_pc']and int(f.read_register('ebp'))==row['caller_fp'],'return callback belongs to another invocation/frame'
                assert memory(int(f.pc())-5,5)==row['caller_call'],'native return instruction changed'
                assert int(f.read_var('offset'))==row['offset']and int(f.read_var('module'))==3 and int(f.read_var('size'))==608
                buckets=gdb.parse_and_eval('(void **)%d'%row['buckets']);head=int(buckets[row['bucket']])
                assert head==row['base']and int(gdb.parse_and_eval('public_symbol_cursor'))==row['cursor']and int(gdb.parse_and_eval('public_symbol_available'))==row['available']
                row.update(after=memory(row['base'],36),published=head,return_pc=int(f.pc()),return_fp=int(f.read_register('ebp')),return_line=f.find_sal().line)
                active.clear();save();return len(rows)==3
            except Exception as error:self.row['error']=str(error);save();return True
    class Entered(gdb.Breakpoint):
        def __init__(self):super().__init__('create_symbol_reference',internal=True)
        def stop(self):
            try:
                f=gdb.newest_frame();caller=f.older();assert not active and len(rows)<3 and caller.name()=='record_symbol_offsets'
                row={k:int(f.read_var(k))for k in('record','buckets','module','offset','kind')};assert row['module']==3 and row['kind']==0x400 and row['offset']==[108,296,496][len(rows)]
                parent=caller.older();assert parent.name()=='pack_new_types'and row['record']==int(parent.read_var('start'))+row['offset']and int(caller.read_var('size'))==608
                size=int.from_bytes(bytes.fromhex(memory(row['record'],2)),'little')+2;assert size in(48,56)
                row.update(sequence=len(rows),source=memory(row['record'],size),caller_pc=int(caller.pc()),caller_line=caller.find_sal().line,caller_call=memory(int(caller.pc())-5,5),constructor_address=int(gdb.parse_and_eval('(void *)create_symbol_reference')),caller_fp=int(caller.read_register('ebp')),thread=gdb.selected_thread().global_num,entry_pc=int(f.pc()))
                rows.append(row);active.append(row);Returned(f,row);save();return False
            except Exception as error:rows.append(dict(error=str(error)));save();return True
    class Reserved(gdb.Breakpoint):
        def __init__(self):super().__init__(source+':'+str(line),internal=True)
        def stop(self):
            try:
                f=gdb.newest_frame();assert f.name()=='create_symbol_reference'and len(active)==1;row=active[0]
                assert all(int(f.read_var(k))==row[k]for k in('record','buckets','module','offset','kind'))and 'base'not in row
                node=int(f.read_var('node'));bucket=int(f.read_var('bucket'));blocks=gdb.cvpack_symbol_blocks['allocations'];assert len(blocks)==2
                available=int(gdb.parse_and_eval('public_symbol_available'));cursor=int(gdb.parse_and_eval('public_symbol_cursor'))
                assert node==blocks[1]['base']+1500+36*row['sequence']and cursor==node+36 and available==6600-36*(row['sequence']+1)
                row.update(base=node,bucket=bucket,hash=int(f.read_var('hash')),name_offset=int(f.read_var('name_offset')),segment=int(f.read_var('segment')),address=int(f.read_var('address')),head_before=int(gdb.parse_and_eval('(void **)%d'%row['buckets'])[bucket]),before=memory(node,36),available=available,cursor=cursor,slab_base=blocks[1]['base'],source_line=line)
                save();return False
            except Exception as error:
                if active:active[0]['error']=str(error)
                else:rows.append(dict(error=str(error)))
                save();return True
    Entered();Reserved();save()
