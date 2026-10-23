
../../../toolchains/1.0-nt/bin/C13232.EXE:     file format pei-i386


Disassembly of section .text:

0005a850 <.text+0x43850>:
   5a850:	83 ec 44             	sub    esp,0x44
   5a853:	53                   	push   ebx
   5a854:	56                   	push   esi
   5a855:	57                   	push   edi
   5a856:	55                   	push   ebp
   5a857:	2b ff                	sub    edi,edi
   5a859:	8b 6c 24 58          	mov    ebp,DWORD PTR [esp+0x58]
   5a85d:	39 2d e8 3a 07 00    	cmp    DWORD PTR ds:0x73ae8,ebp
   5a863:	77 1b                	ja     0x5a880
   5a865:	c7 05 cc 39 07 00 09 	mov    DWORD PTR ds:0x739cc,0x9
   5a86c:	00 00 00 
   5a86f:	b8 ff ff ff ff       	mov    eax,0xffffffff
   5a874:	5d                   	pop    ebp
   5a875:	5f                   	pop    edi
   5a876:	5e                   	pop    esi
   5a877:	5b                   	pop    ebx
   5a878:	83 c4 44             	add    esp,0x44
   5a87b:	c3                   	ret
   5a87c:	8d 64 24 00          	lea    esp,[esp+0x0]
   5a880:	8d 1c ad 30 3b 07 00 	lea    ebx,[ebp*4+0x73b30]
   5a887:	8b 03                	mov    eax,DWORD PTR [ebx]
   5a889:	50                   	push   eax
   5a88a:	e8 e5 fd ff ff       	call   0x5a674
   5a88f:	25 ff 7f ff ff       	and    eax,0xffff7fff
   5a894:	83 f8 01             	cmp    eax,0x1
   5a897:	0f 84 a3 00 00 00    	je     0x5a940
   5a89d:	83 f8 02             	cmp    eax,0x2
   5a8a0:	74 44                	je     0x5a8e6
   5a8a2:	83 f8 03             	cmp    eax,0x3
   5a8a5:	74 3a                	je     0x5a8e1
   5a8a7:	0b c0                	or     eax,eax
   5a8a9:	75 19                	jne    0x5a8c4
   5a8ab:	c7 05 cc 39 07 00 09 	mov    DWORD PTR ds:0x739cc,0x9
   5a8b2:	00 00 00 
   5a8b5:	bf ff ff ff ff       	mov    edi,0xffffffff
   5a8ba:	8b c7                	mov    eax,edi
   5a8bc:	5d                   	pop    ebp
   5a8bd:	5f                   	pop    edi
   5a8be:	5e                   	pop    esi
   5a8bf:	5b                   	pop    ebx
   5a8c0:	83 c4 44             	add    esp,0x44
   5a8c3:	c3                   	ret
   5a8c4:	e8 99 fd ff ff       	call   0x5a662
   5a8c9:	50                   	push   eax
   5a8ca:	e8 91 b1 ff ff       	call   0x55a60
   5a8cf:	83 c4 04             	add    esp,0x4
   5a8d2:	bf ff ff ff ff       	mov    edi,0xffffffff
   5a8d7:	8b c7                	mov    eax,edi
   5a8d9:	5d                   	pop    ebp
   5a8da:	5f                   	pop    edi
   5a8db:	5e                   	pop    esi
   5a8dc:	5b                   	pop    ebx
   5a8dd:	83 c4 44             	add    esp,0x44
   5a8e0:	c3                   	ret
   5a8e1:	83 f8 02             	cmp    eax,0x2
   5a8e4:	75 0c                	jne    0x5a8f2
   5a8e6:	8b 74 24 5c          	mov    esi,DWORD PTR [esp+0x5c]
   5a8ea:	66 c7 46 06 00 20    	mov    WORD PTR [esi+0x6],0x2000
   5a8f0:	eb 0a                	jmp    0x5a8fc
   5a8f2:	8b 74 24 5c          	mov    esi,DWORD PTR [esp+0x5c]
   5a8f6:	66 c7 46 06 00 10    	mov    WORD PTR [esi+0x6],0x1000
   5a8fc:	89 2e                	mov    DWORD PTR [esi],ebp
   5a8fe:	89 6e 10             	mov    DWORD PTR [esi+0x10],ebp
   5a901:	66 c7 46 08 01 00    	mov    WORD PTR [esi+0x8],0x1
   5a907:	66 c7 46 04 00 00    	mov    WORD PTR [esi+0x4],0x0
   5a90d:	8b c7                	mov    eax,edi
   5a90f:	66 c7 46 0c 00 00    	mov    WORD PTR [esi+0xc],0x0
   5a915:	66 c7 46 0a 00 00    	mov    WORD PTR [esi+0xa],0x0
   5a91b:	c7 46 14 00 00 00 00 	mov    DWORD PTR [esi+0x14],0x0
   5a922:	c7 46 20 00 00 00 00 	mov    DWORD PTR [esi+0x20],0x0
   5a929:	c7 46 1c 00 00 00 00 	mov    DWORD PTR [esi+0x1c],0x0
   5a930:	c7 46 18 00 00 00 00 	mov    DWORD PTR [esi+0x18],0x0
   5a937:	5d                   	pop    ebp
   5a938:	5f                   	pop    edi
   5a939:	5e                   	pop    esi
   5a93a:	5b                   	pop    ebx
   5a93b:	83 c4 44             	add    esp,0x44
   5a93e:	c3                   	ret
   5a93f:	90                   	nop
   5a940:	8b 74 24 5c          	mov    esi,DWORD PTR [esp+0x5c]
   5a944:	8d 44 24 20          	lea    eax,[esp+0x20]
   5a948:	66 c7 46 06 00 00    	mov    WORD PTR [esi+0x6],0x0
   5a94e:	66 c7 46 0c 00 00    	mov    WORD PTR [esi+0xc],0x0
   5a954:	66 c7 46 0a 00 00    	mov    WORD PTR [esi+0xa],0x0
   5a95a:	66 c7 46 04 00 00    	mov    WORD PTR [esi+0x4],0x0
   5a960:	66 c7 46 08 01 00    	mov    WORD PTR [esi+0x8],0x1
   5a966:	50                   	push   eax
   5a967:	8b 03                	mov    eax,DWORD PTR [ebx]
   5a969:	50                   	push   eax
   5a96a:	e8 7d 01 00 00       	call   0x5aaec
   5a96f:	0b c0                	or     eax,eax
   5a971:	75 1d                	jne    0x5a990
   5a973:	e8 ea fc ff ff       	call   0x5a662
   5a978:	50                   	push   eax
   5a979:	e8 e2 b0 ff ff       	call   0x55a60
   5a97e:	83 c4 04             	add    esp,0x4
   5a981:	bf ff ff ff ff       	mov    edi,0xffffffff
   5a986:	8b c7                	mov    eax,edi
   5a988:	5d                   	pop    ebp
   5a989:	5f                   	pop    edi
   5a98a:	5e                   	pop    esi
   5a98b:	5b                   	pop    ebx
   5a98c:	83 c4 44             	add    esp,0x44
   5a98f:	c3                   	ret
   5a990:	f6 44 24 20 01       	test   BYTE PTR [esp+0x20],0x1
   5a995:	74 09                	je     0x5a9a0
   5a997:	66 81 4e 06 24 01    	or     WORD PTR [esi+0x6],0x124
   5a99d:	eb 07                	jmp    0x5a9a6
   5a99f:	90                   	nop
   5a9a0:	66 81 4e 06 b6 01    	or     WORD PTR [esi+0x6],0x1b6
   5a9a6:	8d 44 24 10          	lea    eax,[esp+0x10]
   5a9aa:	50                   	push   eax
   5a9ab:	8d 44 24 38          	lea    eax,[esp+0x38]
   5a9af:	50                   	push   eax
   5a9b0:	e8 13 fd ff ff       	call   0x5a6c8
   5a9b5:	8b 44 24 1c          	mov    eax,DWORD PTR [esp+0x1c]
   5a9b9:	25 ff ff 00 00       	and    eax,0xffff
   5a9be:	50                   	push   eax
   5a9bf:	33 c0                	xor    eax,eax
   5a9c1:	66 8b 44 24 1e       	mov    ax,WORD PTR [esp+0x1e]
   5a9c6:	50                   	push   eax
   5a9c7:	8b 44 24 20          	mov    eax,DWORD PTR [esp+0x20]
   5a9cb:	25 ff ff 00 00       	and    eax,0xffff
   5a9d0:	50                   	push   eax
   5a9d1:	33 c0                	xor    eax,eax
   5a9d3:	66 8b 44 24 22       	mov    ax,WORD PTR [esp+0x22]
   5a9d8:	50                   	push   eax
   5a9d9:	33 c0                	xor    eax,eax
   5a9db:	66 8b 44 24 22       	mov    ax,WORD PTR [esp+0x22]
   5a9e0:	50                   	push   eax
   5a9e1:	8b 44 24 24          	mov    eax,DWORD PTR [esp+0x24]
   5a9e5:	25 ff ff 00 00       	and    eax,0xffff
   5a9ea:	50                   	push   eax
   5a9eb:	e8 40 cf ff ff       	call   0x57930
   5a9f0:	83 c4 18             	add    esp,0x18
   5a9f3:	89 46 1c             	mov    DWORD PTR [esi+0x1c],eax
   5a9f6:	83 7c 24 2c 00       	cmp    DWORD PTR [esp+0x2c],0x0
   5a9fb:	75 09                	jne    0x5aa06
   5a9fd:	83 7c 24 30 00       	cmp    DWORD PTR [esp+0x30],0x0
   5aa02:	75 02                	jne    0x5aa06
   5aa04:	eb 4d                	jmp    0x5aa53
   5aa06:	8d 44 24 10          	lea    eax,[esp+0x10]
   5aa0a:	50                   	push   eax
   5aa0b:	8d 44 24 30          	lea    eax,[esp+0x30]
   5aa0f:	50                   	push   eax
   5aa10:	e8 b3 fc ff ff       	call   0x5a6c8
   5aa15:	8b 44 24 1c          	mov    eax,DWORD PTR [esp+0x1c]
   5aa19:	25 ff ff 00 00       	and    eax,0xffff
   5aa1e:	50                   	push   eax
   5aa1f:	33 c0                	xor    eax,eax
   5aa21:	66 8b 44 24 1e       	mov    ax,WORD PTR [esp+0x1e]
   5aa26:	50                   	push   eax
   5aa27:	8b 44 24 20          	mov    eax,DWORD PTR [esp+0x20]
   5aa2b:	25 ff ff 00 00       	and    eax,0xffff
   5aa30:	50                   	push   eax
   5aa31:	33 c0                	xor    eax,eax
   5aa33:	66 8b 44 24 22       	mov    ax,WORD PTR [esp+0x22]
   5aa38:	50                   	push   eax
   5aa39:	33 c0                	xor    eax,eax
   5aa3b:	66 8b 44 24 22       	mov    ax,WORD PTR [esp+0x22]
   5aa40:	50                   	push   eax
   5aa41:	8b 44 24 24          	mov    eax,DWORD PTR [esp+0x24]
   5aa45:	25 ff ff 00 00       	and    eax,0xffff
   5aa4a:	50                   	push   eax
   5aa4b:	e8 e0 ce ff ff       	call   0x57930
   5aa50:	83 c4 18             	add    esp,0x18
   5aa53:	89 46 18             	mov    DWORD PTR [esi+0x18],eax
   5aa56:	83 7c 24 24 00       	cmp    DWORD PTR [esp+0x24],0x0
   5aa5b:	75 0c                	jne    0x5aa69
   5aa5d:	83 7c 24 28 00       	cmp    DWORD PTR [esp+0x28],0x0
   5aa62:	75 05                	jne    0x5aa69
   5aa64:	8b 46 1c             	mov    eax,DWORD PTR [esi+0x1c]
   5aa67:	eb 4d                	jmp    0x5aab6
   5aa69:	8d 44 24 10          	lea    eax,[esp+0x10]
   5aa6d:	50                   	push   eax
   5aa6e:	8d 44 24 28          	lea    eax,[esp+0x28]
   5aa72:	50                   	push   eax
   5aa73:	e8 50 fc ff ff       	call   0x5a6c8
   5aa78:	8b 44 24 1c          	mov    eax,DWORD PTR [esp+0x1c]
   5aa7c:	25 ff ff 00 00       	and    eax,0xffff
   5aa81:	50                   	push   eax
   5aa82:	33 c0                	xor    eax,eax
   5aa84:	66 8b 44 24 1e       	mov    ax,WORD PTR [esp+0x1e]
   5aa89:	50                   	push   eax
   5aa8a:	8b 44 24 20          	mov    eax,DWORD PTR [esp+0x20]
   5aa8e:	25 ff ff 00 00       	and    eax,0xffff
   5aa93:	50                   	push   eax
   5aa94:	33 c0                	xor    eax,eax
   5aa96:	66 8b 44 24 22       	mov    ax,WORD PTR [esp+0x22]
   5aa9b:	50                   	push   eax
   5aa9c:	33 c0                	xor    eax,eax
   5aa9e:	66 8b 44 24 22       	mov    ax,WORD PTR [esp+0x22]
   5aaa3:	50                   	push   eax
   5aaa4:	8b 44 24 24          	mov    eax,DWORD PTR [esp+0x24]
   5aaa8:	25 ff ff 00 00       	and    eax,0xffff
   5aaad:	50                   	push   eax
   5aaae:	e8 7d ce ff ff       	call   0x57930
   5aab3:	83 c4 18             	add    esp,0x18
   5aab6:	89 46 20             	mov    DWORD PTR [esi+0x20],eax
   5aab9:	8b 44 24 44          	mov    eax,DWORD PTR [esp+0x44]
   5aabd:	89 46 14             	mov    DWORD PTR [esi+0x14],eax
   5aac0:	66 81 4e 06 00 80    	or     WORD PTR [esi+0x6],0x8000
   5aac6:	8b c7                	mov    eax,edi
   5aac8:	c7 06 00 00 00 00    	mov    DWORD PTR [esi],0x0
   5aace:	c7 46 10 00 00 00 00 	mov    DWORD PTR [esi+0x10],0x0
   5aad5:	5d                   	pop    ebp
   5aad6:	5f                   	pop    edi
   5aad7:	5e                   	pop    esi
   5aad8:	5b                   	pop    ebx
   5aad9:	83 c4 44             	add    esp,0x44
   5aadc:	c3                   	ret
   5aadd:	8d 49 00             	lea    ecx,[ecx+0x0]
