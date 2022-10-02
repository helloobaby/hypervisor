#include "ia32-doc/out/ia32.hpp"
#include "utils.h"
#include "guest-context.h"

#include <intrin.h>

//wrapper
extern "C" bool c_vmexit_handler(guest_context* vcpu);
void dispatch_vm_exit(guest_context* vcpu, const vmx_vmexit_reason reason);
//

//handlers
void vmexit_handler_cpuid(guest_context* vcpu);
void vmexit_handler_msr_access(guest_context* vcpu,bool is_write = false);
void vmexit_handler_vmxoff(guest_context* vcpu);
//

//utils
void skip_instruction();
void inject_hw_exception(uint32_t const vector);
void inject_hw_exception(uint32_t const vector, uint32_t const error);
//


bool c_vmexit_handler(guest_context* vcpu) {
	
	// ��ȡvmexit��ԭ��
	vmx_vmexit_reason reason;
	__vmx_vmread(VMCS_EXIT_REASON,(uint64_t*)&reason.flags);

	// ����ǿͻ���Ҫ�ر����⻯(һ����ж��������ʱ��,ֱ�����⴦��)
	if (reason.flags == VMX_EXIT_REASON_EXECUTE_VMXOFF) {
		vmexit_handler_vmxoff(vcpu);
		return false;
	}



	dispatch_vm_exit(vcpu,reason);
	return true;
}

void dispatch_vm_exit(guest_context* vcpu,const vmx_vmexit_reason reason) {

	// ��ȡ����vm-exit��rip һ����windbg���Ե�ʱ��,�����ﲻʹ��
	// u  poi(_rip)
	// dt vcpu
	[[maybe_unused]] uint64_t _rip;
	__vmx_vmread(VMCS_GUEST_RIP, &_rip);

	switch (reason.basic_exit_reason)
	{
	case VMX_EXIT_REASON_EXECUTE_CPUID://cpuid������ô����vmcs field,��������������vmexit
		vmexit_handler_cpuid(vcpu);
		break;
	case VMX_EXIT_REASON_EXECUTE_RDMSR:
		vmexit_handler_msr_access(vcpu);
		break;
	case VMX_EXIT_REASON_EXECUTE_WRMSR:
		vmexit_handler_msr_access(vcpu, true);
		break;
	default:
		__debugbreak();
	}

}


void vmexit_handler_cpuid(guest_context* vcpu) {

#ifdef DBG
	// ����
	if (vcpu->eax == 0x11111111 || vcpu->eax == 11111111) {
		vcpu->ebx = 0x2222;
		vcpu->ecx = 0x3333;
		vcpu->edx = 0x4444;
		skip_instruction();
		return;
	}
#endif // DBG



	int regs[4];
	__cpuidex(regs, vcpu->eax, vcpu->ecx);

	vcpu->rax = regs[0];
	vcpu->rbx = regs[1];
	vcpu->rcx = regs[2];
	vcpu->rdx = regs[3];

	skip_instruction();
	return;
}

// ��������vm-exit������ָ��
void skip_instruction() {
	uint64_t old_rip;	
	uint64_t old_rip_length;
	__vmx_vmread(VMCS_GUEST_RIP,&old_rip);
	__vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &old_rip_length);
	auto new_rip = old_rip + old_rip_length;

	__vmx_vmwrite(VMCS_GUEST_RIP, new_rip);
}

void vmexit_handler_msr_access(guest_context* vcpu,bool is_write) {
	
	if (is_invalid_msr_access(vcpu->ecx)) {	//��ʹmsr bitmap������1,����reserved msr�ķ���Ҳ��(������)vm-exit
		// https://www.unknowncheats.me/forum/3425463-post15.html
		// intel3�� 31.10.5    
		// ��Ҫ����һ��#GP��guest
		// inject_hw_exception(general_protection, 0);
		skip_instruction();
		return;
	}
	else { //����msr access����
		
		//skip_instruction();
	}
}


// inject a vectored exception into the guest
void inject_hw_exception(uint32_t const vector) {
	vmentry_interrupt_information interrupt_info;
	interrupt_info.flags = 0;
	interrupt_info.vector = vector;
	interrupt_info.interruption_type = hardware_exception;
	interrupt_info.deliver_error_code = 0;
	interrupt_info.valid = 1;
	__vmx_vmwrite(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, interrupt_info.flags);
}

// inject a vectored exception into the guest (with an error code)
void inject_hw_exception(uint32_t const vector, uint32_t const error) {
	vmentry_interrupt_information interrupt_info;
	interrupt_info.flags = 0;
	interrupt_info.vector = vector;
	interrupt_info.interruption_type = hardware_exception;
	interrupt_info.deliver_error_code = 1;
	interrupt_info.valid = 1;
	__vmx_vmwrite(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, interrupt_info.flags);
	__vmx_vmwrite(VMCS_CTRL_VMENTRY_EXCEPTION_ERROR_CODE, error);
}

void vmexit_handler_vmxoff(guest_context* vcpu) {
	// ���񵽿ͻ���ִ��vmxoff,������Ҫ�ر����⻯

	__vmx_off();
	//skip_instruction();
}