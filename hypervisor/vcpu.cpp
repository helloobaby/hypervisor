#include "vcpu.h"
#include "mm.h"

u32 g_logical_processor;

// 检测VMX能否被开启
bool check_hypervisor_can_enable() {
    cpuid_eax_01 cpuid;
    __cpuid(reinterpret_cast<int*>(&cpuid), 1);

    // 3.23.6       检测cpu本身是否支持VMX技术
    if (!cpuid.cpuid_feature_information_ecx.virtual_machine_extensions)
        return false;

    ia32_feature_control_register msr;
    msr.flags = __readmsr(IA32_FEATURE_CONTROL);

    // 3.23.7
    if (!msr.lock_bit || !msr.enable_vmx_outside_smx)    //
        return false;

    return true;
}

// 为当前逻辑核提供vmcs结构
bool prepare_vmcs(vmcs& vmcs) {
    ia32_vmx_basic_register vmx_basic;
    vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

    // 3.24.2
    vmcs.revision_id = vmx_basic.vmcs_revision_id;
    vmcs.shadow_vmcs_indicator = 0;

    auto vmcs_phys = get_physical(&vmcs);
    NT_ASSERT(vmcs_phys % 0x1000 == 0);

    if (__vmx_vmclear(&vmcs_phys) != 0)
        return false;

    if (__vmx_vmptrld(&vmcs_phys) != 0)
        return false;

    return true;
}