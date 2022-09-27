#include "vcpu.h"
#include "mm.h"
#include "utils.h"


u32 g_logical_processor;

cached_data _cached_data;

void cached_cpu_data() {
    __cpuid(reinterpret_cast<int*>(&_cached_data.cpuid_01), 1);
    _cached_data._cr0.flags = __readcr0();
    _cached_data._cr4.flags = __readcr4();
}

// ���VMX�ܷ񱻿���
bool check_hypervisor_can_enable() {

    // 3.23.6       ���cpu�����Ƿ�֧��VMX����
    if (!_cached_data.cpuid_01.cpuid_feature_information_ecx.virtual_machine_extensions)
        return false;

    ia32_feature_control_register msr;
    msr.flags = __readmsr(IA32_FEATURE_CONTROL);

    // 3.23.7
    if (!msr.lock_bit || !msr.enable_vmx_outside_smx)    //
        return false;

    return true;
}

// Ϊ��ǰ�߼����ṩvmcs�ṹ
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

//������vmxonָ�������
void enable_vmx_operation() {
    _disable();

    auto cr0 = _cached_data._cr0.flags;
    auto cr4 = _cached_data._cr4.flags;
    //print("[+] ori cr4 %x\n", cr4);

    // 3.23.7
    cr4 |= CR4_VMX_ENABLE_FLAG;

    // 3.23.8
    cr0 |= __readmsr(IA32_VMX_CR0_FIXED0);
    cr0 &= __readmsr(IA32_VMX_CR0_FIXED1);
    cr4 |= __readmsr(IA32_VMX_CR4_FIXED0);
    cr4 &= __readmsr(IA32_VMX_CR4_FIXED1);
    //print("[+] fixed cr4 %x\n", cr4);


    __writecr0(cr0);
    __writecr4(cr4);

    _enable();
}

u64 virtualize_everycpu_ipi_routine(u64 Argument) {

    enable_vmx_operation();         //ʹ��VMXON

    // ����ǰ���ķ���һ�ݵ�����hypervisor����
    hypervisor* t = (hypervisor*)ExAllocatePoolWithTag(NonPagedPool, sizeof(hypervisor), 'ssss');
    if (!t) {
        print("[E] cant alloc hypervisor struct\n");
        return 0;
    }

    char b;
    // __vmx_on�Ĳ�����Ҫ�ṩһ��vmxon�ṹ
    ia32_vmx_basic_register vmx_basic;
    vmx_basic.flags = __readmsr(IA32_VMX_BASIC);
    t->vmxon.revision_id = vmx_basic.vmcs_revision_id;
    t->vmxon.must_be_zero = 0;

    auto phyvmxon = get_physical(&t->vmxon);
    NT_ASSERT((__readcr4() & CR4_VMX_ENABLE_FLAG) != 0);
    NT_ASSERT((phyvmxon % 0x1000) == 0);

    b = __vmx_on(&phyvmxon);        //����vmx rootģʽ
    if (b) {    //������0,ʧ��
        print("[E] vmxon failed\n");
        return 0;
    }








    

    return 0;
}