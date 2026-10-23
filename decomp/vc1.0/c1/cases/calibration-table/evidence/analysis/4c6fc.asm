
../../../toolchains/1.0-nt/bin/C13232.EXE:     file format pei-i386


Disassembly of section .text:

0004c6fc <.text+0x356fc>:
   4c6fc:	55                   	push   ebp
   4c6fd:	8b ec                	mov    ebp,esp
   4c6ff:	53                   	push   ebx
   4c700:	56                   	push   esi
   4c701:	57                   	push   edi
   4c702:	8b 7d 08             	mov    edi,DWORD PTR [ebp+0x8]
   4c705:	db 07                	fild   DWORD PTR [edi]
   4c707:	8b 75 0c             	mov    esi,DWORD PTR [ebp+0xc]
   4c70a:	66 c7 46 0a 04 00    	mov    WORD PTR [esi+0xa],0x4
   4c710:	e8 dc 01 00 00       	call   0x4c8f1
   4c715:	33 c0                	xor    eax,eax
   4c717:	5f                   	pop    edi
   4c718:	5e                   	pop    esi
   4c719:	5b                   	pop    ebx
   4c71a:	8b e5                	mov    esp,ebp
   4c71c:	5d                   	pop    ebp
   4c71d:	c3                   	ret
   4c71e:	55                   	push   ebp
   4c71f:	8b ec                	mov    ebp,esp
   4c721:	53                   	push   ebx
   4c722:	56                   	push   esi
   4c723:	57                   	push   edi
   4c724:	66 33 c9             	xor    cx,cx
   4c727:	66 51                	push   cx
   4c729:	66 51                	push   cx
   4c72b:	8b 7d 08             	mov    edi,DWORD PTR [ebp+0x8]
   4c72e:	66 ff 77 02          	push   WORD PTR [edi+0x2]
   4c732:	66 ff 37             	push   WORD PTR [edi]
   4c735:	8b fc                	mov    edi,esp
   4c737:	df 2f                	fild   QWORD PTR [edi]
   4c739:	8b 75 0c             	mov    esi,DWORD PTR [ebp+0xc]
   4c73c:	66 c7 46 0a 04 00    	mov    WORD PTR [esi+0xa],0x4
   4c742:	83 c4 08             	add    esp,0x8
   4c745:	e8 a7 01 00 00       	call   0x4c8f1
   4c74a:	33 c0                	xor    eax,eax
   4c74c:	5f                   	pop    edi
   4c74d:	5e                   	pop    esi
   4c74e:	5b                   	pop    ebx
   4c74f:	8b e5                	mov    esp,ebp
   4c751:	5d                   	pop    ebp
   4c752:	c3                   	ret
