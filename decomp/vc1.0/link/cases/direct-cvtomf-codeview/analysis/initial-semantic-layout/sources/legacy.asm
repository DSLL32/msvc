; Declarative fixture instructions/data. The Python writer emits OMF records.
.386
.model flat
.code
_legacy_identity:
    push ebp
    mov ebp,esp
    mov eax,[ebp+8]
    nop
    leave
    ret
    align 16

_legacy_value:
    push ebp
    mov ebp,esp
    sub esp,4
    mov dword ptr [ebp-4],42
    mov eax,[ebp-4]
epilogue:
    leave
    ret
.data
_legacy_nodes:
    dd 19,0,2,OFFSET _legacy_nodes+16
    dd 23,0,4,OFFSET _legacy_nodes+32
    dd 31,0,6,OFFSET _legacy_nodes+48
    dd 37,0,8,0
_legacy_head dd OFFSET _legacy_nodes
_legacy_calls dd OFFSET _legacy_value,OFFSET _legacy_value
_legacy_numbers dd 2,4,6,8
_legacy_refs dd OFFSET _legacy_nodes,OFFSET _legacy_nodes+32
END
