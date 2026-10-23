
../../../toolchains/1.0-nt/bin/C13232.EXE:     file format pei-i386


Disassembly of section .text:

00030fd0 <.text+0x19fd0>:
   30fd0:	53                   	push   ebx
   30fd1:	56                   	push   esi
   30fd2:	8b 44 24 0c          	mov    eax,DWORD PTR [esp+0xc]
   30fd6:	be 01 00 00 00       	mov    esi,0x1
   30fdb:	8b 40 04             	mov    eax,DWORD PTR [eax+0x4]
   30fde:	0b c0                	or     eax,eax
   30fe0:	74 44                	je     0x31026
   30fe2:	ba 10 00 00 00       	mov    edx,0x10
   30fe7:	bb 08 00 00 00       	mov    ebx,0x8
   30fec:	0b f6                	or     esi,esi
   30fee:	7e 36                	jle    0x31026
   30ff0:	8a 08                	mov    cl,BYTE PTR [eax]
   30ff2:	80 e1 1f             	and    cl,0x1f
   30ff5:	80 f9 01             	cmp    cl,0x1
   30ff8:	72 25                	jb     0x3101f
   30ffa:	80 f9 0d             	cmp    cl,0xd
   30ffd:	77 0b                	ja     0x3100a
   30fff:	80 f9 05             	cmp    cl,0x5
   31002:	74 06                	je     0x3100a
   31004:	84 50 15             	test   BYTE PTR [eax+0x15],dl
   31007:	74 01                	je     0x3100a
   31009:	4e                   	dec    esi
   3100a:	80 f9 01             	cmp    cl,0x1
   3100d:	72 10                	jb     0x3101f
   3100f:	80 f9 0d             	cmp    cl,0xd
   31012:	77 0b                	ja     0x3101f
   31014:	80 f9 05             	cmp    cl,0x5
   31017:	74 06                	je     0x3101f
   31019:	84 58 15             	test   BYTE PTR [eax+0x15],bl
   3101c:	74 01                	je     0x3101f
   3101e:	46                   	inc    esi
   3101f:	8b 40 04             	mov    eax,DWORD PTR [eax+0x4]
   31022:	0b c0                	or     eax,eax
   31024:	75 c6                	jne    0x30fec
   31026:	5e                   	pop    esi
   31027:	5b                   	pop    ebx
   31028:	c2 04 00             	ret    0x4
