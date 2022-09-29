//https://github.com/torvalds/linux/blob/c6dd78fcb8eefa15dd861889e0f59d301cb5230c/tools/testing/selftests/kvm/lib/x86_64/vmx.c#L1

#include "vcpu.h"
#include "mm.h"
#include "utils.h"
#include "arch.h"
#include "vmcs_helper.h"

u32 g_logical_processor;

cached_data _cached_data;

extern "C" {
    void vmexit_handler(void);
}

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

bool load_vmcs(vmcs& vmcs) {
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

void prepare_vmcs() {
    init_vmcs_control_fields();
    init_vmcs_host_state();
    init_vmcs_guest_state();
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

#ifdef DBG
    //��Ҫ����fixed �޸�����Щλ
    print("[+] ori cr0 %d  now cr0 %d\n", _cached_data._cr0, __readcr0());
    print("[+] ori cr4 %d  now cr4 %d\n", _cached_data._cr4, __readcr4());
#endif
}


void init_vmcs_control_fields() {

    // Pin-based VM-Excution control�ֶ�
    // intel��3 A.3.1
    ia32_vmx_pinbased_ctls_register pin_based_ctrl;
    pin_based_ctrl.flags = 0;                               //0��ʼ��  
    write_ctrl_pin_based_safe(pin_based_ctrl);

    // Processor-Based VM-Execution Controls
    // intel��3 24.6.2
    ia32_vmx_procbased_ctls_register proc_based_ctrl;
    proc_based_ctrl.flags = 0;
    write_ctrl_proc_based_safe(proc_based_ctrl);

    //����proc_based_ctrl.activate_secondary_controlsδ����,���������ⲿ��д��д����
    ia32_vmx_procbased_ctls2_register proc_based_ctrl2;
    proc_based_ctrl2.flags = 0;
    write_ctrl_proc_based2_safe(proc_based_ctrl2);

    ia32_vmx_exit_ctls_register exit_ctrl;
    exit_ctrl.flags = 0;
    write_ctrl_exit_safe(exit_ctrl);

    ia32_vmx_entry_ctls_register entry_ctrl;
    entry_ctrl.flags = 0;
    write_ctrl_entry_safe(entry_ctrl);

}

// https://github.com/torvalds/linux/blob/c6dd78fcb8eefa15dd861889e0f59d301cb5230c/tools/testing/selftests/kvm/lib/x86_64/vmx.c#L195
// ���������⻯����3.9
void init_vmcs_host_state() {

    u64 exit_controls;      //ʵ������32λ���ֶ�
    __vmx_vmread(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS,&exit_controls);
    print("[+] exit_controls %d\n",(u32)exit_controls);

    //host�μĴ���
    NT_ASSERT(__vmx_vmwrite(VMCS_HOST_ES_SELECTOR, x86::read_es().flags) == 0);
    __vmx_vmwrite(VMCS_HOST_CS_SELECTOR, x86::read_cs().flags);
    __vmx_vmwrite(VMCS_HOST_SS_SELECTOR, x86::read_ss().flags);
    __vmx_vmwrite(VMCS_HOST_DS_SELECTOR, x86::read_ds().flags);
    __vmx_vmwrite(VMCS_HOST_FS_SELECTOR, x86::read_fs().flags);
    __vmx_vmwrite(VMCS_HOST_GS_SELECTOR, x86::read_gs().flags);
    __vmx_vmwrite(VMCS_HOST_TR_SELECTOR, x86::read_tr().flags);

    //host���ƼĴ���
    __vmx_vmwrite(VMCS_HOST_CR0, __readcr0());
    __vmx_vmwrite(VMCS_HOST_CR3, __readcr3());
    __vmx_vmwrite(VMCS_HOST_CR4, __readcr4());

    // hostָ��ָ��
    __vmx_vmwrite(VMCS_HOST_RIP, (u64)(&vmexit_handler));
    // ����KERNEL_STACK_SIZE�ڴ��host����ջ

    //hostջָ��
    void* host_stack = ExAllocatePool(NonPagedPool, KERNEL_STACK_SIZE);
    __vmx_vmwrite(VMCS_HOST_RSP, (size_t)host_stack);

    NT_ASSERT(host_stack != 0);
    print("[+][core%x] host_stack %p\n", KeGetCurrentProcessorNumber(),host_stack);

    //host���msr
    __vmx_vmwrite(VMCS_HOST_SYSENTER_CS, 0);
    __vmx_vmwrite(VMCS_HOST_SYSENTER_ESP, 0);
    __vmx_vmwrite(VMCS_HOST_SYSENTER_EIP, 0);
    
    //host�λ�ַ
    __vmx_vmwrite(VMCS_HOST_FS_BASE, __readmsr(IA32_FS_BASE));
    __vmx_vmwrite(VMCS_HOST_GS_BASE, __readmsr(IA32_GS_BASE));
    
    segment_descriptor_register_64 idt;
    __sidt(&idt);

    segment_descriptor_register_64 gdt;

    // https://learn.microsoft.com/en-us/cpp/intrinsics/x64-amd64-intrinsics-list?view=msvc-170
    // ����url�м���_sgdt,����intrin.h��û���������������,����������һ��
    void _sgdt(void*);  
    _sgdt(&gdt);

    __vmx_vmwrite(VMCS_HOST_GDTR_BASE,gdt.base_address);
    __vmx_vmwrite(VMCS_HOST_IDTR_BASE,idt.base_address);
    //__vmx_vmwrite(VMCS_HOST_TR_BASE, reinterpret_cast<size_t>(&cpu->host_tss));

}

void init_vmcs_guest_state() {

}

u64 virtualize_everycpu_ipi_routine(u64 Argument) {

    NT_ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);        //��֤�޷��߳��л�

    print("[+] virtualize_everycpu_ipi_routine run core %d\n", KeGetCurrentProcessorNumber());

    enable_vmx_operation();         //ʹ��VMXON

    // ����ǰ���ķ���һ�ݵ�����hypervisor����
    hypervisor* t = (hypervisor*)ExAllocatePool(NonPagedPool, sizeof(hypervisor));
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

    b = __vmx_on(&phyvmxon);        //����vmx rootģʽ      (vmx_on���λᵼ��ʧ��,������vmxoff)
    if (b) {    //������0,ʧ��
        print("[E] vmxon failed\n");
        return 0;
    }

    load_vmcs(t->vmcs);     //��ִ������Ļ�vmread\vmwrite���ᱨ��
    prepare_vmcs();

    
    




    
    

    return 0;
}

u64 stop_virtualize_everycpu_ipi_routine(u64 Argument) {

    __vmx_off();

    return 0;
}


void stop_hypervisor() {
    KeIpiGenericCall(stop_virtualize_everycpu_ipi_routine, NULL);
}

