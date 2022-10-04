;
;
;guest-state�б���ļĴ�������,��vm-exit��ʱ��Ҫ��û�к��ǵ������Ĵ�����������
;
;
;
;
extern c_vmexit_handler:proc
extern ?unload_hypervisor@@3_NA:dword ; bool���ͱ�����Ĭ�϶������ֽڷ��ʵ�
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
	call c_vmexit_handler   ;��Ϊ���C�������в�����,���ݾ���,���������Ҫ����0x28���ֽ�(������ֻ��Ҫ8���ֽ�)
	add rsp,28h


    ;������֮��ָ��Ĵ���
    mov rax, guest_context.$dr0[rsp]
    mov dr0, rax
    mov rax, guest_context.$dr1[rsp]
    mov dr1, rax
    mov rax, guest_context.$dr2[rsp]
    mov dr2, rax
    mov rax, guest_context.$dr3[rsp]
    mov dr3, rax
    mov rax, guest_context.$dr6[rsp]
    mov dr6, rax

  ; control registers
    mov rax, guest_context.$cr2[rsp]
    mov cr2, rax
    mov rax, guest_context.$cr8[rsp]
    mov cr8, rax

  ; general-purpose registers
    mov rax, guest_context.$rax[rsp]
    mov rcx, guest_context.$rcx[rsp]
    mov rdx, guest_context.$rdx[rsp]
    mov rbx, guest_context.$rbx[rsp]
    mov rbp, guest_context.$rbp[rsp]
    mov rsi, guest_context.$rsi[rsp]
    mov rdi, guest_context.$rdi[rsp]
    mov r8,  guest_context.$r8[rsp]
    mov r9,  guest_context.$r9[rsp]
    mov r10, guest_context.$r10[rsp]
    mov r11, guest_context.$r11[rsp]
    mov r12, guest_context.$r12[rsp]
    mov r13, guest_context.$r13[rsp]
    mov r14, guest_context.$r14[rsp]

    movaps xmm0,guest_context.$xmm0[rsp]
    movaps xmm1,guest_context.$xmm1[rsp]
    movaps xmm2,guest_context.$xmm2[rsp]
    movaps xmm3,guest_context.$xmm3[rsp]
    movaps xmm4,guest_context.$xmm4[rsp]
    movaps xmm5,guest_context.$xmm5[rsp] 
    
    cmp ?unload_hypervisor@@3_NA,1   ;���Ϊ����1�Ļ�,�ر����⻯
    je @f
    vmresume
@@:            ;�ر����⻯ 2������
               ;1.����ǰhost�Ļ������guest�Ļ���
               ;2.vmxoff
   
               
               
               















    ret

vmexit_handler endp

end