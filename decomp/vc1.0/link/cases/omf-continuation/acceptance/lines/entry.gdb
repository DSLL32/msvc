set pagination off
set confirm off
break *omf_record_symbol_lines
run -lib /out:continued.lib continued.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/omf-continuation-continued-omf_record_symbol_lines-04lzwh5d/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/omf-continuation-continued-omf_record_symbol_lines-04lzwh5d/native.stderr
printf "NATIVE_MATCH omf_record_symbol_lines\n"
x/6wx $esp
bt 5
python
import gdb,json,struct
logical='omf_record_symbol_lines'
esp=int(gdb.parse_and_eval('$esp'))
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n))
words=struct.unpack('<5I',read(esp,20))
value=lambda n:int(gdb.parse_and_eval(n))
caller=gdb.newest_frame().older();assert caller
state=dict(logical=logical,caller_name=caller.name(),**{'pass':value('omf_pass')})
if logical in ('omf_insert_relocation_symbol','omf_lookup_relocation_symbol'):
    state['formals']=dict(symbol=words[1],offset=words[2]);bucket=(words[1]+words[2])%211
    node=gdb.parse_and_eval('omf_relocation_buckets')[bucket];nodes=[]
    while int(node):
        assert len(nodes)<32
        row=dict(symbol=int(node['symbol']),offset=int(node['offset']))
        if logical=='omf_lookup_relocation_symbol':row['resolved']=int(node['resolved'])
        nodes.append(row);node=node['next']
    state.update(bucket=bucket,nodes=nodes,terminated=True)
elif logical=='omf_extend_section':
    section=struct.unpack('<h',struct.pack('<H',words[1]&65535))[0]
    assert words[4]<=256
    state.update(formals=dict(section=section,offset=words[2],length=words[4],data=list(read(words[3],words[4]))),prior_section_size=value('omf_coff_sections[%d].SizeOfRawData'%(section-1)),prior_total_bytes=value('omf_coff_raw_bytes'))
elif logical=='omf_record_symbol_lines':
    remaining=value('omf_record_remaining');assert 0<=remaining<=256
    stream=gdb.parse_and_eval('omf_input');assert int(stream['_cnt'])>=remaining
    state.update(formals={},remaining=remaining,wide=value('omf_record_wide'),unread_record=list(read(int(stream['_ptr']),remaining)))
else:
    state.update(formals=dict(format=gdb.Value(words[1]).cast(gdb.lookup_type('char').pointer()).string()),program=gdb.parse_and_eval('omf_program_name').string())
open('/root/.ai-tmp/msvc2/vc1.0/link/frontiers/omf-continuation-continued-omf_record_symbol_lines-04lzwh5d/native-omf-continuation-abi.json','w').write(json.dumps(state,indent=2)+'\n')
end
quit
