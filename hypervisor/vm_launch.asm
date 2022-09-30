.code

; bool __vm_launch();
vm_launch proc
	  ; set VMCS_GUEST_RSP to the current value of RSP
  mov rax, 681Ch
  vmwrite rax, rsp

  ; set VMCS_GUEST_RIP to the address of <successful_launch>
  mov rax, 681Eh
  mov rdx, successful_launch
  vmwrite rax, rdx

  vmlaunch

  ; if we reached here, then we failed to launch
  xor al, al
  ret

successful_launch:
  int 3                ;����(�ж�vmlaunch�Ƿ�ɹ�,����ɹ��˵Ļ���������ϵ�����)
  mov al, 1
  ret
vm_launch endp

end