;
;
;guest-state�б���ļĴ�������,��vm-exit��ʱ��Ҫ��û�к��ǵ������Ĵ�����������
;
;
;
;
extern c_vmexit_handler:proc
.code

guest_context struct
  ; general-purpose registers
  $rax qword ?
  $rcx qword ?
  $rdx qword ?
  $rbx qword ?
  qword ? ; padding
  $rbp qword ?
  $rsi qword ?
  $rdi qword ?
  $r8  qword ?
  $r9  qword ?
  $r10 qword ?
  $r11 qword ?
  $r12 qword ?
  $r13 qword ?
  $r14 qword ?
  $r15 qword ?

  ; control registers
  $cr2 qword ?
  $cr8 qword ?

  ; debug registers
  $dr0 qword ?
  $dr1 qword ?
  $dr2 qword ?
  $dr3 qword ?
  $dr6 qword ?
  $xmm0 xmmword ?	;xmm0-xmm5�õ�Ƶ�ʸ�,һ������ʧ�Ե�
  $xmm1 xmmword ?
  $xmm2 xmmword ?
  $xmm3 xmmword ?
  $xmm4 xmmword ?
  $xmm5 xmmword ?


guest_context ends



vmexit_handler proc

	sub rsp,120h
	; general-purpose registers
    mov guest_context.$rax[rsp], rax
    mov guest_context.$rcx[rsp], rcx
    mov guest_context.$rdx[rsp], rdx
    mov guest_context.$rbx[rsp], rbx
    mov guest_context.$rbp[rsp], rbp
    mov guest_context.$rsi[rsp], rsi
    mov guest_context.$rdi[rsp], rdi
    mov guest_context.$r8[rsp],  r8
    mov guest_context.$r9[rsp],  r9
    mov guest_context.$r10[rsp], r10
    mov guest_context.$r11[rsp], r11
    mov guest_context.$r12[rsp], r12
    mov guest_context.$r13[rsp], r13
    mov guest_context.$r14[rsp], r14
    mov guest_context.$r15[rsp], r15

    ; control registers
    mov rax, cr2
    mov guest_context.$cr2[rsp], rax
    mov rax, cr8
    mov guest_context.$cr8[rsp], rax

      ; debug registers
    mov rax, dr0
    mov guest_context.$dr0[rsp], rax
    mov rax, dr1
    mov guest_context.$dr1[rsp], rax
    mov rax, dr2
    mov guest_context.$dr2[rsp], rax
    mov rax, dr3
    mov guest_context.$dr3[rsp], rax
    mov rax, dr6
    mov guest_context.$dr6[rsp], rax

    movaps guest_context.$xmm0[rsp],xmm0
    movaps guest_context.$xmm1[rsp],xmm1
    movaps guest_context.$xmm2[rsp],xmm2
    movaps guest_context.$xmm3[rsp],xmm3
    movaps guest_context.$xmm4[rsp],xmm4
    movaps guest_context.$xmm5[rsp],xmm5

    mov rcx,rsp ;��һ������

	sub rsp,28h
	call c_vmexit_handler
	add rsp,28h

	add rsp,120h

    cmp eax,0   ;���Ϊ0�Ļ�,�ر����⻯
    je @f
    vmresume
@@:            ;�ر����⻯
    

    ret

vmexit_handler endp

end