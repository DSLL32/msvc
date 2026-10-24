"""Loaded only inside GDB by direct-native.py; no original execution."""
import gdb
import json
import struct
from pathlib import Path

settings = json.loads(Path(gdb.convenience_variable('direct_settings').string()).read_text())
output = Path(settings['directory'])
registers = ['eax','ecx','edx','ebx','esi','edi','ebp','esp','eip','eflags','fctrl']
def number(expression):
    return int(gdb.parse_and_eval(expression)) & 0xffffffff
def memory(address, size):
    return bytes(gdb.selected_inferior().read_memory(address,size))
def snapshot():
    if settings.get('kind')=='container-support':
        return container_snapshot()
    if settings.get('kind')=='legacy-support':
        return support_snapshot()
    operand=gdb.parse_and_eval('asm_operand').dereference()
    address=operand['address'].dereference()
    symbol=address['symbol']
    declaration=symbol.dereference() if int(symbol) else None
    buffers={}
    for name in settings['buffered_streams']:
        stream=number(name)
        if not stream:
            buffers[name]=None
            continue
        ptr,count,base=struct.unpack('<III',memory(stream,12))
        assert 0 <= ptr-base <= 65536,(name,ptr,base)
        buffers[name]=memory(base,ptr-base).hex() if ptr>base else ''
    return dict(registers={r:number('$'+r)for r in registers},
                error_count=number('error_count'),asm_current_token=number('asm_current_token'),
                current_token_value=memory(number('&current_token_value'),8).hex(),
                repair_control=memory(number('&asm_parser_control'),16).hex(),
                type_0000=number('type_0000'),
                asm_state=[number('asm_operand_count'),int(operand['size']),int(operand['flags']),int(address['displacement']),int(address['address_flags']),int(address['flags']),int(address['first_register']),int(address['second_register']),int(bool(int(symbol))),int(declaration['storage'][0]) if declaration else 0,int(declaration['index']) if declaration else 0],
                asm_name=declaration['name'].string() if declaration else '',buffers=buffers)

def vector_snapshot(address):
    assert address
    vector=gdb.parse_and_eval('(Vector *)'+str(address)).dereference()
    count=int(vector['count']);capacity=int(vector['capacity'])
    assert count==3 and count<=capacity<=256
    items=[int(vector['items'][i])for i in range(count)]
    names=[gdb.parse_and_eval('(char *)'+str(item)).string()for item in items]
    assert all(name in ['forced-a.h','forced-b.h','forced-c.h']for name in names)
    return dict(address=address,count=count,capacity=capacity,arena=int(vector['arena']),items=int(vector['items']),values=items,names=names)

def container_snapshot():
    buffers={}
    for name in settings['buffered_streams']:
        stream=number(name)
        if not stream:buffers[name]=None;continue
        pointer,count,base=struct.unpack('<III',memory(stream,12))
        assert 0<=pointer-base<=65536
        buffers[name]=memory(base,pointer-base).hex()if pointer>base else''
    result=dict(registers={r:number('$'+r)for r in registers},owner=container_owner,vector=vector_snapshot(container_owner),error_count=number('error_count'),buffers=buffers)
    if settings['unit']>=17:result['typed']=typed_snapshot()
    if settings['unit']==7:result['type']=container_type_snapshot(container_type)
    if 8<=settings['unit']<=13:result['owned']=container_owned_snapshot()
    return result

def typed_ir_snapshot(address,binary=False):
    kind,flags=struct.unpack('<BB',memory(address,2))
    type=struct.unpack('<I',memory(address+4,4))[0]
    return dict(address=address,kind=kind,flags=flags,type=type,typeflags=struct.unpack('<H',memory(type,2))[0],value=struct.unpack('<I',memory(address+12,4))[0],right=struct.unpack('<I',memory(address+16,4))[0]if binary else 0)

def typed_aggregate_snapshot():
    tag,last,type,size,bitfield,alignment,used,flags=struct.unpack('<IIIIHHBB',memory(typed_context,22))
    member_type=struct.unpack('<I',memory(typed_member+12,4))[0]
    return dict(context=typed_context,tag=tag,last=last,type=type,size=size,bitfield=bitfield,alignment=alignment,used=used,flags=flags,member=typed_member,member_type=member_type,member_flags=struct.unpack('<H',memory(member_type,2))[0])

def typed_snapshot():
    value=dict(errors=number('error_count'),counter=number('unnamed_entity_next_index'),override=number('declaration_output_override'),parameters=number('function_parameter_declarations'))
    if 27<=settings['unit']<=29:value['aggregate']=typed_aggregate_snapshot()
    else:
        value['owners']=typed_owners
        if settings['unit']in(19,21):value['type']=container_type_snapshot(typed_owners['callbacktype'])
    return value

def typed_prepare():
    global typed_owners,typed_context,typed_member
    unit=settings['unit']
    if 27<=unit<=29:
        depth=number('aggregate_depth');assert depth<16
        typed_context=number('&aggregate_contexts')+depth*24
        typed_member=struct.unpack('<I',memory(number('$esp')+4,4))[0]
        context=typed_aggregate_snapshot();tag=context['tag'];name=struct.unpack('<I',memory(tag+8,4))[0]
        assert gdb.parse_and_eval('(char *)'+str(name)).string()=='ContainerItem'
        settings['arguments']=[struct.unpack('<I',memory(typed_member+12,4))[0]]if unit==29 else[]
        return
    symbol=find_support_owner('container_result');callback=find_support_owner('ContainerCallback');sink=find_support_owner('container_sink')
    typed_owners=dict(symbol=int(symbol),type=int(symbol.dereference()['type']),callback=int(callback),callbacktype=int(callback.dereference()['type']),shape=int(callback.dereference()['type'].dereference()['related']),sinktype=int(sink.dereference()['type']))
    assert all(typed_owners.values())
    if unit in(18,22,23):
        constructors=[]
        specifications=[('make_integer_constant',37),('make_unsigned_integer_constant',11)]if unit==18 else[('make_integer_constant',37)]if unit==22 else[('make_unsigned_integer_constant',11)]
        for name,argument in specifications:
            address=int(gdb.parse_and_eval(name+'('+str(argument)+')'));value=typed_ir_snapshot(address)
            assert value['kind']==7 and value['flags']==3 and value['value']==argument
            assert value['typeflags']==(0x64 if name=='make_integer_constant'else 0x4064)
            constructors.append(dict(function=name,argument=argument,address=address,ir=value))
        settings['constructors']=constructors
        settings['arguments']=[80,constructors[0]['address'],constructors[1]['address']]if unit==18 else[typed_owners['sinktype'],constructors[0]['address']]
    else:
        name=int(symbol.dereference()['name']);detail=int(callback.dereference()['name'])
        settings['arguments']={17:[37],19:[typed_owners['shape']],20:[typed_owners['type']],21:[typed_owners['shape']],24:[int(symbol),number('global_file')],25:[248,int(symbol),0,name,detail,typed_owners['sinktype'],typed_owners['callbacktype']],26:[],30:[int(symbol)]}[unit]
        if unit in(24,25):
            assert number('function_parameter_declarations')==0 and name and detail
            typed_owners.update(name=name,detail=detail)
        if unit==24:typed_owners['stream']=number('global_file')

def bitset_snapshot(address):
    count=struct.unpack('<I',memory(address,4))[0];assert count==31
    return dict(address=address,count=count,bytes=memory(address+4,5).hex())

def container_owned_snapshot():
    unit=settings['unit']
    if unit in(8,9):
        vector,index=struct.unpack('<Ih',memory(container_owned[0],6))
        return dict(address=container_owned[0],vector=vector,index=index)
    if unit in(10,11,12):
        if unit==12 and not container_owned_live:return dict(released=container_owned[0])
        return [bitset_snapshot(address)for address in container_owned]
    assert unit==13
    return dict(address=container_owned[0],value=struct.unpack('<I',memory(container_owned[0],4))[0],initial=container_initial_flags)

def container_construct():
    global container_owned,container_owned_live,container_initial_flags
    unit=settings['unit'];container_owned=[];container_owned_live=True;constructors=[]
    name='allocate_permanent'if unit in(8,9,13)else'new_bitset';argument=4 if unit==13 else 8 if unit<10 else 31
    for index in range(2 if unit==11 else 1):
        address=int(gdb.parse_and_eval(name+'('+str(argument)+')'))
        assert address;container_owned.append(address)
        initialized=memory(address,argument if name=='allocate_permanent'else 9)
        assert initialized==(b'\0'*argument if name=='allocate_permanent'else bytes.fromhex('1f00000000000000ff'))
        constructors.append(dict(function=name,argument=argument,address=address,initialized_bytes=initialized.hex()))
        if unit in(8,9):gdb.selected_inferior().write_memory(address,struct.pack('<Ih',container_owner,1))
        elif unit==13:
            container_initial_flags=int(find_support_owner('container_result').dereference()['attributes'])
            gdb.selected_inferior().write_memory(address,struct.pack('<I',container_initial_flags))
        else:gdb.selected_inferior().write_memory(address+4,bytes.fromhex('5aa50f40'if index==0 else'0ff03320'))
    settings['constructors']=constructors

def container_type_snapshot(address):
    type=gdb.parse_and_eval('(CType *)'+str(address)).dereference();shapes=[];shape=type['related']
    while int(shape):
        assert len(shapes)<3
        item=shape.dereference();shapes.append(dict(address=int(shape),flags=int(item['flags']),attributes=int(item['attributes']),related=int(item['related'])));shape=item['related']
    return dict(address=address,flags=int(type['flags']),attributes=int(type['attributes']),related=int(type['related']),depth=number('type_description_depth'),shapes=shapes)

def find_support_owner(name):
    scope=gdb.parse_and_eval('global_scope').dereference()
    mask=int(scope['bucket_mask'])
    assert 0<=mask<=1023
    for bucket in range(mask+1):
        entry=scope['buckets'][bucket]
        steps=0
        while int(entry):
            steps+=1
            assert steps<=1024
            symbol=entry.dereference()
            if int(symbol['name']) and symbol['name'].string()==name:return entry
            entry=symbol['next']
    raise AssertionError('genuinely parsed support owner missing: '+name)
def support_snapshot():
    symbol=support_owner.dereference()
    type=symbol['type'].dereference()
    shape=type['related']
    buffers={}
    for name in settings['buffered_streams']:
        stream=number(name)
        if not stream:buffers[name]=None;continue
        pointer,count,base=struct.unpack('<III',memory(stream,12))
        assert 0<=pointer-base<=65536
        buffers[name]=memory(base,pointer-base).hex() if pointer>base else ''
    result=dict(type_description_depth=number('type_description_depth'),registers={r:number('$'+r)for r in registers},
        owner=int(support_owner),name_address=int(symbol['name']),type=int(symbol['type']),
        related=int(shape),kind=int(symbol['kind']),attributes=int(symbol['attributes']),
        references=int(symbol['references']),type_flags=int(type['flags']),type_attributes=int(type['attributes']),
        shape_flags=int(shape.dereference()['flags'])if int(shape)else 0,
        shape_attributes=int(shape.dereference()['attributes'])if int(shape)else 0,
        based_symbol=number('based_declarator_symbol'),debug_info=number('options.debug_info'),
        error_count=number('error_count'),owner_name=symbol['name'].string(),buffers=buffers)
    if settings['unit']==10:result['segment']=dict(next_symbol_index=number('next_symbol_index'),association=int(symbol['associated']['members']))
    if settings['unit']==11:result['fixup']=dict(buffer=number('debug_record_buffer'),head=number('legacy_type_fixups'),tail=number('legacy_type_fixup_tail'),allocation_bytes=4096)
    if settings['unit']==12:result['intrinsic']=dict(option_Oi=number('options.option_Oi'),interned=number('interned_name'),next_symbol_index=number('next_symbol_index'))
    return result


def literal_snapshot():
    token=support_token.dereference()
    head=memory(int(support_token),2).hex()
    value_type=token['type'].dereference()
    return dict(symbol=int(support_owner),token=int(support_token),head=head,
        type=int(token['type']),flags=int(value_type['flags']),
        extended=int(token['value']['pointer']),
        value=memory(int(token['value']['pointer']),10).hex(),
        line=number('line_file'),auxiliary=number('auxiliary_ir_stream'),
        index=int(support_owner.dereference()['index']))

def run_literal():
    global support_owner,support_token
    breakpoints=[];prior_bytes={}
    try:
        marker_name=settings['marker'];marker_address=number('(unsigned)'+marker_name)
        marker=gdb.Breakpoint('*'+marker_name,internal=True);breakpoints.append(marker)
        gdb.execute('run >'+str(output/'stdout')+' 2>&1')
        marker_address=number('(unsigned)'+marker_name)
        assert number('$eip')==marker_address,'natural initializer entry differs'
        prior_bytes[marker_address]=memory(marker_address,1)
        original_stack=number('$esp');ret,symbol,token,stream=struct.unpack('<IIII',memory(original_stack,16))
        assert stream==0 and symbol and token
        prior_bytes[ret]=memory(ret,1)
        call=memory(ret-5,5)
        assert call[0]==0xe8 and (ret+struct.unpack('<i',call[1:])[0])&0xffffffff==marker_address,'actual caller is not a direct call to the marker'
        support_owner=gdb.parse_and_eval('(FrontendSymbol *)'+str(symbol))
        support_token=gdb.parse_and_eval('(IrToken *)'+str(token))
        assert support_owner.dereference()['name'].string()==settings['owner_name']
        before=snapshot();literal_before=literal_snapshot()
        assert literal_before['flags']&0x100 and (int(support_token.dereference()['flags'])&7)==3 and literal_before['line']
        gdb.execute('set $direct_storage = ((void *(*)(unsigned))malloc)(65536)')
        storage=number('$direct_storage');assert storage and snapshot()==before and literal_snapshot()==literal_before
        stack=storage+65536-512
        words=[ret,symbol,token]
        gdb.selected_inferior().write_memory(stack,struct.pack('<III',*words)+b'\xa5'*20)
        parent=settings['function'];entry=gdb.Breakpoint(parent,internal=True);breakpoints.append(entry)
        returned=gdb.Breakpoint('*'+str(ret),internal=True);returned.condition='$esp == '+str(stack+12);breakpoints.append(returned)
        gdb.execute('set $esp = '+str(stack));gdb.execute('set $eip = (unsigned)'+parent);gdb.execute('continue')
        assert gdb.newest_frame().name()==parent
        slots=[struct.unpack('<I',memory(number('$ebp')+8+4*i,4))[0]for i in range(2)]
        formals=[number(n)for n in settings['formals']]
        assert slots==formals==[symbol,token]
        entry.delete();gdb.execute('continue')
        assert number('$eip')==marker_address,'genuine parent did not re-enter initializer'
        nested_sp=number('$esp');nested=list(struct.unpack('<IIII',memory(nested_sp,16)))
        assert storage<=nested_sp and nested_sp+16<=stack
        assert nested[1:]==[symbol,token,literal_before['line']]
        reentry=dict(marker=marker_address,esp=nested_sp,arguments=nested[1:])
        marker.delete()
        writer_name=settings['literal_writer'];writer_address=number('(unsigned)'+writer_name)
        writer=gdb.Breakpoint('*'+writer_name,internal=True);breakpoints.append(writer)
        gdb.execute('continue');assert number('$eip')==writer_address,'wrong literal leaf entry'
        writer_sp=number('$esp');value=struct.unpack('<I',memory(writer_sp+4,4))[0];width=settings['literal_width']
        assert storage<=writer_sp and writer_sp+8<=stack and storage<=value and value+width<=stack
        assert number('auxiliary_ir_stream')==number('line_file')==literal_before['line']
        leaf=dict(entry=writer_address,esp=writer_sp,value=value,width=width,stream=number('auxiliary_ir_stream'),bytes=memory(value,width).hex())
        report=dict(before=before,literal_before=literal_before,storage=storage,stack=stack,arguments=slots,formals=formals,reentry=reentry,writer=leaf,writer_context=snapshot(),literal_writer=literal_snapshot(),status='entry',return_address=ret,return_byte=memory(ret,1).hex())
        if settings['body']:
            writer.delete();gdb.execute('continue');assert number('$eip')==ret,'literal parent did not return to its actual caller'
            after=snapshot();literal_after=literal_snapshot()
            assert after['registers']['esp']==stack+12
            for register in ['ebx','esi','edi','ebp','fctrl']:assert after['registers'][register]==before['registers'][register],('literal nonvolatile differs',register)
            canary=memory(stack+12,16);assert canary==b'\xa5'*16
            report.update(after=after,literal_after=literal_after,canary=canary.hex(),status='returned')
        # GDB owns the physical trap bytes. Delete both harness breakpoints on
        # success and failure, restoring the exact underlying application bytes.
        for breakpoint in breakpoints:
            if breakpoint.is_valid():breakpoint.delete()
        assert all(memory(address,1)==byte for address,byte in prior_bytes.items())
        report['breakpoints_restored']=True
        (output/'state.json').write_text(json.dumps(report,indent=2)+'\n')
    finally:
        for breakpoint in breakpoints:
            if breakpoint.is_valid():breakpoint.delete()
        cleanup={str(address):dict(expected=byte.hex(),observed=memory(address,1).hex())for address,byte in prior_bytes.items()}
        (output/'breakpoint-cleanup.json').write_text(json.dumps(cleanup,indent=2)+'\n')
        assert all(item['expected']==item['observed']for item in cleanup.values()),'native harness breakpoint restoration differs'
        gdb.execute('kill')

gdb.execute('set pagination off')
gdb.execute('set confirm off')
gdb.execute('set disable-randomization off')
if settings.get('kind')=='legacy-support'and settings.get('unit',-1)in(8,9):
    run_literal()
    gdb.execute('quit')
marker_name=settings.get('marker','report_asm_operand_error')
marker=gdb.Breakpoint('*'+marker_name,internal=True)
gdb.execute('run >'+str(output/'stdout')+' 2>&1')
assert number('$eip')==number('(unsigned)'+marker_name)
if settings.get('kind')=='container-support':
    container_owner=number('forced_source_inputs')
    assert container_owner==number('options.forced_includes')
    actual=vector_snapshot(container_owner)
    assert actual['capacity']==40 and actual['arena']==-1
    if settings['unit']==7:
        container_type=int(find_support_owner('container_result').dereference()['type'])
        assert container_type and len(container_type_snapshot(container_type)['shapes'])==1
    if settings['unit']>=17:typed_prepare()
    elif 8<=settings['unit']<=13:
        container_construct()
        settings['arguments']=[container_owned[0]]
        if settings['unit']==8:settings['arguments'].append(actual['values'][2])
        if settings['unit']==11:settings['arguments'].append(container_owned[1])
        if settings['unit']==13:settings['arguments'].append(0xf37f9)
    else:settings['arguments']={0:[container_owner,1],1:[container_owner,1],2:[container_owner,actual['values'][2]],3:[container_owner],4:[container_owner,container_owner],5:[container_owner],6:[actual['values'][0],actual['values'][2]],7:[2,container_type]if settings['unit']==7 else[],14:[1],15:[container_owner],16:[]}[settings['unit']]
if settings.get('kind')=='legacy-support':
    support_owner=find_support_owner(settings.get('context_owner_name',settings['owner_name']))
    if settings['unit']in(10,11):
        shape=support_owner.dereference()['type'].dereference()['related'];assert int(shape) and int(shape.dereference()['flags'])==0x400 and int(shape.dereference()['attributes'])&7==5,('actual segment shape',int(shape),str(shape.dereference())if int(shape)else'null')
        support_owner=gdb.parse_and_eval('(FrontendSymbol *)'+str(int(shape.dereference()['field08'])))
        assert int(support_owner.dereference()['kind'])==13 and support_owner.dereference()['name'].string()==settings['owner_name']
    owner=support_owner.dereference();unit=settings['unit']
    if unit==11:assert number('debug_record_buffer') and number('options.line_records')==0, 'real legacy 4096-byte buffer constructor required'
    settings['arguments']=([0x800,int(owner['name'])]if unit<2 else[0x800])if unit<3 else({3:[int(support_owner)],4:[2,int(support_owner)],5:[int(owner['type'])],6:[int(owner['type']),7],7:[int(owner['name']),int(owner['name'])+5],10:[int(support_owner)],11:[int(support_owner),number('debug_record_buffer')+8,4],12:[172]}[unit])
before=snapshot()
# GDB's ordinary inferior call allocates a separately owned scratch stack and
# restores the stopped caller context. It never changes compiler-owned globals.
gdb.execute('set $direct_storage = ((void *(*)(unsigned))malloc)(65536)')
storage=number('$direct_storage')
assert storage
assert snapshot()==before,'GDB allocation changed stopped compiler state'
stack=storage+65536-512
words=[number('$eip')]+settings['arguments']
assert len(settings['arguments'])<=7
raw=struct.pack('<'+'I'*len(words),*words)+b'\xa5'*(48-4*len(words))
assert len(raw)==48 and stack+len(raw)<=storage+65536
gdb.selected_inferior().write_memory(stack,raw)
function=settings['function']
entry=gdb.Breakpoint(function,internal=True)
gdb.execute('set $esp = '+str(stack))
gdb.execute('set $eip = (unsigned)'+function)
gdb.execute('continue')
assert gdb.newest_frame().name()==function,('wrong direct entry',gdb.newest_frame().name())
slots=[struct.unpack('<I',memory(number('$ebp')+8+4*i,4))[0]for i in range(len(settings['arguments']))]
formals=[number(n)for n in settings['formals']]
assert slots==settings['arguments'] and formals==slots,('direct ABI slots/formals differ',slots,formals,settings['arguments'])
report=dict(before=before,storage=storage,stack=stack,arguments=slots,formals=formals,entry=snapshot(),status='entry')
if 'constructors'in settings:report['constructors']=settings['constructors']
if settings['body']:
    entry.delete()
    if settings.get('frontier'):
        nested=gdb.Breakpoint('*'+settings['frontier'],internal=True)
        gdb.execute('continue')
        assert number('$eip')==number('(unsigned)'+settings['frontier'])
        argument=struct.unpack('<I',memory(number('$esp')+4,4))[0]
        assert argument==before['owner'],'candidate callee lost the real parsed owner'
        frontier=dict(function=settings['frontier'],argument=argument,context=snapshot())
        if settings['frontier']=='clone_vector_with_extra':
            frontier['arguments']=list(struct.unpack('<III',memory(number('$esp')+4,12)))
            assert frontier['arguments']==[before['owner'],0,1]
        report.update(status='callee-entry',frontier=frontier)
        (output/'state.json').write_text(json.dumps(report,indent=2)+'\n')
        nested.delete();marker.delete();gdb.execute('kill');gdb.execute('quit')
    gdb.execute('continue')
    assert number('$eip')==words[0],('direct callback stopped elsewhere',hex(number('$eip')),gdb.newest_frame().name())
    if settings.get('kind')=='container-support'and settings['unit']==12:container_owned_live=False
    after=snapshot()
    assert after['registers']['esp']==stack+4+(0 if settings.get('convention')=='cdecl'else 4*len(slots)),'direct stack cleanup differs'
    for r in ['ebx','esi','edi','ebp','fctrl']:
        assert after['registers'][r]==before['registers'][r],('direct callee-saved register differs',r)
    canary=memory(stack+4+4*len(slots),16)
    assert canary==b'\xa5'*16,'direct stack canary overwritten'
    report.update(after=after,canary=canary.hex(),status='returned')
    if settings.get('kind')=='container-support'and settings['unit']==28:report['typed_name_result']=gdb.parse_and_eval('(char *)'+str(after['registers']['eax'])).string()
    if settings.get('kind')=='container-support'and settings['unit']in(17,18):report['typed_result']=typed_ir_snapshot(after['registers']['eax'],settings['unit']==18)
    if settings.get('kind')=='container-support'and settings['unit']<2:
        report['result']=vector_snapshot(after['registers']['eax'])
    if settings.get('kind')=='container-support'and settings['unit']==6:
        report['name_result']=gdb.parse_and_eval('(char *)'+str(after['registers']['eax'])).string()
    if settings.get('kind')=='container-support'and settings['unit']==7:
        report['type_result']=container_type_snapshot(after['registers']['eax'])
    if settings.get('kind')=='container-support'and settings['unit']==10:report['bitset_result']=bitset_snapshot(after['registers']['eax'])
    if settings.get('kind')=='container-support'and settings['unit']==14:
        count,capacity,arena=struct.unpack('<hhi',memory(after['registers']['eax'],8));report['empty_result']=dict(count=count,capacity=capacity,arena=arena)
    if settings.get('kind')=='legacy-support' and settings['unit']==11:
        fixup=gdb.parse_and_eval('legacy_type_fixups');assert int(fixup) and int(fixup)==number('legacy_type_fixup_tail')
        item=fixup.dereference()
        report['fixup_result']=dict(symbol=int(item['symbol']),offset=int(item['offset']),bytes=int(item['bytes']),next=int(item['next']))
    if settings.get('kind')=='legacy-support' and settings['unit']==10:
        associated=support_owner.dereference()['associated']['members'];assert int(associated)
        item=associated.dereference();name=item['name'].string()
        report['segment_result']=dict(kind=int(item['kind']),flags=int(item['flags']),name=name,type=int(item['type']),index=int(item['index']),attributes=int(item['attributes']),references=int(item['references']),storage=[int(item['storage'][i])for i in range(3)],global_owner=int(find_support_owner(name)))
    if settings.get('kind')=='legacy-support' and settings['unit']in(4,5,6):
        address=after['registers']['eax'];assert address
        if settings['unit']==4:
            token=gdb.parse_and_eval('(IrToken *)'+str(address)).dereference()
            report['result']=dict(address=address,kind=int(token['kind']),flags=int(token['flags']),type=int(token['type']),symbol=int(token['value']['pointer']))
        else:
            type=gdb.parse_and_eval('(CType *)'+str(address)).dereference();shapes=[];shape=type['related']
            for depth in range(2):
                if not int(shape):break
                item=shape.dereference();shapes.append(dict(address=int(shape),flags=int(item['flags']),attributes=int(item['attributes']),related=int(item['related'])));shape=item['related']
            report['result']=dict(address=address,flags=int(type['flags']),attributes=int(type['attributes']),related=int(type['related']),shapes=shapes)
(output/'state.json').write_text(json.dumps(report,indent=2)+'\n')
gdb.execute('kill')
