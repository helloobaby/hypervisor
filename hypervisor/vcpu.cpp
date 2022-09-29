//https://github.com/torvalds/linux/blob/c6dd78fcb8eefa15dd861889e0f59d301cb5230c/tools/testing/selftests/kvm/lib/x86_64/vmx.c#L1

#include "vcpu.h"
#include "mm.h"
#include "utils.h"
#include "arch.h"

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

// 检测VMX能否被开启
bool check_hypervisor_can_enable() {

    // 3.23.6       检测cpu本身是否支持VMX技术
    if (!_cached_data.cpuid_01.cpuid_feature_information_ecx.virtual_machine_extensions)
        return false;

    ia32_feature_control_register msr;
    msr.flags = __readmsr(IA32_FEATURE_CONTROL);

    // 3.23.7
    if (!msr.lock_bit || !msr.enable_vmx_outside_smx)    //
        return false;

    return true;
}

// 为当前逻辑核提供vmcs结构

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
    init_vmcs_host_state();
}

//解锁对vmxon指令的限制
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
    //主要看看fixed 修改了哪些位
    print("[+] ori cr0 %d  now cr0 %d\n", _cached_data._cr0, __readcr0());
    print("[+] ori cr4 %d  now cr4 %d\n", _cached_data._cr4, __readcr4());
#endif
}


void init_vmcs_control_fields() {

    //Pin-based VM-Excution control字段
    ia32_vmx_pinbased_ctls_register pin_based_ctrl;
    pin_based_ctrl.flags = 0;
    pin_based_ctrl.virtual_nmi = 1;
    pin_based_ctrl.nmi_exiting = 1;
    pin_based_ctrl.activate_vmx_preemption_timer = 1;



}

// https://github.com/torvalds/linux/blob/c6dd78fcb8eefa15dd861889e0f59d301cb5230c/tools/testing/selftests/kvm/lib/x86_64/vmx.c#L195
// 处理器虚拟化技术3.9
void init_vmcs_host_state() {

    u64 exit_controls;      //实际上是32位的字段
    __vmx_vmread(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS,&exit_controls);
    print("[+] exit_controls %d\n",(u32)exit_controls);

    //host段寄存器
    NT_ASSERT(__vmx_vmwrite(VMCS_HOST_ES_SELECTOR, x86::read_es().flags) == 0);
    __vmx_vmwrite(VMCS_HOST_CS_SELECTOR, x86::read_cs().flags);
    __vmx_vmwrite(VMCS_HOST_SS_SELECTOR, x86::read_ss().flags);
    __vmx_vmwrite(VMCS_HOST_DS_SELECTOR, x86::read_ds().flags);
    __vmx_vmwrite(VMCS_HOST_FS_SELECTOR, x86::read_fs().flags);
    __vmx_vmwrite(VMCS_HOST_GS_SELECTOR, x86::read_gs().flags);
    __vmx_vmwrite(VMCS_HOST_TR_SELECTOR, x86::read_tr().flags);

    //host控制寄存器
    __vmx_vmwrite(VMCS_HOST_CR0, __readcr0());
    __vmx_vmwrite(VMCS_HOST_CR3, __readcr3());
    __vmx_vmwrite(VMCS_HOST_CR4, __readcr4());

    // host指针指针
    __vmx_vmwrite(VMCS_HOST_RIP, (u64)(&vmexit_handler));
    // 分配KERNEL_STACK_SIZE内存给host当堆栈

    //host栈指针
    void* host_stack = ExAllocatePool(NonPagedPool, KERNEL_STACK_SIZE);
    __vmx_vmwrite(VMCS_HOST_RSP, (size_t)host_stack);

    NT_ASSERT(host_stack != 0);
    print("[+][core%x] host_stack %p\n", KeGetCurrentProcessorNumber(),host_stack);

    //host相关msr
    __vmx_vmwrite(VMCS_HOST_SYSENTER_CS, 0);
    __vmx_vmwrite(VMCS_HOST_SYSENTER_ESP, 0);
    __vmx_vmwrite(VMCS_HOST_SYSENTER_EIP, 0);
    
    //host段基址
    


}

void init_vmcs_guest_state() {

}

u64 virtualize_everycpu_ipi_routine(u64 Argument) {

    NT_ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);        //保证无法线程切换

    print("[+] virtualize_everycpu_ipi_routine run core %d\n", KeGetCurrentProcessorNumber());

    enable_vmx_operation();         //使能VMXON

    // 给当前核心分配一份单独的hypervisor数据
    hypervisor* t = (hypervisor*)ExAllocatePool(NonPagedPool, sizeof(hypervisor));
    if (!t) {
        print("[E] cant alloc hypervisor struct\n");
        return 0;
    }


    char b;
    // __vmx_on的参数需要提供一个vmxon结构
    ia32_vmx_basic_register vmx_basic;
    vmx_basic.flags = __readmsr(IA32_VMX_BASIC);
    t->vmxon.revision_id = vmx_basic.vmcs_revision_id;
    t->vmxon.must_be_zero = 0;

    auto phyvmxon = get_physical(&t->vmxon);
    NT_ASSERT((__readcr4() & CR4_VMX_ENABLE_FLAG) != 0);
    NT_ASSERT((phyvmxon % 0x1000) == 0);

    b = __vmx_on(&phyvmxon);        //进入vmx root模式      (vmx_on两次会导致失败,必须先vmxoff)
    if (b) {    //不等于0,失败
        print("[E] vmxon failed\n");
        return 0;
    }

    load_vmcs(t->vmcs);     //不执行这个的话vmread\vmwrite都会报错
    prepare_vmcs();

    test_vmcs_field();
    




    
    

    return 0;
}

u64 stop_virtualize_everycpu_ipi_routine(u64 Argument) {

    __vmx_off();

    return 0;
}


void stop_hypervisor() {
    KeIpiGenericCall(stop_virtualize_everycpu_ipi_routine, NULL);
}


// 有些保留字段设置为0或设置为1是由MSR规定的
// 这个函数用来分析哪些保留位是设置为1,哪些设置为0
void test_vmcs_field() {
    
    // 32bit vector
    u32 pin_base_value = read_ctrl_field(IA32_VMX_PINBASED_CTLS, IA32_VMX_TRUE_PINBASED_CTLS);
    print("[+] pin_base_value %d\n", pin_base_value);

}

// intel3卷A.4
u64 read_ctrl_field(u64 a, u64 true_a) {
    ia32_vmx_basic_register vmx_basic;
    vmx_basic.flags = __readmsr(IA32_VMX_BASIC);
    return __readmsr(vmx_basic.vmx_controls ? true_a : a);
}