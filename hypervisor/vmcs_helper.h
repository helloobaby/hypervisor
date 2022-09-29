
void write_vmcs_ctrl_field(size_t value,
    unsigned long const ctrl_field,
    unsigned long const cap_msr,
    unsigned long const true_cap_msr);


void write_ctrl_pin_based_safe(ia32_vmx_pinbased_ctls_register const value);

void write_ctrl_proc_based_safe(ia32_vmx_procbased_ctls_register const value);

void write_ctrl_proc_based2_safe(ia32_vmx_procbased_ctls2_register const value);

void write_ctrl_exit_safe(ia32_vmx_exit_ctls_register const value);

void write_ctrl_entry_safe(ia32_vmx_entry_ctls_register const value);