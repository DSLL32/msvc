
../../../toolchains/1.0-nt/bin/C13232.EXE:     file format pei-i386


Disassembly of section .text:

00033750 <.text+0x1c750>:
   33750:	56                   	push   esi
   33751:	57                   	push   edi
   33752:	8b 74 24 0c          	mov    esi,DWORD PTR [esp+0xc]
   33756:	8d 7e 10             	lea    edi,[esi+0x10]
   33759:	83 3f 00             	cmp    DWORD PTR [edi],0x0
   3375c:	75 3e                	jne    0x3379c
   3375e:	81 3d f4 b7 06 00 ff 	cmp    DWORD PTR ds:0x6b7f4,0x7fffffff
   33765:	ff ff 7f 
   33768:	72 0a                	jb     0x33774
   3376a:	6a 37                	push   0x37
   3376c:	e8 5f 73 01 00       	call   0x4aad0
   33771:	83 c4 04             	add    esp,0x4
   33774:	a1 f8 b7 06 00       	mov    eax,ds:0x6b7f8
   33779:	39 05 f4 b7 06 00    	cmp    DWORD PTR ds:0x6b7f4,eax
   3377f:	72 0e                	jb     0x3378f
   33781:	83 3d 88 b5 05 00 00 	cmp    DWORD PTR ds:0x5b588,0x0
   33788:	74 05                	je     0x3378f
   3378a:	e8 b1 3d ff ff       	call   0x27540
   3378f:	a1 f4 b7 06 00       	mov    eax,ds:0x6b7f4
   33794:	ff 05 f4 b7 06 00    	inc    DWORD PTR ds:0x6b7f4
   3379a:	89 07                	mov    DWORD PTR [edi],eax
   3379c:	80 4e 14 04          	or     BYTE PTR [esi+0x14],0x4
   337a0:	56                   	push   esi
   337a1:	e8 0a c6 ff ff       	call   0x2fdb0
   337a6:	8b 46 0c             	mov    eax,DWORD PTR [esi+0xc]
   337a9:	f6 40 01 04          	test   BYTE PTR [eax+0x1],0x4
   337ad:	75 31                	jne    0x337e0
   337af:	8b 40 04             	mov    eax,DWORD PTR [eax+0x4]
   337b2:	0b c0                	or     eax,eax
   337b4:	74 15                	je     0x337cb
   337b6:	f6 00 07             	test   BYTE PTR [eax],0x7
   337b9:	75 03                	jne    0x337be
   337bb:	8b 40 04             	mov    eax,DWORD PTR [eax+0x4]
   337be:	0b c0                	or     eax,eax
   337c0:	74 09                	je     0x337cb
   337c2:	66 8b 00             	mov    ax,WORD PTR [eax]
   337c5:	24 07                	and    al,0x7
   337c7:	3c 02                	cmp    al,0x2
   337c9:	74 15                	je     0x337e0
   337cb:	6a 00                	push   0x0
   337cd:	e8 1e 41 ff ff       	call   0x278f0
   337d2:	50                   	push   eax
   337d3:	56                   	push   esi
   337d4:	e8 87 c6 ff ff       	call   0x2fe60
   337d9:	5f                   	pop    edi
   337da:	5e                   	pop    esi
   337db:	c2 04 00             	ret    0x4
   337de:	8b ff                	mov    edi,edi
   337e0:	56                   	push   esi
   337e1:	e8 da c8 ff ff       	call   0x300c0
   337e6:	6a 00                	push   0x0
   337e8:	e8 03 41 ff ff       	call   0x278f0
   337ed:	50                   	push   eax
   337ee:	e8 ad c9 ff ff       	call   0x301a0
   337f3:	56                   	push   esi
   337f4:	e8 07 da ff ff       	call   0x31200
   337f9:	5f                   	pop    edi
   337fa:	5e                   	pop    esi
   337fb:	c2 04 00             	ret    0x4
