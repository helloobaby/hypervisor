#include "ia32-doc/out/ia32.hpp"
#include <intrin.h>
#include "vmcs_helper.h"

void write_vmcs_ctrl_field(size_t value,
    unsigned long const ctrl_field,
    unsigned long const cap_msr,
    unsigned long const true_cap_msr) {
    ia32_vmx_basic_register vmx_basic;
    vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

    // read the "true" capability msr if it is supported
    auto const cap = __readmsr(vmx_basic.vmx_controls ? true_cap_msr : cap_msr);

    // adjust the control according to the capability msr
    value &= cap >> 32;
    value |= cap & 0xFFFFFFFF;

    // write to the vmcs field
    __vmx_vmwrite(ctrl_field, value);
}


void write_ctrl_pin_based_safe(ia32_vmx_pinbased_ctls_register const value) {
    write_vmcs_ctrl_field(value.flags,
        VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS,
        IA32_VMX_PINBASED_CTLS,
        IA32_VMX_TRUE_PINBASED_CTLS);
}

void write_ctrl_proc_based_safe(ia32_vmx_procbased_ctls_register const value) {
    write_vmcs_ctrl_field(value.flags,
        VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
        IA32_VMX_PROCBASED_CTLS,
        IA32_VMX_TRUE_PROCBASED_CTLS);
}

void write_ctrl_proc_based2_safe(ia32_vmx_procbased_ctls2_register const value) {
    write_vmcs_ctrl_field(value.flags,
        VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
        IA32_VMX_PROCBASED_CTLS2,
        IA32_VMX_PROCBASED_CTLS2);
}

void write_ctrl_exit_safe(ia32_vmx_exit_ctls_register const value) {
    write_vmcs_ctrl_field(value.flags,
        VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS,
        IA32_VMX_EXIT_CTLS,
        IA32_VMX_TRUE_EXIT_CTLS);
}

void write_ctrl_entry_safe(ia32_vmx_entry_ctls_register const value) {
    write_vmcs_ctrl_field(value.flags,
        VMCS_CTRL_VMENTRY_CONTROLS,
        IA32_VMX_ENTRY_CTLS,
        IA32_VMX_TRUE_ENTRY_CTLS);
}