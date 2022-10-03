#include "ia32-doc/out/ia32.hpp"
#include "utils.h"
#include "guest-context.h"
#include "hypercalls.h"

#include <ntifs.h>
#include <ntddk.h>
#include <wdm.h>
#include "wmi_trace.h"
#include "vm_exit_handler.tmh"

#include <intrin.h>

bool unload_hypervisor = false;

//wrapper
extern "C" void c_vmexit_handler(guest_context* vcpu);
void dispatch_vm_exit(guest_context* vcpu, const vmx_vmexit_reason reason);
//

//handlers
void vmexit_handler_cpuid(guest_context* vcpu);
void vmexit_handler_msr_access(guest_context* vcpu,bool is_write = false);
void vmexit_handler_vmxoff(guest_context* vcpu);
void vmexit_handler_vmcall(guest_context* vcpu);
//

//utils
void skip_instruction();
void inject_hw_exception(uint32_t const vector);
void inject_hw_exception(uint32_t const vector, uint32_t const error);
//


void c_vmexit_handler(guest_context* vcpu) {
	
	// 读取vmexit的原因
	vmx_vmexit_reason reason;
	__vmx_vmread(VMCS_EXIT_REASON,(uint64_t*)&reason.flags);

	dispatch_vm_exit(vcpu,reason);

}

// 
void dispatch_vm_exit(guest_context* vcpu,const vmx_vmexit_reason reason) {

	// 读取发生vm-exit的rip 一般在windbg调试的时候看,代码里不使用
	// u  poi(_rip)
	// dt vcpu
	[[maybe_unused]] uint64_t _rip;
	__vmx_vmread(VMCS_GUEST_RIP, &_rip);

	// host进来的时候eflags中的IF位被自动清0了,我们需要把irql拉到最高
	//u32 i_effective = KeGetEffectiveIrql();				
	//u32 i = KeGetCurrentIrql();
	//TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[+] vmexit effective irql 0x%x current irql 0x%x\n", i_effective,i);

	KIRQL guest_irql;
	KeRaiseIrql(HIGH_LEVEL, &guest_irql);

	switch (reason.basic_exit_reason)
	{
	case VMX_EXIT_REASON_EXECUTE_CPUID://cpuid不管怎么设置vmcs field,都会无条件触发vmexit
		vmexit_handler_cpuid(vcpu);
		break;
	case VMX_EXIT_REASON_EXECUTE_RDMSR:
		vmexit_handler_msr_access(vcpu);
		break;
	case VMX_EXIT_REASON_EXECUTE_WRMSR:
		vmexit_handler_msr_access(vcpu, true);
		break;
	case VMX_EXIT_REASON_EXECUTE_VMCALL:
		vmexit_handler_vmcall(vcpu);
		break;
	case VMX_EXIT_REASON_EXECUTE_VMRESUME:

		break;
	default:
		__debugbreak();
	}

	KeLowerIrql(guest_irql);
}


void vmexit_handler_cpuid(guest_context* vcpu) {

#ifdef DBG
	// 测试
	if (vcpu->eax == 0x11111111 || vcpu->eax == 11111111) {
		vcpu->ebx = 0x2222;
		vcpu->ecx = 0x3333;
		vcpu->edx = 0x4444;
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[+] test cpuid \n");
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

	u32 i = KeGetCurrentIrql();
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[+] [%s] cpuid vmexit irql %d\n",get_current_process_name(),i);
	return;
}

// 跳过导致vm-exit的那条指令
void skip_instruction() {
	uint64_t old_rip;	
	uint64_t old_rip_length;
	__vmx_vmread(VMCS_GUEST_RIP,&old_rip);
	__vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &old_rip_length);
	auto new_rip = old_rip + old_rip_length;

	__vmx_vmwrite(VMCS_GUEST_RIP, new_rip);
}

void vmexit_handler_msr_access(guest_context* vcpu,bool is_write) {
	
	if (is_invalid_msr_access(vcpu->ecx)) {	//即使msr bitmap不设置1,这种reserved msr的访问也会(无条件)vm-exit
		// https://www.unknowncheats.me/forum/3425463-post15.html
		// intel3卷 31.10.5    
		// 需要反射一个#GP给guest
		// inject_hw_exception(general_protection, 0);
		skip_instruction();
		return;
	}
	else { //正常msr access命中
		
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

	// 捕获到客户机执行vmxoff,
	// 1.简单的跳过这条指令
	// 2.注入异常(隐藏hypervisor)

	skip_instruction();
}


void vmexit_handler_vmcall(guest_context* vcpu) {
	if (vcpu->rax == hypercall_code::unload) {	//卸载虚拟化
		unload_hypervisor = true;
	}
}