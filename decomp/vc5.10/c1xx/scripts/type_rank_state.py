"""Encountered type-rank records and exact typed snapshot/restore projection."""
import json
import struct

PRIMITIVE = [[0x10,1],[0x40,2],[0x80,3],[0x100,4],[0x200,5],[0x400,6],[0x800,7],[0,0]]
COMPLEX = [[1,8],[2,9],[3,10],[4,11],[0x10,12],[0x20,13],[0x40,14],[0x44,15],[0,0]]


def records(blob, offset, count):
    return [[struct.unpack_from('<i',blob,offset+i*8)[0],blob[offset+i*8+4]] for i in range(count)]


def check(s, original, native, trial, text, pre, phase, state, seed, final, symbols, traps):
    model = json.loads((native/'state-model.json').read_text())
    s.require(model['type_rank_projection'] == {
        'primitive_original_base':0x104cd708,'primitive_records':PRIMITIVE,
        'complex_original_base':0x104cd748,'complex_records':COMPLEX,
        'primitive_output_bytes':65,'complex_output_bytes':69,
        'unknown_index_consumer':'hard named frontier before any semantic access outside currently modeled domains',
        'original_record_padding_unmodeled_bytes':51}, 'unreviewed rank projection/domain')
    fields = {row['name']:row for row in model['fields']}
    for name, offset, defaults, output_name, width, base_offset in (
            ('primitive_type_ranks',0x1708,PRIMITIVE,'primitive_type_rank',65,4),
            ('complex_type_ranks',0x1748,COMPLEX,'complex_type_rank',69,68)):
        count = len(defaults)
        for i, pair in enumerate(defaults):
            for member, expected in zip(('code','rank'),pair):
                s.require(fields['%s[%d].%s'%(name,i,member)]['original_default']==expected,'guessed original record default')
        source = lambda suffix: records((original/('frontend-data-'+suffix+'.bin')).read_bytes(),offset,count)
        saved = lambda suffix: records((original/('frontend-snapshot-'+suffix+'.bin')).read_bytes(),offset,count)
        s.require(source('000')==defaults,'loaded original rank defaults differ')
        before = [[31337+i if name=='primitive_type_ranks' else 31437+i,200+i if name=='primitive_type_ranks' else 210+i] for i in range(count)] if seed else defaults
        copied = [[code,rank+40] for code,rank in defaults] if seed else defaults
        if seed:copied[0][0] = 0x20 if name=='primitive_type_ranks' else 5
        s.require(source('006')==pre['frontend'][name]==before,'rank-table distinct nonzero input differs')
        if seed:
            s.require(saved('006')==pre['frontend']['saved_'+name]==copied,'rank-table owned saved input differs')
        else:
            s.require(pre['frontend']['saved_'+name] is None,'rank-table snapshot exists before genuine allocation')
        for suffix, current in [('phase',phase),(final,state)]:
            s.require(source(suffix)==saved(suffix)==current['frontend'][name]==current['frontend']['saved_'+name]==copied,'typed rank snapshot/copy/restore differs')
            native_base=current['frontend']['storage']['first_invocation'][0]
            for i in range(count):
                for field,off,size in [('code',0,4),('rank',4,1)]:
                    s.require(current['frontend']['storage']['%s[%d].%s'%(name,i,field)]==[native_base+base_offset+i*8+off,size],'rank record storage/stride/alias differs')
        def output(suffix):
            raw=(original/(output_name.replace('_','-')+'-'+suffix+'.bin')).read_bytes()
            s.require(len(raw)==width,'missing raw rank output domain')
            return list(raw)
        s.require(output('000')==[0]*width,'original rank output defaults differ')
        wanted_pre=[181 if name=='primitive_type_ranks' else 182]*width if seed else [0]*width
        s.require(output('006')==pre['frontend'][output_name]==wanted_pre,'rank output nonzero prestate differs')
        s.require(output('phase')==phase['frontend'][output_name]==[0]*width,'rank output escaped BSS clear')
        result=[0]*width
        for code, rank in copied:
            if code==0:break
            index=code>>5 if name=='primitive_type_ranks' else code
            s.require(0<=index<width,'unmodeled rank output consumer')
            result[index]=rank
        s.require(output(final)==state['frontend'][output_name]==result,'rank mapping code/rank/termination/result differs')
    # No equality claim is made for 51 original record padding bytes, 8,431
    # unmodeled initialized bytes, or the native struct's own natural padding.
    s.require(len((original/'frontend-data-phase.bin').read_bytes())==8520 and
              (original/'frontend-data-phase.bin').read_bytes()==(original/'frontend-snapshot-phase.bin').read_bytes(),
              'complete original internal-transfer evidence missing')
