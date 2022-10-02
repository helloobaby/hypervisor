//https://github.com/torvalds/linux/blob/c6dd78fcb8eefa15dd861889e0f59d301cb5230c/tools/testing/selftests/kvm/lib/x86_64/vmx.c#L1

#include "vcpu.h"
#include "mm.h"
#include "utils.h"
#include "arch.h"
#include "vmcs_helper.h"
#include "segment.h"
#include "hypercalls.h"

u32 g_logical_processor;

cached_data _cached_data;

extern "C" {
    void vmexit_handler(void);
    u64 vm_launch(void);
}
uint64_t vmx_vmcall(hypercall_input& input);

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

    // 3.23.7
    cr4 |= CR4_VMX_ENABLE_FLAG;

    // 3.23.8
    cr0 |= __readmsr(IA32_VMX_CR0_FIXED0);
    cr0 &= __readmsr(IA32_VMX_CR0_FIXED1);
    cr4 |= __readmsr(IA32_VMX_CR4_FIXED0);
    cr4 &= __readmsr(IA32_VMX_CR4_FIXED1);


    __writecr0(cr0);
    __writecr4(cr4);

    _enable();

#ifdef DBG
    //��Ҫ����fixed �޸�����Щλ
    print("[+] ori cr0 %x  now cr0 %x\n", _cached_data._cr0, __readcr0());
    print("[+] ori cr4 %x  now cr4 %x\n", _cached_data._cr4, __readcr4());
#endif
}


void init_vmcs_control_fields() {

    // Pin-based VM-Excution control�ֶ�
    // intel��3 A.3.1 24.6.1 
    ia32_vmx_pinbased_ctls_register pin_based_ctrl;
    pin_based_ctrl.flags = 0;     //0��ʼ��  
    write_ctrl_pin_based_safe(pin_based_ctrl);

#ifdef DBG  
    __vmx_vmread(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, &pin_based_ctrl.flags);
    print("[+] Pin-based VM-Excution control  %u\n", pin_based_ctrl.flags);
#endif // DBG


    // Processor-Based VM-Execution Controls
    // intel��3 24.6.2
    ia32_vmx_procbased_ctls_register proc_based_ctrl;
    proc_based_ctrl.flags = 0;
    proc_based_ctrl.use_msr_bitmaps = 1;    //msr bitmap����ʹ��,��Ȼÿ��rdmsr��wdmsr����vm-exit
    proc_based_ctrl.activate_secondary_controls = 1;
    // ����4K bitmap
    // ���������⻯����3.5.15
    vmx_msr_bitmap* msr_bitmap = (vmx_msr_bitmap*)ExAllocatePool(NonPagedPool, sizeof(vmx_msr_bitmap));
    NT_ASSERT(msr_bitmap);
    RtlZeroMemory(msr_bitmap, 0x1000);
    __vmx_vmwrite(VMCS_CTRL_MSR_BITMAP_ADDRESS, get_physical(msr_bitmap));
    write_ctrl_proc_based_safe(proc_based_ctrl);

    
    ia32_vmx_procbased_ctls2_register proc_based_ctrl2;
    proc_based_ctrl2.flags = 0;
    proc_based_ctrl2.enable_rdtscp = 1;         //����λ�Ļ�ִ��rdtscp�����#UD   
    proc_based_ctrl2.enable_invpcid = 1;        //ͬ��
    proc_based_ctrl2.enable_xsaves = 1;         //ͬ��
    write_ctrl_proc_based2_safe(proc_based_ctrl2);

    ia32_vmx_exit_ctls_register exit_ctrl;
    exit_ctrl.flags = 0;
    exit_ctrl.host_address_space_size = 1;     
    write_ctrl_exit_safe(exit_ctrl);

    ia32_vmx_entry_ctls_register entry_ctrl;
    entry_ctrl.flags = 0;
    entry_ctrl.ia32e_mode_guest = 1; //intel3��26.3.1.1��6��
    write_ctrl_entry_safe(entry_ctrl);

}

// https://github.com/torvalds/linux/blob/c6dd78fcb8eefa15dd861889e0f59d301cb5230c/tools/testing/selftests/kvm/lib/x86_64/vmx.c#L195
// ���������⻯����3.9
// intel3��24.5
void init_vmcs_host_state() {

    //u64 exit_controls;      //ʵ������32λ���ֶ�
    //__vmx_vmread(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS,&exit_controls);
    //print("[+] exit_controls %d\n",(u32)exit_controls);

    // host�μĴ���(&0xf8���Ǳ�֤CPL��TIΪ0,Ҳ����0��Ȩ�޺�ʹ��GDT��)
    // ���������⻯����4.4.4.3 ��selector�ֶεļ��
    NT_ASSERT(__vmx_vmwrite(VMCS_HOST_ES_SELECTOR, x86::read_es().flags & 0xf8) == 0);
    __vmx_vmwrite(VMCS_HOST_CS_SELECTOR, x86::read_cs().flags & 0xf8);
    __vmx_vmwrite(VMCS_HOST_SS_SELECTOR, x86::read_ss().flags & 0xf8);
    __vmx_vmwrite(VMCS_HOST_DS_SELECTOR, x86::read_ds().flags & 0xf8);
    __vmx_vmwrite(VMCS_HOST_FS_SELECTOR, x86::read_fs().flags & 0xf8);
    __vmx_vmwrite(VMCS_HOST_GS_SELECTOR, x86::read_gs().flags & 0xf8);
    __vmx_vmwrite(VMCS_HOST_TR_SELECTOR, x86::read_tr().flags & 0xf8);
#ifdef DBG
    print("[+] es %u\n[+] cs %u\n[+] ss %u\n[+] ds %u\n[+] fs %u\n[+] gs %u\n[+] tr %u\n", x86::read_es().flags, x86::read_cs().flags, x86::read_ss().flags, x86::read_ds().flags, x86::read_fs().flags, x86::read_gs().flags, x86::read_tr().flags);
#endif


    //host���ƼĴ���
    __vmx_vmwrite(VMCS_HOST_CR0, __readcr0());
    __vmx_vmwrite(VMCS_HOST_CR3, __readcr3());
    __vmx_vmwrite(VMCS_HOST_CR4, __readcr4());

    // hostָ��ָ��
    __vmx_vmwrite(VMCS_HOST_RIP, (u64)(&vmexit_handler));

    // ����KERNEL_STACK_SIZE�ڴ��host����ջ
    void* host_stack = ExAllocatePool(NonPagedPool, KERNEL_STACK_SIZE);
    u64 host_stack_limit = (((u64)host_stack + KERNEL_STACK_SIZE) & ~0b1111ull) - 8;
    __vmx_vmwrite(VMCS_HOST_RSP, host_stack_limit);
#ifdef DBG
    print("[+] host_stack_limit 0x%llx\n", host_stack_limit);
    print("[+] [core%x] host_stack %p\n", KeGetCurrentProcessorNumber(),host_stack);
#endif // DBG
    NT_ASSERT(host_stack != 0);         //�ڴ治��

    //host��gs��ַ(��Ϊhost�϶���������64λ�ں�ģʽ��,fsû����)
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

#ifdef DBG
    print("[+] host gdt.base 0x%llx\n", gdt.base_address);
    print("[+] host idt.base 0x%llx\n", idt.base_address);
#endif // DBG


    

    //���tr��ַ����ûɶ�õİ취
    segment_selector tr = x86::read_tr();
    NT_ASSERT(tr.table == 0);               //windowsӦ���ǲ�����ldt��
    u64 tss_base = segment_base(gdt,tr);
#ifdef DBG
    print("[+] tss_base 0x%llx\n", tss_base);
#endif // DBG
    __vmx_vmwrite(VMCS_HOST_TR_BASE, tss_base);

}

// https://github.com/torvalds/linux/blob/c6dd78fcb8eefa15dd861889e0f59d301cb5230c/tools/testing/selftests/kvm/lib/x86_64/vmx.c#L235
void init_vmcs_guest_state() {
    __vmx_vmwrite(VMCS_GUEST_CR3, __readcr3());

    __vmx_vmwrite(VMCS_GUEST_CR0, __readcr0());
    __vmx_vmwrite(VMCS_GUEST_CR4,__readcr4());

    __vmx_vmwrite(VMCS_GUEST_DR7, __readdr(7));

    // RIP and RSP are set in vm-launch.asm
    __vmx_vmwrite(VMCS_GUEST_RSP, 0);
    __vmx_vmwrite(VMCS_GUEST_RIP, 0);

    __vmx_vmwrite(VMCS_GUEST_RFLAGS, __readeflags());

    __vmx_vmwrite(VMCS_GUEST_CS_SELECTOR, x86::read_cs().flags);
    __vmx_vmwrite(VMCS_GUEST_SS_SELECTOR, x86::read_ss().flags);
    __vmx_vmwrite(VMCS_GUEST_DS_SELECTOR, x86::read_ds().flags);
    __vmx_vmwrite(VMCS_GUEST_ES_SELECTOR, x86::read_es().flags);
    __vmx_vmwrite(VMCS_GUEST_FS_SELECTOR, x86::read_fs().flags);
    __vmx_vmwrite(VMCS_GUEST_GS_SELECTOR, x86::read_gs().flags);
    __vmx_vmwrite(VMCS_GUEST_TR_SELECTOR, x86::read_tr().flags);
    __vmx_vmwrite(VMCS_GUEST_LDTR_SELECTOR, x86::read_ldtr().flags);

    segment_descriptor_register_64 gdtr, idtr;
    void _sgdt(void*);
    _sgdt(&gdtr);
    __sidt(&idtr);

    __vmx_vmwrite(VMCS_GUEST_CS_BASE, segment_base(gdtr, x86::read_cs()));
    __vmx_vmwrite(VMCS_GUEST_SS_BASE, segment_base(gdtr, x86::read_ss()));
    __vmx_vmwrite(VMCS_GUEST_DS_BASE, segment_base(gdtr, x86::read_ds()));
    __vmx_vmwrite(VMCS_GUEST_ES_BASE, segment_base(gdtr, x86::read_es()));
    __vmx_vmwrite(VMCS_GUEST_FS_BASE, __readmsr(IA32_FS_BASE));
    __vmx_vmwrite(VMCS_GUEST_GS_BASE, __readmsr(IA32_GS_BASE));
    __vmx_vmwrite(VMCS_GUEST_TR_BASE, segment_base(gdtr, x86::read_tr()));
    __vmx_vmwrite(VMCS_GUEST_LDTR_BASE, segment_base(gdtr, x86::read_ldtr()));

    __vmx_vmwrite(VMCS_GUEST_CS_LIMIT, __segmentlimit(x86::read_cs().flags));
    __vmx_vmwrite(VMCS_GUEST_SS_LIMIT, __segmentlimit(x86::read_ss().flags));
    __vmx_vmwrite(VMCS_GUEST_DS_LIMIT, __segmentlimit(x86::read_ds().flags));
    __vmx_vmwrite(VMCS_GUEST_ES_LIMIT, __segmentlimit(x86::read_es().flags));
    __vmx_vmwrite(VMCS_GUEST_FS_LIMIT, __segmentlimit(x86::read_fs().flags));
    __vmx_vmwrite(VMCS_GUEST_GS_LIMIT, __segmentlimit(x86::read_gs().flags));
    __vmx_vmwrite(VMCS_GUEST_TR_LIMIT, __segmentlimit(x86::read_tr().flags));
    __vmx_vmwrite(VMCS_GUEST_LDTR_LIMIT, __segmentlimit(x86::read_ldtr().flags));

    __vmx_vmwrite(VMCS_GUEST_CS_ACCESS_RIGHTS, segment_access(gdtr, x86::read_cs()).flags);
    __vmx_vmwrite(VMCS_GUEST_SS_ACCESS_RIGHTS, segment_access(gdtr, x86::read_ss()).flags);
    __vmx_vmwrite(VMCS_GUEST_DS_ACCESS_RIGHTS, segment_access(gdtr, x86::read_ds()).flags);
    __vmx_vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS, segment_access(gdtr, x86::read_es()).flags);
    __vmx_vmwrite(VMCS_GUEST_FS_ACCESS_RIGHTS, segment_access(gdtr, x86::read_fs()).flags);
    __vmx_vmwrite(VMCS_GUEST_GS_ACCESS_RIGHTS, segment_access(gdtr, x86::read_gs()).flags);
    __vmx_vmwrite(VMCS_GUEST_TR_ACCESS_RIGHTS, segment_access(gdtr, x86::read_tr()).flags);
    __vmx_vmwrite(VMCS_GUEST_LDTR_ACCESS_RIGHTS, segment_access(gdtr, x86::read_ldtr()).flags);

    __vmx_vmwrite(VMCS_GUEST_GDTR_BASE, gdtr.base_address);
    __vmx_vmwrite(VMCS_GUEST_IDTR_BASE, idtr.base_address);

    __vmx_vmwrite(VMCS_GUEST_GDTR_LIMIT, gdtr.limit);
    __vmx_vmwrite(VMCS_GUEST_IDTR_LIMIT, idtr.limit);

    if(_cached_data.cpuid_01.cpuid_feature_information_edx.page_attribute_table)
        __vmx_vmwrite(VMCS_GUEST_PAT, __readmsr(IA32_PAT));

    //intel3�� 26.3.1.5���һ��
    __vmx_vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, MAXULONG64);

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
    RtlZeroMemory(t, sizeof(hypervisor));

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

    b = vm_launch();
    if (!b) {
        print("[+] vmlaunch fail\n");

        size_t error;
        b = __vmx_vmread(VMCS_VM_INSTRUCTION_ERROR,&error);
        if (!b) {//error��Ӧintel3�� 30.4
            print("[+] vmx error %u\n", error);
        }
    }

    //�ɹ�vmlaunch֮�� guest�����￪ʼ����
    

    
    

    return 0;
}

u64 stop_virtualize_everycpu_ipi_routine(u64 Argument) {

    // Ӧ��������vmcall,��host��vmxoff,������������(guest�������)ֱ��vmxoff
    hypercall_input hi;
    hi.code = hypercall_code::unload;
    vmx_vmcall(hi);

    return 0;
}


void stop_hypervisor() {
    KeIpiGenericCall(stop_virtualize_everycpu_ipi_routine, NULL);
}

